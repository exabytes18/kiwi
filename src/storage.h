#ifndef KIWI_STORAGE_H_
#define KIWI_STORAGE_H_

#include "rocksdb/db.h"
#include "server_config.h"

using namespace std;

class Storage {
public:
    Storage(ServerConfig const& server_config);
    void Deliver(uint64_t offset); // Raft will call Storage.Deliver() once a quorum have written the values to their log. This is intended to be a very fast call with all the work being done by some internal storage class thread.
    ~Storage(void);

private:
    rocksdb::DB* db;
};

#endif  // KIWI_STORAGE_H_
