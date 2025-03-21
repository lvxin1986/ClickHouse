#include <Core/NamesAndTypes.h>
#include <Core/SortCursor.h>
#include <Columns/ColumnNullable.h>
#include <Interpreters/MergeJoin.h>
#include <Interpreters/AnalyzedJoin.h>
#include <Interpreters/sortBlock.h>
#include <Interpreters/join_common.h>
#include <DataStreams/materializeBlock.h>
#include <DataStreams/MergeSortingBlockInputStream.h>

namespace DB
{

namespace ErrorCodes
{
    extern const int SET_SIZE_LIMIT_EXCEEDED;
    extern const int NOT_IMPLEMENTED;
    extern const int PARAMETER_OUT_OF_BOUND;
    extern const int LOGICAL_ERROR;
}

namespace
{

template <bool has_nulls>
int nullableCompareAt(const IColumn & left_column, const IColumn & right_column, size_t lhs_pos, size_t rhs_pos)
{
    static constexpr int null_direction_hint = 1;

    if constexpr (has_nulls)
    {
        auto * left_nullable = checkAndGetColumn<ColumnNullable>(left_column);
        auto * right_nullable = checkAndGetColumn<ColumnNullable>(right_column);

        if (left_nullable && right_nullable)
        {
            int res = left_column.compareAt(lhs_pos, rhs_pos, right_column, null_direction_hint);
            if (res)
                return res;

            /// NULL != NULL case
            if (left_column.isNullAt(lhs_pos))
                return null_direction_hint;
        }

        if (left_nullable && !right_nullable)
        {
            if (left_column.isNullAt(lhs_pos))
                return null_direction_hint;
            return left_nullable->getNestedColumn().compareAt(lhs_pos, rhs_pos, right_column, null_direction_hint);
        }

        if (!left_nullable && right_nullable)
        {
            if (right_column.isNullAt(rhs_pos))
                return -null_direction_hint;
            return left_column.compareAt(lhs_pos, rhs_pos, right_nullable->getNestedColumn(), null_direction_hint);
        }
    }

    /// !left_nullable && !right_nullable
    return left_column.compareAt(lhs_pos, rhs_pos, right_column, null_direction_hint);
}

Block extractMinMax(const Block & block, const Block & keys)
{
    if (block.rows() == 0)
        throw Exception("Unexpected empty block", ErrorCodes::LOGICAL_ERROR);

    Block min_max = keys.cloneEmpty();
    MutableColumns columns = min_max.mutateColumns();

    for (size_t i = 0; i < columns.size(); ++i)
    {
        auto & src_column = block.getByName(keys.getByPosition(i).name);

        columns[i]->insertFrom(*src_column.column, 0);
        columns[i]->insertFrom(*src_column.column, block.rows() - 1);
    }

    min_max.setColumns(std::move(columns));
    return min_max;
}

}

struct MergeJoinEqualRange
{
    size_t left_start = 0;
    size_t right_start = 0;
    size_t left_length = 0;
    size_t right_length = 0;

    bool empty() const { return !left_length && !right_length; }
};

using Range = MergeJoinEqualRange;


class MergeJoinCursor
{
public:
    MergeJoinCursor(const Block & block, const SortDescription & desc_)
        : impl(SortCursorImpl(block, desc_))
    {}

    size_t position() const { return impl.pos; }
    size_t end() const { return impl.rows; }
    bool atEnd() const { return impl.pos >= impl.rows; }
    void nextN(size_t num) { impl.pos += num; }

    void setCompareNullability(const MergeJoinCursor & rhs)
    {
        has_nullable_columns = false;

        for (size_t i = 0; i < impl.sort_columns_size; ++i)
        {
            bool is_left_nullable = isColumnNullable(*impl.sort_columns[i]);
            bool is_right_nullable = isColumnNullable(*rhs.impl.sort_columns[i]);

            if (is_left_nullable || is_right_nullable)
            {
                has_nullable_columns = true;
                break;
            }
        }
    }

    Range getNextEqualRange(MergeJoinCursor & rhs)
    {
        if (has_nullable_columns)
            return getNextEqualRangeImpl<true>(rhs);
        return getNextEqualRangeImpl<false>(rhs);
    }

