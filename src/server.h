#ifndef KIWI_SERVER_H_
#define KIWI_SERVER_H_

#include <set>
#include "buffered_network_connection.h"
#include "server_config.h"
#include "storage.h"


class Server {
public:
    Server(ServerConfig const& config, Storage& storage);
    static void* ThreadWrapper(void* ptr);
    void ThreadMain(void);
    ~Server(void);

private:
    ServerConfig const& config;
    Storage& storage;
    int kq;
    pthread_t thread;
    std::set<BufferedNetworkConnection*> connections;

    void AddFD(int fd, short filter, void* data);
};

#endif  // KIWI_SERVER_H_
