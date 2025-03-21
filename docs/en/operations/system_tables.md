# System tables

System tables are used for implementing part of the system's functionality, and for providing access to information about how the system is working.
You can't delete a system table (but you can perform DETACH).
System tables don't have files with data on the disk or files with metadata. The server creates all the system tables when it starts.
System tables are read-only.
They are located in the 'system' database.

## system.asynchronous_metrics {#system_tables-asynchronous_metrics}

Contains metrics that are calculated periodically in the background. For example, the amount of RAM in use.

Columns:

- `metric` ([String](../data_types/string.md)) — Metric name.
- `value` ([Float64](../data_types/float.md)) — Metric value.

**Example**

```sql
SELECT * FROM system.asynchronous_metrics LIMIT 10
```

```text
┌─metric──────────────────────────────────┬──────value─┐
│ jemalloc.background_thread.run_interval │          0 │
│ jemalloc.background_thread.num_runs     │          0 │
│ jemalloc.background_thread.num_threads  │          0 │
│ jemalloc.retained                       │  422551552 │
│ jemalloc.mapped                         │ 1682989056 │
│ jemalloc.resident                       │ 1656446976 │
│ jemalloc.metadata_thp                   │          0 │
│ jemalloc.metadata                       │   10226856 │
│ UncompressedCacheCells                  │          0 │
│ MarkCacheFiles                          │          0 │
└─────────────────────────────────────────┴────────────┘
```

**See Also**