    int intersect(const Block & right_block, const Block & right_table_keys, const Names & key_names)
    {
        const Block min_max = extractMinMax(right_block, right_table_keys);
        if (end() == 0 || min_max.rows() != 2)
            throw Exception("Unexpected block size", ErrorCodes::LOGICAL_ERROR);

        size_t last_position = end() - 1;
        int first_vs_max = 0;
        int last_vs_min = 0;

        for (size_t i = 0; i < impl.sort_columns.size(); ++i)
        {
            auto & left_column = *impl.sort_columns[i];
            auto & right_column = *min_max.getByName(key_names[i]).column; /// cannot get by position cause of possible duplicates

            if (!first_vs_max)
                first_vs_max = nullableCompareAt<true>(left_column, right_column, position(), 1);

            if (!last_vs_min)
                last_vs_min = nullableCompareAt<true>(left_column, right_column, last_position, 0);
        }

        if (first_vs_max > 0)
            return 1;
        if (last_vs_min < 0)
            return -1;
        return 0;
    }

private:
    SortCursorImpl impl;
    bool has_nullable_columns = false;

    template <bool has_nulls>
    Range getNextEqualRangeImpl(MergeJoinCursor & rhs)
    {
        while (!atEnd() && !rhs.atEnd())
        {
            int cmp = compareAt<has_nulls>(rhs, impl.pos, rhs.impl.pos);
            if (cmp < 0)
                impl.next();
            if (cmp > 0)
                rhs.impl.next();
            if (!cmp)
            {
                Range range{impl.pos, rhs.impl.pos, 0, 0};
                range.left_length = getEqualLength();
                range.right_length = rhs.getEqualLength();
                return range;
            }
        }

        return Range{impl.pos, rhs.impl.pos, 0, 0};
    }

    template <bool has_nulls>
    int compareAt(const MergeJoinCursor & rhs, size_t lhs_pos, size_t rhs_pos) const
    {
        int res = 0;
        for (size_t i = 0; i < impl.sort_columns_size; ++i)
        {
            auto * left_column = impl.sort_columns[i];
            auto * right_column = rhs.impl.sort_columns[i];

            res = nullableCompareAt<has_nulls>(*left_column, *right_column, lhs_pos, rhs_pos);
            if (res)
                break;
        }
        return res;
    }

    size_t getEqualLength()
    {
        if (atEnd())
            return 0;

        size_t pos = impl.pos;
        while (sameNext(pos))
            ++pos;
        return pos - impl.pos + 1;
    }

