#pragma once

#include "SettingsCommon.h"
#include <Core/Defines.h>


namespace Poco
{
    namespace Util
    {
        class AbstractConfiguration;
    }
}

namespace boost
{
    namespace program_options
    {
        class options_description;
    }
}


namespace DB
{

class IColumn;


/** Settings of query execution.
  */
struct Settings : public SettingsCollection<Settings>
{
    /// For initialization from empty initializer-list to be "value initialization", not "aggregate initialization" in C++14.
    /// http://en.cppreference.com/w/cpp/language/aggregate_initialization
    Settings() {}

    /** List of settings: type, name, default value.
      *
      * This looks rather unconvenient. It is done that way to avoid repeating settings in different places.
      * Note: as an alternative, we could implement settings to be completely dynamic in form of map: String -> Field,
      *  but we are not going to do it, because settings is used everywhere as static struct fields.
      */

#define LIST_OF_SETTINGS(M)                                            \
    M(SettingUInt64, min_compress_block_size, 65536, "The actual size of the block to compress, if the uncompressed data less than max_compress_block_size is no less than this value and no less than the volume of data for one mark.") \
    M(SettingUInt64, max_compress_block_size, 1048576, "The maximum size of blocks of uncompressed data before compressing for writing to a table.") \
    M(SettingUInt64, max_block_size, DEFAULT_BLOCK_SIZE, "Maximum block size for reading") \
    M(SettingUInt64, max_insert_block_size, DEFAULT_INSERT_BLOCK_SIZE, "The maximum block size for insertion, if we control the creation of blocks for insertion.") \
    M(SettingUInt64, min_insert_block_size_rows, DEFAULT_INSERT_BLOCK_SIZE, "Squash blocks passed to INSERT query to specified size in rows, if blocks are not big enough.") \
    M(SettingUInt64, min_insert_block_size_bytes, (DEFAULT_INSERT_BLOCK_SIZE * 256), "Squash blocks passed to INSERT query to specified size in bytes, if blocks are not big enough.") \
    M(SettingMaxThreads, max_threads, 0, "The maximum number of threads to execute the request. By default, it is determined automatically.") \
    M(SettingMaxThreads, max_alter_threads, 0, "The maximum number of threads to execute the ALTER requests. By default, it is determined automatically.") \
    M(SettingUInt64, max_read_buffer_size, DBMS_DEFAULT_BUFFER_SIZE, "The maximum size of the buffer to read from the filesystem.") \
    M(SettingUInt64, max_distributed_connections, 1024, "The maximum number of connections for distributed processing of one query (should be greater than max_threads).") \
    M(SettingUInt64, max_query_size, 262144, "Which part of the query can be read into RAM for parsing (the remaining data for INSERT, if any, is read later)") \
    M(SettingUInt64, interactive_delay, 100000, "The interval in microseconds to check if the request is cancelled, and to send progress info.") \
    M(SettingSeconds, connect_timeout, DBMS_DEFAULT_CONNECT_TIMEOUT_SEC, "Connection timeout if there are no replicas.") \
    M(SettingMilliseconds, connect_timeout_with_failover_ms, DBMS_DEFAULT_CONNECT_TIMEOUT_WITH_FAILOVER_MS, "Connection timeout for selecting first healthy replica.") \
    M(SettingSeconds, receive_timeout, DBMS_DEFAULT_RECEIVE_TIMEOUT_SEC, "") \
    M(SettingSeconds, send_timeout, DBMS_DEFAULT_SEND_TIMEOUT_SEC, "") \
    M(SettingSeconds, tcp_keep_alive_timeout, 0, "The time in seconds the connection needs to remain idle before TCP starts sending keepalive probes") \
    M(SettingMilliseconds, queue_max_wait_ms, 0, "The wait time in the request queue, if the number of concurrent requests exceeds the maximum.") \
    M(SettingMilliseconds, connection_pool_max_wait_ms, 0, "The wait time when connection pool is full.") \
    M(SettingMilliseconds, replace_running_query_max_wait_ms, 5000, "The wait time for running query with the same query_id to finish when setting 'replace_running_query' is active.") \
    M(SettingMilliseconds, kafka_max_wait_ms, 5000, "The wait time for reading from Kafka before retry.") \
    M(SettingUInt64, poll_interval, DBMS_DEFAULT_POLL_INTERVAL, "Block at the query wait loop on the server for the specified number of seconds.") \
    M(SettingUInt64, idle_connection_timeout, 3600, "Close idle TCP connections after specified number of seconds.") \
    M(SettingUInt64, distributed_connections_pool_size, DBMS_DEFAULT_DISTRIBUTED_CONNECTIONS_POOL_SIZE, "Maximum number of connections with one remote server in the pool.") \
    M(SettingUInt64, connections_with_failover_max_tries, DBMS_CONNECTION_POOL_WITH_FAILOVER_DEFAULT_MAX_TRIES, "The maximum number of attempts to connect to replicas.") \
    M(SettingUInt64, s3_min_upload_part_size, 512*1024*1024, "The mininum size of part to upload during multipart upload to S3.") \
    M(SettingBool, extremes, false, "Calculate minimums and maximums of the result columns. They can be output in JSON-formats.") \
    M(SettingBool, use_uncompressed_cache, true, "Whether to use the cache of uncompressed blocks.") \
    M(SettingBool, replace_running_query, false, "Whether the running request should be canceled with the same id as the new one.") \
    M(SettingUInt64, background_pool_size, 16, "Number of threads performing background work for tables (for example, merging in merge tree). Only has meaning at server startup.") \
    M(SettingUInt64, background_schedule_pool_size, 16, "Number of threads performing background tasks for replicated tables. Only has meaning at server startup.") \
    \
    M(SettingMilliseconds, distributed_directory_monitor_sleep_time_ms, 100, "Sleep time for StorageDistributed DirectoryMonitors, in case of any errors delay grows exponentially.") \
    M(SettingMilliseconds, distributed_directory_monitor_max_sleep_time_ms, 30000, "Maximum sleep time for StorageDistributed DirectoryMonitors, it limits exponential growth too.") \
    \
    M(SettingBool, distributed_directory_monitor_batch_inserts, false, "Should StorageDistributed DirectoryMonitors try to batch individual inserts into bigger ones.") \
    \
    M(SettingBool, optimize_move_to_prewhere, true, "Allows disabling WHERE to PREWHERE optimization in SELECT queries from MergeTree.") \
    \
    M(SettingUInt64, replication_alter_partitions_sync, 1, "Wait for actions to manipulate the partitions. 0 - do not wait, 1 - wait for execution only of itself, 2 - wait for everyone.") \
    M(SettingUInt64, replication_alter_columns_timeout, 60, "Wait for actions to change the table structure within the specified number of seconds. 0 - wait unlimited time.") \
    \
    M(SettingLoadBalancing, load_balancing, LoadBalancing::RANDOM, "Which replicas (among healthy replicas) to preferably send a query to (on the first attempt) for distributed processing.") \
    \
    M(SettingTotalsMode, totals_mode, TotalsMode::AFTER_HAVING_EXCLUSIVE, "How to calculate TOTALS when HAVING is present, as well as when max_rows_to_group_by and group_by_overflow_mode = ‘any’ are present.") \
    M(SettingFloat, totals_auto_threshold, 0.5, "The threshold for totals_mode = 'auto'.") \
    \
    M(SettingBool, allow_suspicious_low_cardinality_types, false, "In CREATE TABLE statement allows specifying LowCardinality modifier for types of small fixed size (8 or less). Enabling this may increase merge times and memory consumption.") \
    M(SettingBool, compile_expressions, false, "Compile some scalar functions and operators to native code.") \
    M(SettingUInt64, min_count_to_compile, 3, "The number of structurally identical queries before they are compiled.") \
    M(SettingUInt64, min_count_to_compile_expression, 3, "The number of identical expressions before they are JIT-compiled") \
    M(SettingUInt64, group_by_two_level_threshold, 100000, "From what number of keys, a two-level aggregation starts. 0 - the threshold is not set.") \
    M(SettingUInt64, group_by_two_level_threshold_bytes, 100000000, "From what size of the aggregation state in bytes, a two-level aggregation begins to be used. 0 - the threshold is not set. Two-level aggregation is used when at least one of the thresholds is triggered.") \
    M(SettingBool, distributed_aggregation_memory_efficient, false, "Is the memory-saving mode of distributed aggregation enabled.") \
    M(SettingUInt64, aggregation_memory_efficient_merge_threads, 0, "Number of threads to use for merge intermediate aggregation results in memory efficient mode. When bigger, then more memory is consumed. 0 means - same as 'max_threads'.") \
    \
    M(SettingUInt64, max_parallel_replicas, 1, "The maximum number of replicas of each shard used when the query is executed. For consistency (to get different parts of the same partition), this option only works for the specified sampling key. The lag of the replicas is not controlled.") \
    M(SettingUInt64, parallel_replicas_count, 0, "") \
    M(SettingUInt64, parallel_replica_offset, 0, "") \
    \
    M(SettingBool, skip_unavailable_shards, false, "If 1, ClickHouse silently skips unavailable shards and nodes unresolvable through DNS. Shard is marked as unavailable when none of the replicas can be reached.") \
    \
    M(SettingBool, distributed_group_by_no_merge, false, "Do not merge aggregation states from different servers for distributed query processing - in case it is for certain that there are different keys on different shards.") \
    M(SettingBool, optimize_skip_unused_shards, false, "Assumes that data is distributed by sharding_key. Optimization to skip unused shards if SELECT query filters by sharding_key.") \
    \
    M(SettingUInt64, merge_tree_min_rows_for_concurrent_read, (20 * 8192), "If at least as many lines are read from one file, the reading can be parallelized.") \
    M(SettingUInt64, merge_tree_min_bytes_for_concurrent_read, (24 * 10 * 1024 * 1024), "If at least as many bytes are read from one file, the reading can be parallelized.") \
    M(SettingUInt64, merge_tree_min_rows_for_seek, 0, "You can skip reading more than that number of rows at the price of one seek per file.") \
    M(SettingUInt64, merge_tree_min_bytes_for_seek, 0, "You can skip reading more than that number of bytes at the price of one seek per file.") \
    M(SettingUInt64, merge_tree_coarse_index_granularity, 8, "If the index segment can contain the required keys, divide it into as many parts and recursively check them.") \
    M(SettingUInt64, merge_tree_max_rows_to_use_cache, (128 * 8192), "The maximum number of rows per request, to use the cache of uncompressed data. If the request is large, the cache is not used. (For large queries not to flush out the cache.)") \
    M(SettingUInt64, merge_tree_max_bytes_to_use_cache, (192 * 10 * 1024 * 1024), "The maximum number of rows per request, to use the cache of uncompressed data. If the request is large, the cache is not used. (For large queries not to flush out the cache.)") \
    \
    M(SettingBool, merge_tree_uniform_read_distribution, true, "Distribute read from MergeTree over threads evenly, ensuring stable average execution time of each thread within one read operation.") \
    \
    M(SettingUInt64, mysql_max_rows_to_insert, 65536, "The maximum number of rows in MySQL batch insertion of the MySQL storage engine") \
    \
    M(SettingUInt64, optimize_min_equality_disjunction_chain_length, 3, "The minimum length of the expression `expr = x1 OR ... expr = xN` for optimization ") \
    \
    M(SettingUInt64, min_bytes_to_use_direct_io, 0, "The minimum number of bytes for reading the data with O_DIRECT option during SELECT queries execution. 0 - disabled.") \
    \
    M(SettingBool, force_index_by_date, 0, "Throw an exception if there is a partition key in a table, and it is not used.") \
    M(SettingBool, force_primary_key, 0, "Throw an exception if there is primary key in a table, and it is not used.") \
    \
    M(SettingUInt64, mark_cache_min_lifetime, 10000, "If the maximum size of mark_cache is exceeded, delete only records older than mark_cache_min_lifetime seconds.") \
    \
    M(SettingFloat, max_streams_to_max_threads_ratio, 1, "Allows you to use more sources than the number of threads - to more evenly distribute work across threads. It is assumed that this is a temporary solution, since it will be possible in the future to make the number of sources equal to the number of threads, but for each source to dynamically select available work for itself.") \
    M(SettingFloat, max_streams_multiplier_for_merge_tables, 5, "Ask more streams when reading from Merge table. Streams will be spread across tables that Merge table will use. This allows more even distribution of work across threads and especially helpful when merged tables differ in size.") \
    \
    M(SettingString, network_compression_method, "LZ4", "Allows you to select the method of data compression when writing.") \
    \
    M(SettingInt64, network_zstd_compression_level, 1, "Allows you to select the level of ZSTD compression.") \
    \
    M(SettingUInt64, priority, 0, "Priority of the query. 1 - the highest, higher value - lower priority; 0 - do not use priorities.") \
    M(SettingInt64, os_thread_priority, 0, "If non zero - set corresponding 'nice' value for query processing threads. Can be used to adjust query priority for OS scheduler.") \
    \
    M(SettingBool, log_queries, 0, "Log requests and write the log to the system table.") \
    \
    M(SettingUInt64, log_queries_cut_to_length, 100000, "If query length is greater than specified threshold (in bytes), then cut query when writing to query log. Also limit length of printed query in ordinary text log.") \
    \
    M(SettingDistributedProductMode, distributed_product_mode, DistributedProductMode::DENY, "How are distributed subqueries performed inside IN or JOIN sections?") \
    \
    M(SettingUInt64, max_concurrent_queries_for_user, 0, "The maximum number of concurrent requests per user.") \
    \
    M(SettingBool, insert_deduplicate, true, "For INSERT queries in the replicated table, specifies that deduplication of insertings blocks should be preformed") \
    \
    M(SettingUInt64, insert_quorum, 0, "For INSERT queries in the replicated table, wait writing for the specified number of replicas and linearize the addition of the data. 0 - disabled.") \
    M(SettingMilliseconds, insert_quorum_timeout, 600000, "") \
    M(SettingUInt64, select_sequential_consistency, 0, "For SELECT queries from the replicated table, throw an exception if the replica does not have a chunk written with the quorum; do not read the parts that have not yet been written with the quorum.") \
    M(SettingUInt64, table_function_remote_max_addresses, 1000, "The maximum number of different shards and the maximum number of replicas of one shard in the `remote` function.") \
    M(SettingMilliseconds, read_backoff_min_latency_ms, 1000, "Setting to reduce the number of threads in case of slow reads. Pay attention only to reads that took at least that much time.") \
    M(SettingUInt64, read_backoff_max_throughput, 1048576, "Settings to reduce the number of threads in case of slow reads. Count events when the read bandwidth is less than that many bytes per second.") \
    M(SettingMilliseconds, read_backoff_min_interval_between_events_ms, 1000, "Settings to reduce the number of threads in case of slow reads. Do not pay attention to the event, if the previous one has passed less than a certain amount of time.") \
    M(SettingUInt64, read_backoff_min_events, 2, "Settings to reduce the number of threads in case of slow reads. The number of events after which the number of threads will be reduced.") \
    \
    M(SettingFloat, memory_tracker_fault_probability, 0., "For testing of `exception safety` - throw an exception every time you allocate memory with the specified probability.") \
    \
    M(SettingBool, enable_http_compression, 0, "Compress the result if the client over HTTP said that it understands data compressed by gzip or deflate.") \
    M(SettingInt64, http_zlib_compression_level, 3, "Compression level - used if the client on HTTP said that it understands data compressed by gzip or deflate.") \
    \
    M(SettingBool, http_native_compression_disable_checksumming_on_decompress, 0, "If you uncompress the POST data from the client compressed by the native format, do not check the checksum.") \
    \
    M(SettingString, count_distinct_implementation, "uniqExact", "What aggregate function to use for implementation of count(DISTINCT ...)") \
    \
    M(SettingBool, output_format_write_statistics, true, "Write statistics about read rows, bytes, time elapsed in suitable output formats.") \
    \
    M(SettingBool, add_http_cors_header, false, "Write add http CORS header.") \
    \
    M(SettingUInt64, max_http_get_redirects, 0, "Max number of http GET redirects hops allowed. Make sure additional security measures are in place to prevent a malicious server to redirect your requests to unexpected services.") \
    \
    M(SettingBool, input_format_skip_unknown_fields, false, "Skip columns with unknown names from input data (it works for JSONEachRow, CSVWithNames, TSVWithNames and TSKV formats).") \
    M(SettingBool, input_format_with_names_use_header, false, "For TSVWithNames and CSVWithNames input formats this controls whether format parser is to assume that column data appear in the input exactly as they are specified in the header.") \
    M(SettingBool, input_format_import_nested_json, false, "Map nested JSON data to nested tables (it works for JSONEachRow format).") \
    M(SettingBool, input_format_defaults_for_omitted_fields, true, "For input data calculate default expressions for omitted fields (it works for JSONEachRow format).") \
    M(SettingBool, input_format_null_as_default, false, "For CSV format initialize null fields with default values if data type of this field is not nullable") \
    \
    M(SettingBool, input_format_values_interpret_expressions, true, "For Values format: if field could not be parsed by streaming parser, run SQL parser and try to interpret it as SQL expression.") \
    \
    M(SettingBool, output_format_json_quote_64bit_integers, true, "Controls quoting of 64-bit integers in JSON output format.") \
    \
    M(SettingBool, output_format_json_quote_denormals, false, "Enables '+nan', '-nan', '+inf', '-inf' outputs in JSON output format.") \
    \
    M(SettingBool, output_format_json_escape_forward_slashes, true, "Controls escaping forward slashes for string outputs in JSON output format. This is intended for compatibility with JavaScript. Don't confuse with backslashes that are always escaped.") \
    \
    M(SettingUInt64, output_format_pretty_max_rows, 10000, "Rows limit for Pretty formats.") \
    M(SettingUInt64, output_format_pretty_max_column_pad_width, 250, "Maximum width to pad all values in a column in Pretty formats.") \
    M(SettingBool, output_format_pretty_color, true, "Use ANSI escape sequences to paint colors in Pretty formats") \
    M(SettingUInt64, output_format_parquet_row_group_size, 1000000, "Row group size in rows.") \
    \
    M(SettingBool, use_client_time_zone, false, "Use client timezone for interpreting DateTime string values, instead of adopting server timezone.") \
    \
    M(SettingBool, send_progress_in_http_headers, false, "Send progress notifications using X-ClickHouse-Progress headers. Some clients do not support high amount of HTTP headers (Python requests in particular), so it is disabled by default.") \
    \
    M(SettingUInt64, http_headers_progress_interval_ms, 100, "Do not send HTTP headers X-ClickHouse-Progress more frequently than at each specified interval.") \
    \
    M(SettingBool, fsync_metadata, 1, "Do fsync after changing metadata for tables and databases (.sql files). Could be disabled in case of poor latency on server with high load of DDL queries and high load of disk subsystem.") \
    \
    M(SettingUInt64, input_format_allow_errors_num, 0, "Maximum absolute amount of errors while reading text formats (like CSV, TSV). In case of error, if both absolute and relative values are non-zero, and at least absolute or relative amount of errors is lower than corresponding value, will skip until next line and continue.") \
    M(SettingFloat, input_format_allow_errors_ratio, 0, "Maximum relative amount of errors while reading text formats (like CSV, TSV). In case of error, if both absolute and relative values are non-zero, and at least absolute or relative amount of errors is lower than corresponding value, will skip until next line and continue.") \
    \
    M(SettingBool, join_use_nulls, 0, "Use NULLs for non-joined rows of outer JOINs for types that can be inside Nullable. If false, use default value of corresponding columns data type.") \
    \
    M(SettingJoinStrictness, join_default_strictness, JoinStrictness::ALL, "Set default strictness in JOIN query. Possible values: empty string, 'ANY', 'ALL'. If empty, query without strictness will throw exception.") \
    M(SettingBool, any_join_distinct_right_table_keys, false, "Enable old ANY JOIN logic with many-to-one left-to-right table keys mapping for all ANY JOINs. It leads to confusing not equal results for 't1 ANY LEFT JOIN t2' and 't2 ANY RIGHT JOIN t1'. ANY RIGHT JOIN needs one-to-many keys maping to be consistent with LEFT one.") \
    \
    M(SettingUInt64, preferred_block_size_bytes, 1000000, "") \
    \
    M(SettingUInt64, max_replica_delay_for_distributed_queries, 300, "If set, distributed queries of Replicated tables will choose servers with replication delay in seconds less than the specified value (not inclusive). Zero means do not take delay into account.") \
    M(SettingBool, fallback_to_stale_replicas_for_distributed_queries, 1, "Suppose max_replica_delay_for_distributed_queries is set and all replicas for the queried table are stale. If this setting is enabled, the query will be performed anyway, otherwise the error will be reported.") \
    M(SettingUInt64, preferred_max_column_in_block_size_bytes, 0, "Limit on max column size in block while reading. Helps to decrease cache misses count. Should be close to L2 cache size.") \
    \
    M(SettingBool, insert_distributed_sync, false, "If setting is enabled, insert query into distributed waits until data will be sent to all nodes in cluster.") \
    M(SettingUInt64, insert_distributed_timeout, 0, "Timeout for insert query into distributed. Setting is used only with insert_distributed_sync enabled. Zero value means no timeout.") \
    M(SettingInt64, distributed_ddl_task_timeout, 180, "Timeout for DDL query responses from all hosts in cluster. If a ddl request has not been performed on all hosts, a response will contain a timeout error and a request will be executed in an async mode. Negative value means infinite.") \
    M(SettingMilliseconds, stream_flush_interval_ms, 7500, "Timeout for flushing data from streaming storages.") \
    M(SettingMilliseconds, stream_poll_timeout_ms, 500, "Timeout for polling data from/to streaming storages.") \
    M(SettingString, format_schema, "", "Schema identifier (used by schema-based formats)") \
    M(SettingString, format_schema_rows, "", "Row format string for Template format") \
    M(SettingString, format_schema_rows_between_delimiter, "\n", "Delimiter between rows for Template format") \
    M(SettingBool, insert_allow_materialized_columns, 0, "If setting is enabled, Allow materialized columns in INSERT.") \
    M(SettingSeconds, http_connection_timeout, DEFAULT_HTTP_READ_BUFFER_CONNECTION_TIMEOUT, "HTTP connection timeout.") \
    M(SettingSeconds, http_send_timeout, DEFAULT_HTTP_READ_BUFFER_TIMEOUT, "HTTP send timeout") \
    M(SettingSeconds, http_receive_timeout, DEFAULT_HTTP_READ_BUFFER_TIMEOUT, "HTTP receive timeout") \
    M(SettingBool, optimize_throw_if_noop, false, "If setting is enabled and OPTIMIZE query didn't actually assign a merge then an explanatory exception is thrown") \
    M(SettingBool, use_index_for_in_with_subqueries, true, "Try using an index if there is a subquery or a table expression on the right side of the IN operator.") \
    M(SettingBool, joined_subquery_requires_alias, false, "Force joined subqueries to have aliases for correct name qualification.") \
    M(SettingBool, empty_result_for_aggregation_by_empty_set, false, "Return empty result when aggregating without keys on empty set.") \
    M(SettingBool, allow_distributed_ddl, true, "If it is set to true, then a user is allowed to executed distributed DDL queries.") \
    M(SettingUInt64, odbc_max_field_size, 1024, "Max size of filed can be read from ODBC dictionary. Long strings are truncated.") \
    M(SettingUInt64, query_profiler_real_time_period_ns, 1000000000, "Highly experimental. Period for real clock timer of query profiler (in nanoseconds). Set 0 value to turn off real clock query profiler. Recommended value is at least 10000000 (100 times a second) for single queries or 1000000000 (once a second) for cluster-wide profiling.") \
    M(SettingUInt64, query_profiler_cpu_time_period_ns, 1000000000, "Highly experimental. Period for CPU clock timer of query profiler (in nanoseconds). Set 0 value to turn off CPU clock query profiler. Recommended value is at least 10000000 (100 times a second) for single queries or 1000000000 (once a second) for cluster-wide profiling.") \
    \
    \
    /** Limits during query execution are part of the settings. \
      * Used to provide a more safe execution of queries from the user interface. \
      * Basically, limits are checked for each block (not every row). That is, the limits can be slightly violated. \
      * Almost all limits apply only to SELECTs. \
      * Almost all limits apply to each stream individually. \
      */ \
    \
    M(SettingUInt64, max_rows_to_read, 0, "Limit on read rows from the most 'deep' sources. That is, only in the deepest subquery. When reading from a remote server, it is only checked on a remote server.") \
    M(SettingUInt64, max_bytes_to_read, 0, "Limit on read bytes (after decompression) from the most 'deep' sources. That is, only in the deepest subquery. When reading from a remote server, it is only checked on a remote server.") \
    M(SettingOverflowMode, read_overflow_mode, OverflowMode::THROW, "What to do when the limit is exceeded.") \
    \
    M(SettingUInt64, max_rows_to_group_by, 0, "") \
    M(SettingOverflowModeGroupBy, group_by_overflow_mode, OverflowMode::THROW, "What to do when the limit is exceeded.") \
    M(SettingUInt64, max_bytes_before_external_group_by, 0, "") \
    \
    M(SettingUInt64, max_rows_to_sort, 0, "") \
    M(SettingUInt64, max_bytes_to_sort, 0, "") \
    M(SettingOverflowMode, sort_overflow_mode, OverflowMode::THROW, "What to do when the limit is exceeded.") \
    M(SettingUInt64, max_bytes_before_external_sort, 0, "") \
    M(SettingUInt64, max_bytes_before_remerge_sort, 1000000000, "In case of ORDER BY with LIMIT, when memory usage is higher than specified threshold, perform additional steps of merging blocks before final merge to keep just top LIMIT rows.") \
    \
    M(SettingUInt64, max_result_rows, 0, "Limit on result size in rows. Also checked for intermediate data sent from remote servers.") \
    M(SettingUInt64, max_result_bytes, 0, "Limit on result size in bytes (uncompressed). Also checked for intermediate data sent from remote servers.") \
    M(SettingOverflowMode, result_overflow_mode, OverflowMode::THROW, "What to do when the limit is exceeded.") \
    \
    /* TODO: Check also when merging and finalizing aggregate functions. */ \
    M(SettingSeconds, max_execution_time, 0, "") \
    M(SettingOverflowMode, timeout_overflow_mode, OverflowMode::THROW, "What to do when the limit is exceeded.") \
    \
    M(SettingUInt64, min_execution_speed, 0, "Minimum number of execution rows per second.") \
    M(SettingUInt64, max_execution_speed, 0, "Maximum number of execution rows per second.") \
    M(SettingUInt64, min_execution_speed_bytes, 0, "Minimum number of execution bytes per second.") \
    M(SettingUInt64, max_execution_speed_bytes, 0, "Maximum number of execution bytes per second.") \
    M(SettingSeconds, timeout_before_checking_execution_speed, 0, "Check that the speed is not too low after the specified time has elapsed.") \
    \
    M(SettingUInt64, max_columns_to_read, 0, "") \
    M(SettingUInt64, max_temporary_columns, 0, "") \
    M(SettingUInt64, max_temporary_non_const_columns, 0, "") \
    \
    M(SettingUInt64, max_subquery_depth, 100, "") \
    M(SettingUInt64, max_pipeline_depth, 1000, "") \
    M(SettingUInt64, max_ast_depth, 1000, "Maximum depth of query syntax tree. Checked after parsing.") \
    M(SettingUInt64, max_ast_elements, 50000, "Maximum size of query syntax tree in number of nodes. Checked after parsing.") \
    M(SettingUInt64, max_expanded_ast_elements, 500000, "Maximum size of query syntax tree in number of nodes after expansion of aliases and the asterisk.") \
    \
    M(SettingUInt64, readonly, 0, "0 - everything is allowed. 1 - only read requests. 2 - only read requests, as well as changing settings, except for the 'readonly' setting.") \
    \
    M(SettingUInt64, max_rows_in_set, 0, "Maximum size of the set (in number of elements) resulting from the execution of the IN section.") \
    M(SettingUInt64, max_bytes_in_set, 0, "Maximum size of the set (in bytes in memory) resulting from the execution of the IN section.") \
    M(SettingOverflowMode, set_overflow_mode, OverflowMode::THROW, "What to do when the limit is exceeded.") \
    \
    M(SettingUInt64, max_rows_in_join, 0, "Maximum size of the hash table for JOIN (in number of rows).") \
    M(SettingUInt64, max_bytes_in_join, 0, "Maximum size of the hash table for JOIN (in number of bytes in memory).") \
    M(SettingOverflowMode, join_overflow_mode, OverflowMode::THROW, "What to do when the limit is exceeded.") \
    M(SettingBool, join_any_take_last_row, false, "When disabled (default) ANY JOIN will take the first found row for a key. When enabled, it will take the last row seen if there are multiple rows for the same key.") \
    M(SettingBool, partial_merge_join, false, "Use partial merge join instead of hash join for LEFT and INNER JOINs.") \
    M(SettingBool, partial_merge_join_optimisations, false, "Enable optimisations in partial merge join") \
    M(SettingUInt64, partial_merge_join_rows_in_right_blocks, 10000, "Split right-hand joining data in blocks of specified size.") \
    \
    M(SettingUInt64, max_rows_to_transfer, 0, "Maximum size (in rows) of the transmitted external table obtained when the GLOBAL IN/JOIN section is executed.") \
    M(SettingUInt64, max_bytes_to_transfer, 0, "Maximum size (in uncompressed bytes) of the transmitted external table obtained when the GLOBAL IN/JOIN section is executed.") \
    M(SettingOverflowMode, transfer_overflow_mode, OverflowMode::THROW, "What to do when the limit is exceeded.") \
    \
    M(SettingUInt64, max_rows_in_distinct, 0, "Maximum number of elements during execution of DISTINCT.") \
    M(SettingUInt64, max_bytes_in_distinct, 0, "Maximum total size of state (in uncompressed bytes) in memory for the execution of DISTINCT.") \
    M(SettingOverflowMode, distinct_overflow_mode, OverflowMode::THROW, "What to do when the limit is exceeded.") \
    \
    M(SettingUInt64, max_memory_usage, 0, "Maximum memory usage for processing of single query. Zero means unlimited.") \
    M(SettingUInt64, max_memory_usage_for_user, 0, "Maximum memory usage for processing all concurrently running queries for the user. Zero means unlimited.") \
    M(SettingUInt64, max_memory_usage_for_all_queries, 0, "Maximum memory usage for processing all concurrently running queries on the server. Zero means unlimited.") \
    \
    M(SettingUInt64, max_network_bandwidth, 0, "The maximum speed of data exchange over the network in bytes per second for a query. Zero means unlimited.") \
    M(SettingUInt64, max_network_bytes, 0, "The maximum number of bytes (compressed) to receive or transmit over the network for execution of the query.") \
    M(SettingUInt64, max_network_bandwidth_for_user, 0, "The maximum speed of data exchange over the network in bytes per second for all concurrently running user queries. Zero means unlimited.")\
    M(SettingUInt64, max_network_bandwidth_for_all_users, 0, "The maximum speed of data exchange over the network in bytes per second for all concurrently running queries. Zero means unlimited.") \
    M(SettingChar, format_csv_delimiter, ',', "The character to be considered as a delimiter in CSV data. If setting with a string, a string has to have a length of 1.") \
    M(SettingBool, format_csv_allow_single_quotes, 1, "If it is set to true, allow strings in single quotes.") \
    M(SettingBool, format_csv_allow_double_quotes, 1, "If it is set to true, allow strings in double quotes.") \
    M(SettingBool, input_format_csv_unquoted_null_literal_as_null, false, "Consider unquoted NULL literal as \\N") \
    \
    M(SettingDateTimeInputFormat, date_time_input_format, FormatSettings::DateTimeInputFormat::Basic, "Method to read DateTime from text input formats. Possible values: 'basic' and 'best_effort'.") \
    M(SettingBool, log_profile_events, true, "Log query performance statistics into the query_log and query_thread_log.") \
    M(SettingBool, log_query_settings, true, "Log query settings into the query_log.") \
    M(SettingBool, log_query_threads, true, "Log query threads into system.query_thread_log table. This setting have effect only when 'log_queries' is true.") \
    M(SettingLogsLevel, send_logs_level, LogsLevel::none, "Send server text logs with specified minimum level to client. Valid values: 'trace', 'debug', 'information', 'warning', 'error', 'none'") \
    M(SettingBool, enable_optimize_predicate_expression, 1, "If it is set to true, optimize predicates to subqueries.") \
    M(SettingBool, enable_optimize_predicate_expression_to_final_subquery, 1, "Allow push predicate to final subquery.") \
    \
    M(SettingUInt64, low_cardinality_max_dictionary_size, 8192, "Maximum size (in rows) of shared global dictionary for LowCardinality type.") \
    M(SettingBool, low_cardinality_use_single_dictionary_for_part, false, "LowCardinality type serialization setting. If is true, than will use additional keys when global dictionary overflows. Otherwise, will create several shared dictionaries.") \
    M(SettingBool, decimal_check_overflow, true, "Check overflow of decimal arithmetic/comparison operations") \
    \
    M(SettingBool, prefer_localhost_replica, 1, "1 - always send query to local replica, if it exists. 0 - choose replica to send query between local and remote ones according to load_balancing") \
    M(SettingUInt64, max_fetch_partition_retries_count, 5, "Amount of retries while fetching partition from another host.") \
    M(SettingBool, asterisk_left_columns_only, 0, "If it is set to true, the asterisk only return left of join query.") \
    M(SettingUInt64, http_max_multipart_form_data_size, 1024 * 1024 * 1024, "Limit on size of multipart/form-data content. This setting cannot be parsed from URL parameters and should be set in user profile. Note that content is parsed and external tables are created in memory before start of query execution. And this is the only limit that has effect on that stage (limits on max memory usage and max execution time have no effect while reading HTTP form data).") \
    M(SettingBool, calculate_text_stack_trace, 1, "Calculate text stack trace in case of exceptions during query execution. This is the default. It requires symbol lookups that may slow down fuzzing tests when huge amount of wrong queries are executed. In normal cases you should not disable this option.") \
    M(SettingBool, allow_ddl, true, "If it is set to true, then a user is allowed to executed DDL queries.") \
    M(SettingBool, parallel_view_processing, false, "Enables pushing to attached views concurrently instead of sequentially.") \
    M(SettingBool, enable_debug_queries, false, "Enables debug queries such as AST.") \
    M(SettingBool, enable_unaligned_array_join, false, "Allow ARRAY JOIN with multiple arrays that have different sizes. When this settings is enabled, arrays will be resized to the longest one.") \
    M(SettingBool, optimize_read_in_order, true, "Enable ORDER BY optimization for reading data in corresponding order in MergeTree tables.") \
    M(SettingBool, low_cardinality_allow_in_native_format, true, "Use LowCardinality type in Native format. Otherwise, convert LowCardinality columns to ordinary for select query, and convert ordinary columns to required LowCardinality for insert query.") \
    M(SettingBool, allow_experimental_multiple_joins_emulation, true, "Emulate multiple joins using subselects") \
    M(SettingBool, allow_experimental_cross_to_join_conversion, true, "Convert CROSS JOIN to INNER JOIN if possible") \
    M(SettingBool, cancel_http_readonly_queries_on_client_close, false, "Cancel HTTP readonly queries when a client closes the connection without waiting for response.") \
    M(SettingBool, external_table_functions_use_nulls, true, "If it is set to true, external table functions will implicitly use Nullable type if needed. Otherwise NULLs will be substituted with default values. Currently supported only for 'mysql' table function.") \
    M(SettingBool, allow_experimental_data_skipping_indices, false, "If it is set to true, data skipping indices can be used in CREATE TABLE/ALTER TABLE queries.") \
    \
    M(SettingBool, experimental_use_processors, false, "Use processors pipeline.") \
    \
    M(SettingBool, allow_hyperscan, true, "Allow functions that use Hyperscan library. Disable to avoid potentially long compilation times and excessive resource usage.") \
    M(SettingBool, allow_simdjson, true, "Allow using simdjson library in 'JSON*' functions if AVX2 instructions are available. If disabled rapidjson will be used.") \
    M(SettingBool, allow_introspection_functions, false, "Allow functions for introspection of ELF and DWARF for query profiling. These functions are slow and may impose security considerations.") \
    \
    M(SettingUInt64, max_partitions_per_insert_block, 100, "Limit maximum number of partitions in single INSERTed block. Zero means unlimited. Throw exception if the block contains too many partitions. This setting is a safety threshold, because using large number of partitions is a common misconception.") \
    M(SettingBool, check_query_single_value_result, true, "Return check query result as single 1/0 value") \
    M(SettingBool, allow_drop_detached, false, "Allow ALTER TABLE ... DROP DETACHED PART[ITION] ... queries") \
    \
    M(SettingSeconds, distributed_replica_error_half_life, DBMS_CONNECTION_POOL_WITH_FAILOVER_DEFAULT_DECREASE_ERROR_PERIOD, "Time period reduces replica error counter by 2 times.") \
    M(SettingUInt64, distributed_replica_error_cap, DBMS_CONNECTION_POOL_WITH_FAILOVER_MAX_ERROR_COUNT, "Max number of errors per replica, prevents piling up increadible amount of errors if replica was offline for some time and allows it to be reconsidered in a shorter amount of time.") \
    \
    M(SettingBool, allow_experimental_live_view, false, "Enable LIVE VIEW. Not mature enough.") \
    M(SettingSeconds, live_view_heartbeat_interval, DEFAULT_LIVE_VIEW_HEARTBEAT_INTERVAL_SEC, "The heartbeat interval in seconds to indicate live query is alive.") \
    M(SettingSeconds, temporary_live_view_timeout, DEFAULT_TEMPORARY_LIVE_VIEW_TIMEOUT_SEC, "Timeout after which temporary live view is deleted.") \
    M(SettingUInt64, max_live_view_insert_blocks_before_refresh, 64, "Limit maximum number of inserted blocks after which mergeable blocks are dropped and query is re-executed.") \
    M(SettingUInt64, min_free_disk_space_for_temporary_data, 0, "The minimum disk space to keep while writing temporary data used in external sorting and aggregation.") \
    \
    /** Obsolete settings that do nothing but left for compatibility reasons. Remove each one after half a year of obsolescence. */ \
    \
    M(SettingBool, allow_experimental_low_cardinality_type, true, "Obsolete setting, does nothing. Will be removed after 2019-08-13") \
    M(SettingBool, compile, false, "Whether query compilation is enabled. Will be removed after 2020-03-13") \

    DECLARE_SETTINGS_COLLECTION(LIST_OF_SETTINGS)

    /** Set multiple settings from "profile" (in server configuration file (users.xml), profiles contain groups of multiple settings).
     * The profile can also be set using the `set` functions, like the profile setting.
     */
    void setProfile(const String & profile_name, const Poco::Util::AbstractConfiguration & config);

    /// Load settings from configuration file, at "path" prefix in configuration.
    void loadSettingsFromConfig(const String & path, const Poco::Util::AbstractConfiguration & config);

    /// Dumps profile events to two columns of type Array(String)
    void dumpToArrayColumns(IColumn * column_names, IColumn * column_values, bool changed_only = true);

    /// Adds program options to set the settings from a command line.
    /// (Don't forget to call notify() on the `variables_map` after parsing it!)
    void addProgramOptions(boost::program_options::options_description & options);
};

}
