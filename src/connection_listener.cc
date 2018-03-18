#include <errno.h>
#include <iostream>
#include <poll.h>
#include <unistd.h>
#include "buffered_network_connection.h"
#include "connection_listener.h"
#include "exceptions.h"
#include "io_utils.h"


ConnectionListener::ConnectionListener(ServerConfig const& config, ConnectionDispatcher& connection_dispatcher) :
        config(config),
        connection_dispatcher(connection_dispatcher) {

    if (pipe(shutdown_pipe) != 0) {
        throw ServerException("Error creating shutdown pipe: " + string(strerror(errno)));
    }

    try {
        IOUtils::SetNonBlocking(shutdown_pipe[0]);
        IOUtils::SetNonBlocking(shutdown_pipe[1]);

        int err = pthread_create(&acceptor_thread, nullptr, AcceptorThreadWrapper, this);
        if (err != 0) {
            throw ServerException("Error creating acceptor thread: " + string(strerror(err)));
        }

    } catch (...) {
        IOUtils::Close(shutdown_pipe[0]);
        IOUtils::Close(shutdown_pipe[1]);
        throw;
    }
}


ConnectionListener::~ConnectionListener(void) {
    IOUtils::ForceWriteByte(shutdown_pipe[1], 0);
    int err = pthread_join(acceptor_thread, nullptr);
    if (err != 0) {
        cerr << "Fatal: problem joining acceptor thread: " << strerror(err) << endl;
        abort();
    }

    IOUtils::Close(shutdown_pipe[0]);
    IOUtils::Close(shutdown_pipe[1]);
}


void* ConnectionListener::AcceptorThreadWrapper(void* ptr) {
    ConnectionListener* connection_listener = static_cast<ConnectionListener*>(ptr);
    try {
        connection_listener->AcceptorThreadMain();
    } catch (exception const& e) {
        cerr << "Connection acceptor thread crashed: " << e.what() << endl;
        abort();
    } catch (...) {
        cerr << "Connection acceptor thread crashed" << endl;
        abort();
    }
    return nullptr;
}


void ConnectionListener::AcceptorThreadMain(void) {
    auto server_id = config.ServerId();
    auto hosts = config.Hosts();
    auto bind_address = config.BindAddress();
    auto use_ipv4 = config.UseIPV4();
    auto use_ipv6 = config.UseIPV6();

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
                    throw IOException("Problem polling acceptor fd: " + string(strerror(errno)));
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
                                cerr << "Problem accepting connection: " << strerror(errno) << endl;
                                break;
                            }
                        } else {
                            try {
                                IOUtils::SetNonBlocking(fd);
                            } catch (...) {
                                IOUtils::Close(fd);
                                throw;
                            }

                            BufferedNetworkConnection* buffered_network_connection;
                            try {
                                buffered_network_connection = new BufferedNetworkConnection(fd);
                            } catch (...) {
                                IOUtils::Close(fd);
                            }

                            try {
                                connection_dispatcher.HandleIncomingConnection(buffered_network_connection);
                            } catch (...) {
                                delete buffered_network_connection;
                            }
                        }
                    }

                    fds[1].revents = 0;
                }
            }
        }
    } catch (...) {
        IOUtils::Close(listen_fd);
        throw;
    }

    IOUtils::Close(listen_fd);
}