    bool sameNext(size_t lhs_pos) const
    {
        if (lhs_pos + 1 >= impl.rows)
            return false;

        for (size_t i = 0; i < impl.sort_columns_size; ++i)
            if (impl.sort_columns[i]->compareAt(lhs_pos, lhs_pos + 1, *(impl.sort_columns[i]), 1) != 0)
                return false;
        return true;
    }
};

namespace
{

MutableColumns makeMutableColumns(const Block & block, size_t rows_to_reserve = 0)
{
    MutableColumns columns;
    columns.reserve(block.columns());

    for (const auto & src_column : block)
    {
        columns.push_back(src_column.column->cloneEmpty());
        columns.back()->reserve(rows_to_reserve);
    }
    return columns;
}

void makeSortAndMerge(const Names & keys, SortDescription & sort, SortDescription & merge)
{
    NameSet unique_keys;
    for (auto & key_name : keys)
    {
        merge.emplace_back(SortColumnDescription(key_name, 1, 1));

        if (!unique_keys.count(key_name))
        {
            unique_keys.insert(key_name);
            sort.emplace_back(SortColumnDescription(key_name, 1, 1));
        }
    }
}

void copyLeftRange(const Block & block, MutableColumns & columns, size_t start, size_t rows_to_add)
{
    for (size_t i = 0; i < block.columns(); ++i)
    {
        const auto & src_column = block.getByPosition(i).column;
        columns[i]->insertRangeFrom(*src_column, start, rows_to_add);
    }
}

void copyRightRange(const Block & right_block, const Block & right_columns_to_add, MutableColumns & columns,
                    size_t row_position, size_t rows_to_add)
{
    for (size_t i = 0; i < right_columns_to_add.columns(); ++i)
    {
        const auto & src_column = right_block.getByName(right_columns_to_add.getByPosition(i).name).column;
        auto & dst_column = columns[i];
        auto * dst_nullable = typeid_cast<ColumnNullable *>(dst_column.get());

        if (dst_nullable && !isColumnNullable(*src_column))
            dst_nullable->insertManyFromNotNullable(*src_column, row_position, rows_to_add);
        else
            dst_column->insertManyFrom(*src_column, row_position, rows_to_add);
    }
}

void joinEqualsAnyLeft(const Block & right_block, const Block & right_columns_to_add, MutableColumns & right_columns, const Range & range)
{
    copyRightRange(right_block, right_columns_to_add, right_columns, range.right_start, range.left_length);
}

void joinEquals(const Block & left_block, const Block & right_block, const Block & right_columns_to_add,
                MutableColumns & left_columns, MutableColumns & right_columns, const Range & range, bool is_all)
{
    size_t left_rows_to_add = range.left_length;
    size_t right_rows_to_add = is_all ? range.right_length : 1;

    size_t row_position = range.right_start;
    for (size_t right_row = 0; right_row < right_rows_to_add; ++right_row, ++row_position)
    {
        copyLeftRange(left_block, left_columns, range.left_start, left_rows_to_add);
        copyRightRange(right_block, right_columns_to_add, right_columns, row_position, left_rows_to_add);
    }
}

void appendNulls(MutableColumns & right_columns, size_t rows_to_add)
{
    for (auto & column : right_columns)
        column->insertManyDefaults(rows_to_add);
}

void joinInequalsLeft(const Block & left_block, MutableColumns & left_columns, MutableColumns & right_columns,
                      size_t start, size_t end, bool copy_left)
{
    if (end <= start)
        return;

    size_t rows_to_add = end - start;
    if (copy_left)
        copyLeftRange(left_block, left_columns, start, rows_to_add);
    appendNulls(right_columns, rows_to_add);
}

}



MergeJoin::MergeJoin(std::shared_ptr<AnalyzedJoin> table_join_, const Block & right_sample_block)
    : table_join(table_join_)
    , nullable_right_side(table_join->forceNullableRight())
    , is_all(table_join->strictness() == ASTTableJoin::Strictness::All)
    , is_inner(isInner(table_join->kind()))
    , is_left(isLeft(table_join->kind()))
    , skip_not_intersected(table_join->enablePartialMergeJoinOptimisations())
{
    if (!isLeft(table_join->kind()) && !isInner(table_join->kind()))
        throw Exception("Partial merge supported for LEFT and INNER JOINs only", ErrorCodes::NOT_IMPLEMENTED);

    JoinCommon::extractKeysForJoin(table_join->keyNamesRight(), right_sample_block, right_table_keys, right_columns_to_add);

    const NameSet required_right_keys = table_join->requiredRightKeys();
    for (const auto & column : right_table_keys)
        if (required_right_keys.count(column.name))
            right_columns_to_add.insert(ColumnWithTypeAndName{nullptr, column.type, column.name});

    JoinCommon::removeLowCardinalityInplace(right_columns_to_add);
    JoinCommon::createMissedColumns(right_columns_to_add);

    if (nullable_right_side)
        JoinCommon::convertColumnsToNullable(right_columns_to_add);

    makeSortAndMerge(table_join->keyNamesLeft(), left_sort_description, left_merge_description);
    makeSortAndMerge(table_join->keyNamesRight(), right_sort_description, right_merge_description);
}

void MergeJoin::setTotals(const Block & totals_block)
{
    totals = totals_block;
    mergeRightBlocks();
}

void MergeJoin::joinTotals(Block & block) const
{
    JoinCommon::joinTotals(totals, right_columns_to_add, table_join->keyNamesRight(), block);
}

void MergeJoin::mergeRightBlocks()
{
    if (right_blocks.empty())
        return;

    Blocks unsorted_blocks;
    unsorted_blocks.reserve(right_blocks.size());
    for (const auto & block : right_blocks)
        unsorted_blocks.push_back(block);

    size_t max_rows_in_block = table_join->maxRowsInRightBlock();
    if (!max_rows_in_block)
        throw Exception("partial_merge_join_rows_in_right_blocks cannot be zero", ErrorCodes::PARAMETER_OUT_OF_BOUND);

    /// TODO: there should be no splitted keys by blocks for RIGHT|FULL JOIN
    MergeSortingBlocksBlockInputStream stream(unsorted_blocks, right_sort_description, max_rows_in_block);

    right_blocks.clear();
    while (Block block = stream.read())
        right_blocks.emplace_back(std::move(block));
}

bool MergeJoin::addJoinedBlock(const Block & src_block)
{
    Block block = materializeBlock(src_block);
    JoinCommon::removeLowCardinalityInplace(block);

    sortBlock(block, right_sort_description);

    std::unique_lock lock(rwlock);

    right_blocks.push_back(block);
    right_blocks_row_count += block.rows();
    right_blocks_bytes += block.bytes();

    return table_join->sizeLimits().check(right_blocks_row_count, right_blocks_bytes, "JOIN", ErrorCodes::SET_SIZE_LIMIT_EXCEEDED);
}

void MergeJoin::joinBlock(Block & block)
{
    JoinCommon::checkTypesOfKeys(block, table_join->keyNamesLeft(), right_table_keys, table_join->keyNamesRight());
    materializeBlockInplace(block);
    JoinCommon::removeLowCardinalityInplace(block);

    sortBlock(block, left_sort_description);

    std::shared_lock lock(rwlock);

    size_t rows_to_reserve = is_left ? block.rows() : 0;
    MutableColumns left_columns = makeMutableColumns(block, (is_all ? rows_to_reserve : 0));
    MutableColumns right_columns = makeMutableColumns(right_columns_to_add, rows_to_reserve);
    MergeJoinCursor left_cursor(block, left_merge_description);
    size_t left_key_tail = 0;

    if (is_left)
    {
        for (auto it = right_blocks.begin(); it != right_blocks.end(); ++it)
        {
            if (left_cursor.atEnd())
                break;

            if (skip_not_intersected)
            {
                int intersection = left_cursor.intersect(*it, right_table_keys, table_join->keyNamesRight());
                if (intersection < 0)
                    break; /// (left) ... (right)
                if (intersection > 0)
                    continue; /// (right) ... (left)
            }

            leftJoin(left_cursor, block, *it, left_columns, right_columns, left_key_tail);
        }

        left_cursor.nextN(left_key_tail);
        joinInequalsLeft(block, left_columns, right_columns, left_cursor.position(), left_cursor.end(), is_all);
        //left_cursor.nextN(left_cursor.end() - left_cursor.position());

        changeLeftColumns(block, std::move(left_columns));
        addRightColumns(block, std::move(right_columns));
    }
    else if (is_inner)
    {
        for (auto it = right_blocks.begin(); it != right_blocks.end(); ++it)
        {
            if (left_cursor.atEnd())
                break;

            if (skip_not_intersected)
            {
                int intersection = left_cursor.intersect(*it, right_table_keys, table_join->keyNamesRight());
                if (intersection < 0)
                    break; /// (left) ... (right)
                if (intersection > 0)
                    continue; /// (right) ... (left)
            }

            innerJoin(left_cursor, block, *it, left_columns, right_columns, left_key_tail);
        }

        left_cursor.nextN(left_key_tail);
        changeLeftColumns(block, std::move(left_columns));
        addRightColumns(block, std::move(right_columns));
    }
}

void MergeJoin::leftJoin(MergeJoinCursor & left_cursor, const Block & left_block, const Block & right_block,
                         MutableColumns & left_columns, MutableColumns & right_columns, size_t & left_key_tail)
{
    MergeJoinCursor right_cursor(right_block, right_merge_description);
    left_cursor.setCompareNullability(right_cursor);

    while (!left_cursor.atEnd() && !right_cursor.atEnd())
    {
        /// Not zero left_key_tail means there were equality for the last left key in previous leftJoin() call.
        /// Do not join it twice: join only if it's equal with a first right key of current leftJoin() call and skip otherwise.
        size_t left_unequal_position = left_cursor.position() + left_key_tail;
        left_key_tail = 0;

        Range range = left_cursor.getNextEqualRange(right_cursor);

        joinInequalsLeft(left_block, left_columns, right_columns, left_unequal_position, range.left_start, is_all);

        if (range.empty())
            break;

        if (is_all)
            joinEquals(left_block, right_block, right_columns_to_add, left_columns, right_columns, range, is_all);
        else
            joinEqualsAnyLeft(right_block, right_columns_to_add, right_columns, range);

        right_cursor.nextN(range.right_length);

        /// Do not run over last left keys for ALL JOIN (cause of possible duplicates in next right block)
        if (is_all && right_cursor.atEnd())
        {
            left_key_tail = range.left_length;
            break;
        }
        left_cursor.nextN(range.left_length);
    }
}

void MergeJoin::innerJoin(MergeJoinCursor & left_cursor, const Block & left_block, const Block & right_block,
                          MutableColumns & left_columns, MutableColumns & right_columns, size_t & left_key_tail)
{
    MergeJoinCursor right_cursor(right_block, right_merge_description);
    left_cursor.setCompareNullability(right_cursor);

    while (!left_cursor.atEnd() && !right_cursor.atEnd())
    {
        Range range = left_cursor.getNextEqualRange(right_cursor);
        if (range.empty())
            break;

        joinEquals(left_block, right_block, right_columns_to_add, left_columns, right_columns, range, is_all);
        right_cursor.nextN(range.right_length);

        /// Do not run over last left keys for ALL JOIN (cause of possible duplicates in next right block)
        if (is_all && right_cursor.atEnd())
        {
            left_key_tail = range.left_length;
            break;
        }
        left_cursor.nextN(range.left_length);
    }
}

void MergeJoin::changeLeftColumns(Block & block, MutableColumns && columns)
{
    if (is_left && !is_all)
        return;
    block.setColumns(std::move(columns));
}

void MergeJoin::addRightColumns(Block & block, MutableColumns && right_columns)
{
    for (size_t i = 0; i < right_columns_to_add.columns(); ++i)
    {
        const auto & column = right_columns_to_add.getByPosition(i);
        block.insert(ColumnWithTypeAndName{std::move(right_columns[i]), column.type, column.name});
    }
}

}
