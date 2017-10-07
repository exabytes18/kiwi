#ifndef KIWI_STORAGE_H_
#define KIWI_STORAGE_H_

#include "rocksdb/db.h"
#include "server_config.h"

using namespace std;

class Storage {
public:
    Storage(ServerConfig const& server_config);
    ~Storage();

private:
    rocksdb::DB* db;
};

#endif  // KIWI_STORAGE_H_
