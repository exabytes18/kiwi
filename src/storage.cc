#include "exceptions.h"
#include "rocksdb/cache.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/table.h"
#include "storage.h"


Storage::Storage(ServerConfig const& server_config) {
    rocksdb::Options options;
    options.IncreaseParallelism();
    options.max_file_opening_threads = -1;
    options.max_background_jobs = 4;
    options.recycle_log_file_num = 1;
    options.compression = rocksdb::kLZ4Compression;
    options.compaction_readahead_size = 2 * 1024 * 1024;
    options.writable_file_max_buffer_size = 2 * 1024 * 1024;
    options.bytes_per_sync = 1024 * 1024;
    options.target_file_size_base = 256 * 1024 * 1024;
    options.create_if_missing = true;
    options.create_missing_column_families = true;
    options.prefix_extractor.reset(rocksdb::NewCappedPrefixTransform(4));

    rocksdb::BlockBasedTableOptions table_options;
    table_options.block_cache = rocksdb::NewLRUCache(128 * 1024 * 1024);
    table_options.block_size = 16 * 1024;
    table_options.cache_index_and_filter_blocks = true;
    table_options.cache_index_and_filter_blocks_with_high_priority = true;
    table_options.pin_l0_filter_and_index_blocks_in_cache = true;
    table_options.index_type = rocksdb::BlockBasedTableOptions::kTwoLevelIndexSearch;
    table_options.partition_filters = true;
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, true));
    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));

    string const& data_dir = server_config.DataDir();
    rocksdb::Status status = rocksdb::DB::Open(options, data_dir, &db);
    if (!status.ok()) {
        throw StorageException(status.ToString());
    }
}


void Deliver(uint64_t offset) {
}


Storage::~Storage(void) {
    delete db;
}
