#include <errno.h>
#include <iostream>
#include <unistd.h>
#include "exceptions.h"
#include "server.h"


Server::Server(ServerConfig const& server_config, Storage& storage) :
        storage(storage),
        cluster_nodes(),
        acceptor_thread_created(false) {

    if (pipe(shutdown_pipe) != 0) {
        throw ServerException("Error creating shutdown pipe: " + string(strerror(errno)));
    }

    try {
        auto hosts = server_config.Hosts();
        for (auto it = hosts.begin(); it != hosts.end(); ++it) {
            auto id = it->first;
            auto host = it->second;
            cluster_nodes[id] = new ClusterNode(id, host, 1, *this);
        }
    } catch (...) {
        close(shutdown_pipe[0]);
        close(shutdown_pipe[1]);
        
        for (auto it = cluster_nodes.begin(); it != cluster_nodes.end(); ++it) {
            auto cluster_node = it->second;
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

    if (acceptor_thread_created) {
        int err = pthread_join(acceptor_thread, NULL);
        if (err != 0) {
            cerr << "Fatal: problem joining acceptor thread: " << strerror(err) << endl;
            abort();
        }
    }

    close(shutdown_pipe[0]);
    close(shutdown_pipe[1]);
}


void* AcceptorThread(void* ptr) {
    Server* server = static_cast<Server*>(ptr);
    return nullptr;
}


void Server::Start(void) {
    for (auto it = cluster_nodes.begin(); it != cluster_nodes.end(); ++it) {
        auto cluster_node = it->second;
        cluster_node->Start();
    }

    int err = pthread_create(&acceptor_thread, NULL, AcceptorThread, this);
    if (err == 0) {
        acceptor_thread_created = true;
    } else {
        throw ServerException("Error creating acceptor thread: " + string(strerror(errno)));
    }
}


void Server::Shutdown(void) {
}


Server::ClusterNode::ClusterNode(uint32_t id, string const& address, int port, Server& server) :
        id(id),
        address(address),
        port(port),
        server(server) {}


void Server::ClusterNode::Start(void) {
}


void Server::ClusterNode::IncomingConnection(int fd) {
}


void Server::ClusterNode::Shutdown(void) {
}


Server::ClusterNode::~ClusterNode(void) {
}