- [Monitoring](monitoring.md) — Base concepts of ClickHouse monitoring.
- [system.metrics](#system_tables-metrics) — Contains instantly calculated metrics.
- [system.events](#system_tables-events) — Contains a number of events that have occurred.

## system.clusters

Contains information about clusters available in the config file and the servers in them.

Columns:

- `cluster` (String) — The cluster name.
- `shard_num` (UInt32) — The shard number in the cluster, starting from 1.
- `shard_weight` (UInt32) — The relative weight of the shard when writing data.
- `replica_num` (UInt32) — The replica number in the shard, starting from 1.
- `host_name` (String) — The host name, as specified in the config.
- `host_address` (String) — The host IP address obtained from DNS.
- `port` (UInt16) — The port to use for connecting to the server.
- `user` (String) — The name of the user for connecting to the server.
- `errors_count` (UInt32) - number of times this host failed to reach replica.
- `estimated_recovery_time` (UInt32) - seconds left until replica error count is zeroed and it is considered to be back to normal.


Please note that `errors_count` is updated once per query to the cluster, but `estimated_recovery_time` is recalculated on-demand. So there could be a case of non-zero `errors_count` and zero `estimated_recovery_time`, that next query will zero `errors_count` and try to use replica as if it has no errors.

**See also**

- [Table engine Distributed](table_engines/distributed.md)
- [distributed_replica_error_cap setting](settings/settings.md#settings-distributed_replica_error_cap)
- [distributed_replica_error_half_life setting](settings/settings.md#settings-distributed_replica_error_half_life)

## system.columns

Contains information about columns in all the tables.

You can use this table to get information similar to the [DESCRIBE TABLE](../query_language/misc.md#misc-describe-table) query, but for multiple tables at once.

The `system.columns` table contains the following columns (the column type is shown in brackets):

- `database` (String) — Database name.
- `table` (String) — Table name.
- `name` (String) — Column name.
- `type` (String) — Column type.
- `default_kind` (String) — Expression type (`DEFAULT`, `MATERIALIZED`, `ALIAS`) for the default value, or an empty string if it is not defined.
- `default_expression` (String) — Expression for the default value, or an empty string if it is not defined.
- `data_compressed_bytes` (UInt64) — The size of compressed data, in bytes.
- `data_uncompressed_bytes` (UInt64) — The size of decompressed data, in bytes.
- `marks_bytes` (UInt64) — The size of marks, in bytes.
- `comment` (String) — Comment on the column, or an empty string if it is not defined.
- `is_in_partition_key` (UInt8) — Flag that indicates whether the column is in the partition expression.
- `is_in_sorting_key` (UInt8) — Flag that indicates whether the column is in the sorting key expression.
- `is_in_primary_key` (UInt8) — Flag that indicates whether the column is in the primary key expression.
- `is_in_sampling_key` (UInt8) — Flag that indicates whether the column is in the sampling key expression.

## system.contributors {#system_contributors}

Contains information about contributors. All constributors in random order. The order is random at query execution time.

Columns:

- `name` (String) — Contributor (author) name from git log.

**Example**

```sql
SELECT * FROM system.contributors LIMIT 10
```

```text
┌─name─────────────┐
│ Olga Khvostikova │
│ Max Vetrov       │
│ LiuYangkuan      │
│ svladykin        │
│ zamulla          │
│ Šimon Podlipský  │
│ BayoNet          │
│ Ilya Khomutov    │
│ Amy Krishnevsky  │
│ Loud_Scream      │
└──────────────────┘
```

To find out yourself in the table, use a query:

```sql
SELECT * FROM system.contributors WHERE name='Olga Khvostikova'
```
```text
┌─name─────────────┐
│ Olga Khvostikova │
└──────────────────┘
```

## system.databases

This table contains a single String column called 'name' – the name of a database.
Each database that the server knows about has a corresponding entry in the table.
This system table is used for implementing the `SHOW DATABASES` query.

## system.detached_parts {#system_tables-detached_parts}

Contains information about detached parts of [MergeTree](table_engines/mergetree.md) tables. The `reason` column specifies why the part was detached. For user-detached parts, the reason is empty. Such parts can be attached with [ALTER TABLE ATTACH PARTITION|PART](../query_language/query_language/alter/#alter_attach-partition) command. For the description of other columns, see [system.parts](#system_tables-parts). If part name is invalid, values of some columns may be `NULL`. Such parts can be deleted with [ALTER TABLE DROP DETACHED PART](../query_language/query_language/alter/#alter_drop-detached).

## system.dictionaries

Contains information about external dictionaries.

Columns:

- `name` (String) — Dictionary name.
- `type` (String) — Dictionary type: Flat, Hashed, Cache.
- `origin` (String) — Path to the configuration file that describes the dictionary.
- `attribute.names` (Array(String)) — Array of attribute names provided by the dictionary.
- `attribute.types` (Array(String)) — Corresponding array of attribute types that are provided by the dictionary.
- `has_hierarchy` (UInt8) — Whether the dictionary is hierarchical.
- `bytes_allocated` (UInt64) — The amount of RAM the dictionary uses.
- `hit_rate` (Float64) — For cache dictionaries, the percentage of uses for which the value was in the cache.
- `element_count` (UInt64) — The number of items stored in the dictionary.
- `load_factor` (Float64) — The percentage filled in the dictionary (for a hashed dictionary, the percentage filled in the hash table).
- `creation_time` (DateTime) — The time when the dictionary was created or last successfully reloaded.
- `last_exception` (String) — Text of the error that occurs when creating or reloading the dictionary if the dictionary couldn't be created.
- `source` (String) — Text describing the data source for the dictionary.

Note that the amount of memory used by the dictionary is not proportional to the number of items stored in it. So for flat and cached dictionaries, all the memory cells are pre-assigned, regardless of how full the dictionary actually is.

## system.events {#system_tables-events}

Contains information about the number of events that have occurred in the system. For example, in the table, you can find how many `SELECT` queries were processed since the ClickHouse server started.

Columns:

- `event` ([String](../data_types/string.md)) — Event name.
- `value` ([UInt64](../data_types/int_uint.md)) — Number of events occurred.
- `description` ([String](../data_types/string.md)) — Event description.

**Example**

```sql
SELECT * FROM system.events LIMIT 5
```

```text
┌─event─────────────────────────────────┬─value─┬─description────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ Query                                 │    12 │ Number of queries to be interpreted and potentially executed. Does not include queries that failed to parse or were rejected due to AST size limits, quota limits or limits on the number of simultaneously running queries. May include internal queries initiated by ClickHouse itself. Does not count subqueries.                  │
│ SelectQuery                           │     8 │ Same as Query, but only for SELECT queries.                                                                                                                                                                                                                │
│ FileOpen                              │    73 │ Number of files opened.                                                                                                                                                                                                                                    │
│ ReadBufferFromFileDescriptorRead      │   155 │ Number of reads (read/pread) from a file descriptor. Does not include sockets.                                                                                                                                                                             │
│ ReadBufferFromFileDescriptorReadBytes │  9931 │ Number of bytes read from file descriptors. If the file is compressed, this will show the compressed data size.                                                                                                                                              │
└───────────────────────────────────────┴───────┴────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

**See Also**

- [system.asynchronous_metrics](#system_tables-asynchronous_metrics) — Contains periodically calculated metrics.
- [system.metrics](#system_tables-metrics) — Contains instantly calculated metrics.
- [Monitoring](monitoring.md) — Base concepts of ClickHouse monitoring.

## system.functions

Contains information about normal and aggregate functions.

Columns:

- `name`(`String`) – The name of the function.
- `is_aggregate`(`UInt8`) — Whether the function is aggregate.

## system.graphite_retentions

Contains information about parameters [graphite_rollup](server_settings/settings.md#server_settings-graphite_rollup) which are used in tables with [*GraphiteMergeTree](table_engines/graphitemergetree.md) engines.

Columns:

- `config_name`     (String) - `graphite_rollup` parameter name.
- `regexp`          (String) - A pattern for the metric name.
- `function`        (String) - The name of the aggregating function.
- `age`             (UInt64) - The minimum age of the data in seconds.
- `precision`       (UInt64) - How precisely to define the age of the data in seconds.
- `priority`        (UInt16) - Pattern priority.
- `is_default`      (UInt8) - Whether the pattern is the default.
- `Tables.database` (Array(String)) - Array of names of database tables that use the `config_name` parameter.
- `Tables.table`    (Array(String)) - Array of table names that use the `config_name` parameter.


## system.merges

Contains information about merges and part mutations currently in process for tables in the MergeTree family.

Columns:

- `database` (String)  — The name of the database the table is in.
- `table` (String) — Table name.
- `elapsed` (Float64) — The time elapsed (in seconds) since the merge started.
- `progress` (Float64) — The percentage of completed work from 0 to 1.
- `num_parts` (UInt64) — The number of pieces to be merged.
- `result_part_name` (String) — The name of the part that will be formed as the result of merging.
- `is_mutation` (UInt8) - 1 if this process is a part mutation.
- `total_size_bytes_compressed` (UInt64) — The total size of the compressed data in the merged chunks.
- `total_size_marks` (UInt64) — The total number of marks in the merged parts.
- `bytes_read_uncompressed` (UInt64) — Number of bytes read, uncompressed.
- `rows_read` (UInt64) — Number of rows read.
- `bytes_written_uncompressed` (UInt64) — Number of bytes written, uncompressed.
- `rows_written` (UInt64) — Number of rows written.

## system.metrics {#system_tables-metrics}

Contains metrics which can be calculated instantly, or have a current value. For example, the number of simultaneously processed queries or the current replica delay. This table is always up to date.

Columns:

- `metric` ([String](../data_types/string.md)) — Metric name.
- `value` ([Int64](../data_types/int_uint.md)) — Metric value.
- `description` ([String](../data_types/string.md)) — Metric description.

**Example**

```sql
SELECT * FROM system.metrics LIMIT 10
```

```text
┌─metric─────────────────────┬─value─┬─description──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ Query                      │     1 │ Number of executing queries                                                                                                                                                                      │
│ Merge                      │     0 │ Number of executing background merges                                                                                                                                                            │
│ PartMutation               │     0 │ Number of mutations (ALTER DELETE/UPDATE)                                                                                                                                                        │
│ ReplicatedFetch            │     0 │ Number of data parts being fetched from replicas                                                                                                                                                │
│ ReplicatedSend             │     0 │ Number of data parts being sent to replicas                                                                                                                                                      │
│ ReplicatedChecks           │     0 │ Number of data parts checking for consistency                                                                                                                                                    │
│ BackgroundPoolTask         │     0 │ Number of active tasks in BackgroundProcessingPool (merges, mutations, fetches, or replication queue bookkeeping)                                                                                │
│ BackgroundSchedulePoolTask │     0 │ Number of active tasks in BackgroundSchedulePool. This pool is used for periodic ReplicatedMergeTree tasks, like cleaning old data parts, altering data parts, replica re-initialization, etc.   │
│ DiskSpaceReservedForMerge  │     0 │ Disk space reserved for currently running background merges. It is slightly more than the total size of currently merging parts.                                                                     │
│ DistributedSend            │     0 │ Number of connections to remote servers sending data that was INSERTed into Distributed tables. Both synchronous and asynchronous mode.                                                          │
└────────────────────────────┴───────┴──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```
**See Also**

- [system.asynchronous_metrics](#system_tables-asynchronous_metrics) — Contains periodically calculated metrics.
- [system.events](#system_tables-events) — Contains a number of events that occurred.
- [Monitoring](monitoring.md) — Base concepts of ClickHouse monitoring.

## system.numbers

This table contains a single UInt64 column named 'number' that contains almost all the natural numbers starting from zero.
You can use this table for tests, or if you need to do a brute force search.
Reads from this table are not parallelized.

## system.numbers_mt

The same as 'system.numbers' but reads are parallelized. The numbers can be returned in any order.
Used for tests.

## system.one

This table contains a single row with a single 'dummy' UInt8 column containing the value 0.
This table is used if a SELECT query doesn't specify the FROM clause.
This is similar to the DUAL table found in other DBMSs.

## system.parts {#system_tables-parts}

Contains information about parts of [MergeTree](table_engines/mergetree.md) tables.

Each row describes one data part.

Columns:

- `partition` (String) – The partition name. To learn what a partition is, see the description of the [ALTER](../query_language/alter.md#query_language_queries_alter) query.

    Formats:

    - `YYYYMM` for automatic partitioning by month.
    - `any_string` when partitioning manually.

- `name` (String) – Name of the data part.
- `active` (UInt8) – Flag that indicates whether the part is active. If a part is active, it is used in a table; otherwise, it will be deleted. Inactive data parts remain after merging.
- `marks` (UInt64) – The number of marks. To get the approximate number of rows in a data part, multiply `marks` by the index granularity (usually 8192) (this hint doesn't work for adaptive granularity).
- `rows` (UInt64) – The number of rows.
- `bytes_on_disk` (UInt64) – Total size of all the data part files in bytes.
- `data_compressed_bytes` (UInt64) – Total size of compressed data in the data part. All the auxiliary files (for example, files with marks) are not included.
- `data_uncompressed_bytes` (UInt64) – Total size of uncompressed data in the data part. All the auxiliary files (for example, files with marks) are not included.
- `marks_bytes` (UInt64) – The size of the file with marks.
- `modification_time` (DateTime) – The modification time of the directory with the data part. This usually corresponds to the time of data part creation.|
- `remove_time` (DateTime) – The time when the data part became inactive.
- `refcount` (UInt32) – The number of places where the data part is used. A value greater than 2 indicates that the data part is used in queries or merges.
- `min_date` (Date) – The minimum value of the date key in the data part.
- `max_date` (Date) – The maximum value of the date key in the data part.
- `min_time` (DateTime) – The minimum value of the date and time key in the data part.
- `max_time`(`DateTime`) – The maximum value of the date and time key in the data part.
- `partition_id` (String) – Id of the partition.
- `min_block_number` (UInt64) – The minimum number of data parts that make up the current part after merging.
- `max_block_number` (UInt64) – The maximum number of data parts that make up the current part after merging.
- `level` (UInt32) – Depth of the merge tree. Zero means that current part was created by insert rather than by merging other parts.
- `data_version` (UInt64) – Number that is used to determine which mutations should be applied to the data part (the mutations with the higher version than `data_version`).
- `primary_key_bytes_in_memory` (UInt64) – The amount of memory (in bytes) used by primary key values.
- `primary_key_bytes_in_memory_allocated` (UInt64) – The amount of memory (in bytes) reserved for primary key values.
- `is_frozen` (UInt8) – Flag that shows partition data backup existence. 1, the backup exists. 0, the backup doesn't exist. For more details, see [FREEZE PARTITION](../query_language/alter.md#alter_freeze-partition)
- `database` (String) – Name of the database.
- `table` (String) – Name of the table.
- `engine` (String) – Name of the table engine without parameters.
- `path` (String) – Absolute path to the folder with data part files.
- `hash_of_all_files` (String) – [sipHash128](../query_language/functions/hash_functions.md#hash_functions-siphash128) of compressed files.
- `hash_of_uncompressed_files` (String) – [sipHash128](../query_language/functions/hash_functions.md#hash_functions-siphash128) of uncompressed data.
- `uncompressed_hash_of_compressed_files` (String) – [sipHash128](../query_language/functions/hash_functions.md#hash_functions-siphash128) of the file with marks.
- `bytes` (UInt64) – Alias for `bytes_on_disk`.
- `marks_size` (UInt64) – Alias for `marks_bytes`.


## system.part_log {#system_tables-part-log}

The `system.part_log` table is created only if the [part_log](server_settings/settings.md#server_settings-part-log) server setting is specified.

This table contains information about the events that occurred with the [data parts](table_engines/custom_partitioning_key.md) in the [MergeTree](table_engines/mergetree.md) family tables. For instance, adding or merging data.

The `system.part_log` table contains the following columns:

- `event_type` (Enum) — Type of the event that occurred with the data part. Can have one of the following values:
    - `NEW_PART` — inserting
    - `MERGE_PARTS` — merging
    - `DOWNLOAD_PART` — downloading
    - `REMOVE_PART` — removing or detaching using [DETACH PARTITION](../query_language/alter.md#alter_detach-partition)
    - `MUTATE_PART` — updating.
- `event_date` (Date) — Event date.
- `event_time` (DateTime) — Event time.
- `duration_ms` (UInt64) — Duration.
- `database` (String) — Name of the database the data part is in.
- `table` (String) — Name of the table the data part is in.
- `part_name` (String) — Name of the data part.
- `partition_id` (String) — ID of the partition that the data part was inserted to. The column takes the 'all' value if the partitioning is by `tuple()`.
- `rows` (UInt64) — The number of rows in the data part.
- `size_in_bytes` (UInt64) — Size of the data part in bytes.
- `merged_from` (Array(String)) — An array of names of the parts which the current part was made up from (after the merge).
- `bytes_uncompressed` (UInt64) — Size of uncompressed bytes.
- `read_rows` (UInt64) — The number of rows was read during the merge.
- `read_bytes` (UInt64) — The number of bytes was read during the merge.
- `error` (UInt16) — The code number of the occurred error.
- `exception` (String) — Text message of the occurred error.

The `system.part_log` table is created after the first inserting data to the `MergeTree` table.

## system.processes

This system table is used for implementing the `SHOW PROCESSLIST` query.
Columns:

- `user` (String)              – Name of the user who made the request. For distributed query processing, this is the user who helped the requestor server send the query to this server, not the user who made the distributed request on the requestor server.
- `address` (String)           - The IP address the request was made from. The same for distributed processing.
- `elapsed` (Float64)          - The time in seconds since request execution started.
- `rows_read` (UInt64)         - The number of rows read from the table. For distributed processing, on the requestor server, this is the total for all remote servers.
- `bytes_read` (UInt64)        - The number of uncompressed bytes read from the table. For distributed processing, on the requestor server, this is the total for all remote servers.
- `total_rows_approx` (UInt64) - The approximation of the total number of rows that should be read. For distributed processing, on the requestor server, this is the total for all remote servers. It can be updated during request processing, when new sources to process become known.
- `memory_usage` (UInt64)      - How much memory the request uses. It might not include some types of dedicated memory.
- `query` (String)             - The query text. For INSERT, it doesn't include the data to insert.
- `query_id` (String)          - Query ID, if defined.

## system.query_log {#system_tables-query-log}

Contains information about execution of queries. For each query, you can see processing start time, duration of processing, error messages and other information.

!!! note
    The table doesn't contain input data for `INSERT` queries.

ClickHouse creates this table only if the [query_log](server_settings/settings.md#server_settings-query-log) server parameter is specified. This parameter sets the logging rules, such as the logging interval or the name of the table the queries will be logged in.

To enable query logging, set the [log_queries](settings/settings.md#settings-log-queries) parameter to 1. For details, see the [Settings](settings/settings.md) section.

The `system.query_log` table registers two kinds of queries:

1. Initial queries that were run directly by the client.
2. Child queries that were initiated by other queries (for distributed query execution). For these types of queries, information about the parent queries is shown in the `initial_*` columns.

Columns:

- `type` (UInt8) — Type of event that occurred when executing the query. Possible values:
    - 1 — Successful start of query execution.
    - 2 — Successful end of query execution.
    - 3 — Exception before the start of query execution.
    - 4 — Exception during the query execution.
- `event_date` (Date) — Event date.
- `event_time` (DateTime) — Event time.
- `query_start_time` (DateTime) — Start time of query execution.
- `query_duration_ms` (UInt64) — Duration of query execution.
- `read_rows` (UInt64) — Number of read rows.
- `read_bytes` (UInt64) — Number of read bytes.
- `written_rows` (UInt64) — For `INSERT` queries, the number of written rows. For other queries, the column value is 0.
- `written_bytes` (UInt64) — For `INSERT` queries, the number of written bytes. For other queries, the column value is 0.
- `result_rows` (UInt64) — Number of rows in the result.
- `result_bytes` (UInt64) — Number of bytes in the result.
- `memory_usage` (UInt64) — Memory consumption by the query.
- `query` (String) — Query string.
- `exception` (String) — Exception message.
- `stack_trace` (String) — Stack trace (a list of methods called before the error occurred). An empty string, if the query is completed successfully.
- `is_initial_query` (UInt8) — Kind of query. Possible values:
    - 1 — Query was initiated by the client.
    - 0 — Query was initiated by another query for distributed query execution.
- `user` (String) — Name of the user who initiated the current query.
- `query_id` (String) — ID of the query.
- `address` (FixedString(16)) — IP address the query was initiated from.
- `port` (UInt16) — The server port that was used to receive the query.
- `initial_user` (String) —  Name of the user who ran the parent query (for distributed query execution).
- `initial_query_id` (String) — ID of the parent query.
- `initial_address` (FixedString(16)) — IP address that the parent query was launched from.
- `initial_port` (UInt16) — The server port that was used to receive the parent query from the client.
- `interface` (UInt8) — Interface that the query was initiated from. Possible values:
    - 1 — TCP.
    - 2 — HTTP.
- `os_user` (String) — User's OS.
- `client_hostname` (String) — Server name that the [clickhouse-client](../interfaces/cli.md) is connected to.
- `client_name` (String) — The [clickhouse-client](../interfaces/cli.md) name.
- `client_revision` (UInt32) — Revision of the [clickhouse-client](../interfaces/cli.md).
- `client_version_major` (UInt32) — Major version of the [clickhouse-client](../interfaces/cli.md).
- `client_version_minor` (UInt32) — Minor version of the [clickhouse-client](../interfaces/cli.md).
- `client_version_patch` (UInt32) — Patch component of the [clickhouse-client](../interfaces/cli.md) version.
- `http_method` (UInt8) — HTTP method that initiated the query. Possible values:
    - 0 — The query was launched from the TCP interface.
    - 1 — `GET` method was used.
    - 2 — `POST` method was used.
- `http_user_agent` (String) — The `UserAgent` header passed in the HTTP request.
- `quota_key` (String) — The quota key specified in the [quotas](quotas.md) setting.
- `revision` (UInt32) — ClickHouse revision.
- `thread_numbers` (Array(UInt32)) — Number of threads that are participating in query execution.
- `ProfileEvents.Names` (Array(String)) — Counters that measure the following metrics:
    - Time spent on reading and writing over the network.
    - Time spent on reading and writing to a disk.
    - Number of network errors.
    - Time spent on waiting when the network bandwidth is limited.
- `ProfileEvents.Values` (Array(UInt64)) — Values of metrics that are listed in the&#160;`ProfileEvents.Names` column.
- `Settings.Names` (Array(String)) — Names of settings that were changed when the client ran the query. To enable logging changes to settings, set the `log_query_settings` parameter to 1.
- `Settings.Values` (Array(String)) — Values of settings that are listed in the `Settings.Names` column.

Each query creates one or two rows in the `query_log` table, depending on the status of the query:

1. If the query execution is successful, two events with types 1 and 2 are created (see the `type` column).
2. If an error occurred during query processing, two events with types 1 and 4 are created.
3. If an error occurred before launching the query, a single event with type 3 is created.

By default, logs are added to the table at intervals of 7.5 seconds. You can set this interval in the [query_log](server_settings/settings.md#server_settings-query-log) server setting (see the `flush_interval_milliseconds` parameter). To flush the logs forcibly from the memory buffer into the table, use the `SYSTEM FLUSH LOGS` query.

When the table is deleted manually, it will be automatically created on the fly. Note that all the previous logs will be deleted.

!!! note
    The storage period for logs is unlimited. Logs aren't automatically deleted from the table. You need to organize the removal of outdated logs yourself.

You can specify an arbitrary partitioning key for the `system.query_log` table in the [query_log](server_settings/settings.md#server_settings-query-log) server setting (see the `partition_by` parameter).

## system.replicas {#system_tables-replicas}

Contains information and status for replicated tables residing on the local server.
This table can be used for monitoring. The table contains a row for every Replicated\* table.

Example:

``` sql
SELECT *
FROM system.replicas
WHERE table = 'visits'
FORMAT Vertical
```

```text
Row 1:
──────
database:           merge
table:              visits
engine:             ReplicatedCollapsingMergeTree
is_leader:          1
is_readonly:        0
is_session_expired: 0
future_parts:       1
parts_to_check:     0
zookeeper_path:     /clickhouse/tables/01-06/visits
replica_name:       example01-06-1.yandex.ru
replica_path:       /clickhouse/tables/01-06/visits/replicas/example01-06-1.yandex.ru
columns_version:    9
queue_size:         1
inserts_in_queue:   0
merges_in_queue:    1
log_max_index:      596273
log_pointer:        596274
total_replicas:     2
active_replicas:    2
```

Columns:

```text
database:          Database name
table:              Table name
engine:            Table engine name

is_leader:          Whether the replica is the leader.

Only one replica at a time can be the leader. The leader is responsible for selecting background merges to perform.
Note that writes can be performed to any replica that is available and has a session in ZK, regardless of whether it is a leader.

is_readonly:        Whether the replica is in read-only mode.
This mode is turned on if the config doesn't have sections with ZooKeeper, if an unknown error occurred when reinitializing sessions in ZooKeeper, and during session reinitialization in ZooKeeper.

is_session_expired: Whether the session with ZooKeeper has expired.
Basically the same as 'is_readonly'.

future_parts:       The number of data parts that will appear as the result of INSERTs or merges that haven't been done yet.

parts_to_check:    The number of data parts in the queue for verification.
A part is put in the verification queue if there is suspicion that it might be damaged.

zookeeper_path:     Path to table data in ZooKeeper.
replica_name:       Replica name in ZooKeeper. Different replicas of the same table have different names.
replica_path:      Path to replica data in ZooKeeper. The same as concatenating 'zookeeper_path/replicas/replica_path'.

columns_version:    Version number of the table structure.
Indicates how many times ALTER was performed. If replicas have different versions, it means some replicas haven't made all of the ALTERs yet.

queue_size:         Size of the queue for operations waiting to be performed.
Operations include inserting blocks of data, merges, and certain other actions.
It usually coincides with 'future_parts'.

inserts_in_queue:   Number of inserts of blocks of data that need to be made.
Insertions are usually replicated fairly quickly. If this number is large, it means something is wrong.

merges_in_queue:    The number of merges waiting to be made.
Sometimes merges are lengthy, so this value may be greater than zero for a long time.

The next 4 columns have a non-zero value only where there is an active session with ZK.

log_max_index:      Maximum entry number in the log of general activity.
log_pointer:        Maximum entry number in the log of general activity that the replica copied to its execution queue, plus one.
If log_pointer is much smaller than log_max_index, something is wrong.

total_replicas:     The total number of known replicas of this table.
active_replicas:    The number of replicas of this table that have a session in ZooKeeper (i.e., the number of functioning replicas).
```

If you request all the columns, the table may work a bit slowly, since several reads from ZooKeeper are made for each row.
If you don't request the last 4 columns (log_max_index, log_pointer, total_replicas, active_replicas), the table works quickly.

For example, you can check that everything is working correctly like this:

```sql
SELECT
    database,
    table,
    is_leader,
    is_readonly,
    is_session_expired,
    future_parts,
    parts_to_check,
    columns_version,
    queue_size,
    inserts_in_queue,
    merges_in_queue,
    log_max_index,
    log_pointer,
    total_replicas,
    active_replicas
FROM system.replicas
WHERE
       is_readonly
    OR is_session_expired
    OR future_parts > 20
    OR parts_to_check > 10
    OR queue_size > 20
    OR inserts_in_queue > 10
    OR log_max_index - log_pointer > 10
    OR total_replicas < 2
    OR active_replicas < total_replicas
```

If this query doesn't return anything, it means that everything is fine.

## system.settings

Contains information about settings that are currently in use.
I.e. used for executing the query you are using to read from the system.settings table.

Columns:

- `name` (String) — Setting name.
- `value` (String)  — Setting value.
- `changed` (UInt8) — Whether the setting was explicitly defined in the config or explicitly changed.


Example:

```sql
SELECT *
FROM system.settings
WHERE changed
```

```text
┌─name───────────────────┬─value───────┬─changed─┐
│ max_threads            │ 8           │       1 │
│ use_uncompressed_cache │ 0           │       1 │
│ load_balancing         │ random      │       1 │
│ max_memory_usage       │ 10000000000 │       1 │
└────────────────────────┴─────────────┴─────────┘
```

## system.tables

Contains metadata of each table that the server knows about. Detached tables are not shown in `system.tables`.

This table contains the following columns (the column type is shown in brackets):

- `database` (String) — The name of the database the table is in.
- `name` (String) — Table name.
- `engine` (String) — Table engine name (without parameters).
- `is_temporary` (UInt8) - Flag that indicates whether the table is temporary.
- `data_path` (String) - Path to the table data in the file system.
- `metadata_path` (String) - Path to the table metadata in the file system.
- `metadata_modification_time` (DateTime) - Time of latest modification of the table metadata.
- `dependencies_database` (Array(String)) - Database dependencies.
- `dependencies_table` (Array(String)) - Table dependencies ([MaterializedView](table_engines/materializedview.md) tables based on the current table).
- `create_table_query` (String) - The query that was used to create the table.
- `engine_full` (String) - Parameters of the table engine.
- `partition_key` (String) - The partition key expression specified in the table.
- `sorting_key` (String) - The sorting key expression specified in the table.
- `primary_key` (String) - The primary key expression specified in the table.
- `sampling_key` (String) - The sampling key expression specified in the table.

The `system.tables` table is used in `SHOW TABLES` query implementation.

## system.zookeeper

The table does not exist if ZooKeeper is not configured. Allows reading data from the ZooKeeper cluster defined in the config.
The query must have a 'path' equality condition in the WHERE clause. This is the path in ZooKeeper for the children that you want to get data for.

The query `SELECT * FROM system.zookeeper WHERE path = '/clickhouse'` outputs data for all children on the `/clickhouse` node.
To output data for all root nodes, write path = '/'.
If the path specified in 'path' doesn't exist, an exception will be thrown.

Columns:

- `name` (String) — The name of the node.
- `path` (String) — The path to the node.
- `value` (String) — Node value.
- `dataLength` (Int32) — Size of the value.
- `numChildren` (Int32) — Number of descendants.
- `czxid` (Int64) — ID of the transaction that created the node.
- `mzxid` (Int64) — ID of the transaction that last changed the node.
- `pzxid` (Int64) — ID of the transaction that last deleted or added descendants.
- `ctime` (DateTime) — Time of node creation.
- `mtime` (DateTime) — Time of the last modification of the node.
- `version` (Int32) — Node version: the number of times the node was changed.
- `cversion` (Int32) — Number of added or removed descendants.
- `aversion` (Int32) — Number of changes to the ACL.
- `ephemeralOwner` (Int64) — For ephemeral nodes, the ID of the session that owns this node.

Example:

```sql
SELECT *
FROM system.zookeeper
WHERE path = '/clickhouse/tables/01-08/visits/replicas'
FORMAT Vertical
```

```text
Row 1:
──────
name:           example01-08-1.yandex.ru
value:
czxid:          932998691229
mzxid:          932998691229
ctime:          2015-03-27 16:49:51
mtime:          2015-03-27 16:49:51
version:        0
cversion:       47
aversion:       0
ephemeralOwner: 0
dataLength:     0
numChildren:    7
pzxid:          987021031383
path:           /clickhouse/tables/01-08/visits/replicas

Row 2:
──────
name:           example01-08-2.yandex.ru
value:
czxid:          933002738135
mzxid:          933002738135
ctime:          2015-03-27 16:57:01
mtime:          2015-03-27 16:57:01
version:        0
cversion:       37
aversion:       0
ephemeralOwner: 0
dataLength:     0
numChildren:    7
pzxid:          987021252247
path:           /clickhouse/tables/01-08/visits/replicas
```

## system.mutations {#system_tables-mutations}

The table contains information about [mutations](../query_language/alter.md#alter-mutations) of MergeTree tables and their progress. Each mutation command is represented by a single row. The table has the following columns:

**database**, **table** - The name of the database and table to which the mutation was applied.

**mutation_id** - The ID of the mutation. For replicated tables these IDs correspond to znode names in the `<table_path_in_zookeeper>/mutations/` directory in ZooKeeper. For unreplicated tables the IDs correspond to file names in the data directory of the table.

**command** - The mutation command string (the part of the query after `ALTER TABLE [db.]table`).

**create_time** - When this mutation command was submitted for execution.

**block_numbers.partition_id**, **block_numbers.number** - A nested column. For mutations of replicated tables, it contains one record for each partition: the partition ID and the block number that was acquired by the mutation (in each partition, only parts that contain blocks with numbers less than the block number acquired by the mutation in that partition will be mutated). In non-replicated tables, block numbers in all partitions form a single sequence. This means that for mutations of non-replicated tables, the column will contain one record with a single block number acquired by the mutation.

**parts_to_do** - The number of data parts that need to be mutated for the mutation to finish.

**is_done** - Is the mutation done? Note that even if `parts_to_do = 0` it is possible that a mutation of a replicated table is not done yet because of a long-running INSERT that will create a new data part that will need to be mutated.

If there were problems with mutating some parts, the following columns contain additional information:

**latest_failed_part** - The name of the most recent part that could not be mutated.

**latest_fail_time** - The time of the most recent part mutation failure.

**latest_fail_reason** - The exception message that caused the most recent part mutation failure.

[Original article](https://clickhouse.yandex/docs/en/operations/system_tables/) <!--hide-->
