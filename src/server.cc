#include <iostream>
#include <sstream>
#include "exceptions.h"
#include "server.h"


Server::Server(ServerConfig const& config, Storage& storage) :
        config(config),
        storage(storage),
        cluster_nodes() {

    try {
        auto hosts = config.Hosts();
        for (auto it = hosts.begin(); it != hosts.end(); ++it) {
            auto id = it->first;
            auto host = it->second;
            auto cluster_node = new ClusterNode(*this, id, host, 1);
            try {
                cluster_nodes[id] = cluster_node;
            } catch (...) {
                delete cluster_node;
                throw;
            }

            cluster_node->Start();
        }

    } catch (...) {
        for (auto it = cluster_nodes.begin(); it != cluster_nodes.end(); ++it) {
            auto cluster_node = it->second;
            cluster_node->Shutdown();
            delete cluster_node;
        }
        throw;
    }
}


Server::~Server(void) {
    for (auto it = cluster_nodes.begin(); it != cluster_nodes.end(); ++it) {
        auto cluster_node = it->second;
        cluster_node->Shutdown();
        delete cluster_node;
    }
}


void Server::HandleConnection(int fd, ConnectionState* connection_state) {
}


Server::ClusterNode::ClusterNode(Server& server, uint32_t id, SocketAddress const& address, int port) :
        server(server),
        id(id),
        address(address),
        port(port),
        thread_created(false) {}


void* Server::ClusterNode::ThreadWrapper(void* ptr) {
    ClusterNode* cluster_node = static_cast<ClusterNode*>(ptr);
    try {
        cluster_node->ThreadMain();
    } catch (exception const& e) {
        cerr << "Cluster node thread crashed (id=" << cluster_node->id << "): " << e.what() << endl;
        abort();
    } catch (...) {
        cerr << "Cluster node thread crashed (id=" << cluster_node->id << ")" << endl;
        abort();
    }
    return nullptr;
}


void Server::ClusterNode::Start(void) {
    int err = pthread_create(&thread, nullptr, ThreadWrapper, this);
    if (err == 0) {
        thread_created = true;
    } else {
        stringstream ss;
        ss << "Error creating cluster node thread (id=" << id << "): " << strerror(err);
        throw ServerException(ss.str());
    }
}


void Server::ClusterNode::ThreadMain(void) {
    // dedicated thread for poll({shutdown_fd, notification_fd, connection_fd}) + connecting out to the other node if needed (this->id > server.config.ServerId())
}


void Server::ClusterNode::IncomingConnection(int protocol_version, int fd) {
}


void Server::ClusterNode::SendRequest(int data) {
    // When sending a request, we might need to wake up event loop, but we don't want to spam the notification_fd because it's expensive. Hence, we need to employ some sort of optimistic strategy where we can hopefully avoid writing to notification_fd because we can guarantee that event loop is active and will take care of our request immediately.
}


void Server::ClusterNode::Shutdown(void) {
}


Server::ClusterNode::~ClusterNode(void) {
    if (thread_created) {
        int err = pthread_join(thread, nullptr);
        if (err != 0) {
            cerr << "Fatal: problem joining cluster node thread (id=" << id << "): " << strerror(err) << endl;
            abort();
        }
    }
}
