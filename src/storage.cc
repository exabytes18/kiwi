#include "storage.h"

Storage::Storage(ServerConfig const& server_config) :
    db(nullptr),
    data_dir(server_config.DataDir()) {}


Status Storage::Open(void) {
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, data_dir, &db);
    if (!status.ok()) {
        return Status::StorageError(status.ToString());
    }

    return Status();
}


Storage::~Storage() {
    if (db != nullptr) {
        delete db;
    }
}
