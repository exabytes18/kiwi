#ifndef KIWI_SERVER_H_
#define KIWI_SERVER_H_

#include <cstdint>
#include <pthread.h>
#include <set>
#include <string>
#include "buffered_network_connection.h"
#include "event_loop.h"
#include "mutex.h"
#include "server_config.h"
#include "storage.h"

using namespace std;


class Server {
public:
    Server(ServerConfig const& config, Storage& storage);
    ~Server(void);
    void HandleIncomingClientConnection(BufferedNetworkConnection* buffered_network_connection);
    void HandleIncomingClusterNodeConnection(BufferedNetworkConnection* buffered_network_connection);

    class ClusterNode {
    public:
        ClusterNode(Server& server, uint32_t id, SocketAddress const& address);
        ~ClusterNode(void);
        void Start(void);
        void Shutdown(void);

    private:
        Server& server;
        pthread_t thread;
        uint32_t id;
        SocketAddress address;
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
