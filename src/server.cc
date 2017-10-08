#include <errno.h>
#include <iostream>
#include <poll.h>
#include <unistd.h>
#include "exceptions.h"
#include "io_utils.h"
#include "server.h"


Server::Server(ServerConfig const& server_config, Storage& storage) :
        server_config(server_config),
        storage(storage),
        cluster_nodes(),
        acceptor_thread_created(false) {

    if (pipe(shutdown_pipe) != 0) {
        throw ServerException("Error creating shutdown pipe: " + string(strerror(errno)));
    }

    try {
        IOUtils::SetNonBlocking(shutdown_pipe[0]);
        IOUtils::SetNonBlocking(shutdown_pipe[1]);

        auto hosts = server_config.Hosts();
        for (auto it = hosts.begin(); it != hosts.end(); ++it) {
            auto id = it->first;
            auto host = it->second;
            ClusterNode * cluster_node = new ClusterNode(id, host, 1, *this);
            try {
                cluster_nodes[id] = cluster_node;
            } catch (...) {
                delete cluster_node;
                throw;
            } 
        }

    } catch (...) {
        IOUtils::ClosePipe(shutdown_pipe);
        try {
            for (auto it = cluster_nodes.begin(); it != cluster_nodes.end(); ++it) {
                auto cluster_node = it->second;
                delete cluster_node;
            }
        } catch (...) {
            // Do nothing
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

    IOUtils::ClosePipe(shutdown_pipe);
}


void * Server::AcceptorThreadWrapper(void * ptr) {
    Server* server = static_cast<Server*>(ptr);
    try {
        server->AcceptorThreadMain();
    } catch (const exception& e) {
        cerr << "Acceptor thread crashed: " << e.what() << endl;
        abort();
    } catch (...) {
        cerr << "Acceptor thread crashed" << endl;
        abort();
    }
    return nullptr;
}


void Server::Start(void) {
    for (auto it = cluster_nodes.begin(); it != cluster_nodes.end(); ++it) {
        auto cluster_node = it->second;
        cluster_node->Start();
    }

    int err = pthread_create(&acceptor_thread, NULL, AcceptorThreadWrapper, this);
    if (err == 0) {
        acceptor_thread_created = true;
    } else {
        throw ServerException("Error creating acceptor thread: " + string(strerror(errno)));
    }
}


void Server::AcceptorThreadMain(void) {

    auto server_id = server_config.ServerId();
    auto hosts = server_config.Hosts();
    auto bind_address = server_config.BindAddress();
    auto use_ipv4 = server_config.UseIPV4();
    auto use_ipv6 = server_config.UseIPV6();

    int listen_fd = IOUtils::ListenSocket(bind_address, 1024, use_ipv4, use_ipv6);
    IOUtils::SetNonBlocking(listen_fd);

    struct pollfd fds[2];
    fds[0].fd = shutdown_pipe[0];
    fds[0].events = POLLIN;
    fds[1].fd = listen_fd;
    fds[1].events = POLLIN;

    try {
        for (;;) {
            int num_fd_ready = poll(fds, sizeof(fds) / sizeof(fds[0]), -1);
            if (num_fd_ready == -1) {
                if (errno == EINTR || errno == EAGAIN) {
                    continue;
                } else {
                    throw IOException("Probleming polling acceptor fd: " + string(strerror(errno)));
                }
            } else {
                if ((fds[0].revents & POLLIN) != 0) {
                    break;
                }
                if ((fds[1].revents & POLLIN) != 0) {
                    /*
                     * Accept up to 1024 connections before looping again. This ensures that we don't live-lock
                     * during shutdown.
                     */
                    for (int i = 0; i < 1024; i++) {
                        struct sockaddr_storage sockaddr;
                        socklen_t address_len = sizeof(sockaddr);
                        int fd = accept(listen_fd, (struct sockaddr *)&sockaddr, &address_len);
                        if (fd == -1) {
                            if (errno == EINTR) {
                                continue;
                            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                /* try poll()ing again */
                                break;
                            } else {
                                fprintf(stderr, "Problem accepting connection: %s\n", strerror(errno));
                                break;
                            }
                        } else {
                            /* make the connection nonblocking */
                            try {
                                IOUtils::SetNonBlocking(fd);
                            } catch (...) {
                                cerr << "Problem setting accepted connection to be nonblocking: " << strerror(errno) << endl;
                                IOUtils::Close(fd);
                                break;
                            }

                            IncomingConnection(fd);
                        }
                    }
                }
            }
        }
    } catch (...) {
        IOUtils::Close(listen_fd);
        throw;
    }

    IOUtils::Close(listen_fd);
}


void Server::Shutdown(void) {
    IOUtils::ForceWriteByte(shutdown_pipe[1], 0);
}


void Server::IncomingConnection(int fd) {    
}


Server::ClusterNode::ClusterNode(uint32_t id, SocketAddress const& address, int port, Server& server) :
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
