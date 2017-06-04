#ifndef KIWI_STORAGE_H_
#define KIWI_STORAGE_H_

#include "rocksdb/db.h"
#include "server_config.h"
#include "status.h"

using namespace std;

class Storage {
public:
    Storage(ServerConfig const& server_config);
    Status Open(void);
    ~Storage();

private:
    rocksdb::DB* db;
    string data_dir;
};

#endif  // KIWI_STORAGE_H_
