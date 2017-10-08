#ifndef KIWI_SERVER_H_
#define KIWI_SERVER_H_

#include <cstdint>
#include <pthread.h>
#include <string>
#include "server_config.h"
#include "storage.h"

using namespace std;


class Server {
public:
    Server(ServerConfig const& server_config, Storage& storage);
    ~Server(void);
    void Start(void);
    void Shutdown(void);

    class ClusterNode {
    public:
        ClusterNode(uint32_t id, SocketAddress const& address, int port, Server& server);
        ~ClusterNode(void);
        void Start(void);
        void IncomingConnection(int fd);
        void Shutdown(void);
    
    private:
        uint32_t id;
        SocketAddress address;
        int port;
        Server& server;
    };

private:
    ServerConfig const& server_config;
    int shutdown_pipe[2];
    Storage& storage;
    unordered_map<uint32_t, ClusterNode*> cluster_nodes;
    pthread_t acceptor_thread;
    bool acceptor_thread_created;

    static void * AcceptorThreadWrapper(void * ptr);
    void AcceptorThreadMain(void);
    void IncomingConnection(int fd);
};

#endif  // KIWI_SERVER_H_
