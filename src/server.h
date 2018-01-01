#ifndef KIWI_SERVER_H_
#define KIWI_SERVER_H_

#include <cstdint>
#include <pthread.h>
#include <string>
#include "connection_state.h"
#include "server_config.h"
#include "storage.h"

using namespace std;


class Server {
public:
    Server(ServerConfig const& config, Storage& storage);
    ~Server(void);
    void HandleConnection(int fd, ConnectionState* connection_state);

    class ClusterNode {
    public:
        ClusterNode(Server& server, uint32_t id, SocketAddress const& address, int port);
        ~ClusterNode(void);
        void Start(void);
        void Shutdown(void);

    private:
        Server& server;
        pthread_t thread;
        uint32_t id;
        SocketAddress address;
        int port;
        bool thread_created;

        static void* ThreadWrapper(void* ptr);
        void ThreadMain(void);
        void IncomingConnection(int protocol_version, int fd);
        void SendRequest(int data);
    };

private:
    ServerConfig const& config;
    Storage& storage;
    unordered_map<uint32_t, ClusterNode*> cluster_nodes;
};

#endif  // KIWI_SERVER_H_
