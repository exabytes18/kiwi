#include "config.h"
#if defined(HAVE_EPOLL)
    #include <sys/epoll.h>
#elif defined(HAVE_KQUEUE)
    #include <sys/event.h>
#endif

#include <errno.h>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include "buffered_network_connection.h"
#include "exceptions.h"
#include "io_utils.h"
#include "server.h"


using namespace std;

enum KqueueUserEventID {
    kSHUTDOWN = 1,
    kMESSAGE = 2,
};

Server::Server(ServerConfig const& config, Storage& storage) :
        config(config),
        storage(storage),
        connections() {

    kq = kqueue();
    if (kq == -1) {
        throw ServerException("Error creating kqueue: " + string(strerror(errno)));
    }

    try {
        int err = pthread_create(&thread, nullptr, ThreadWrapper, this);
        if (err != 0) {
            throw ServerException("Error creating server thread: " + string(strerror(err)));
        }

    } catch (...) {
        IOUtils::Close(kq);
        throw;
    }
}


void* Server::ThreadWrapper(void* ptr) {
    Server* server = static_cast<Server*>(ptr);
    try {
        server->ThreadMain();
    } catch (exception const& e) {
        cerr << "Server thread crashed: " << e.what() << endl;
        abort();
    } catch (...) {
        cerr << "Server thread crashed" << endl;
        abort();
    }
    return nullptr;
}


void Server::AddFD(int fd, short filter, void* data) {
    struct kevent event;
    EV_SET(&event, fd, filter, EV_ADD, 0, 0, data);

    int err = kevent(kq, &event, 1, nullptr, 0, nullptr);
    if (err == -1) {
        throw ServerException("Error adding event to kqueue: " + string(strerror(errno)));
    }

    if (event.flags & EV_ERROR) {
        throw ServerException("Error adding event to kqueue: " + string(strerror(event.data)));
    }
}


void Server::ThreadMain(void) {

    auto server_id = config.ServerId();
    auto hosts = config.Hosts();
    auto bind_address = config.BindAddress();
    auto use_ipv4 = config.UseIPV4();
    auto use_ipv6 = config.UseIPV6();

    int listen_fd = IOUtils::BindSocket(bind_address, use_ipv4, use_ipv6);
    IOUtils::Listen(listen_fd, 1024);
    IOUtils::SetNonBlocking(listen_fd);
    AddFD(listen_fd, EVFILT_READ, nullptr);

    bool shutdown = false;
    try {
        while (!shutdown) { // We may want to relax this a bit and allow more graceful termination rather than immediately breaking from the loop.
            struct kevent events[64];
            int num_events = kevent(kq, nullptr, 0, events, sizeof(events) / sizeof(events[0]), nullptr);
            if (num_events == -1) {
                throw ServerException("Problem querying ready events from kqueue: " + string(strerror(errno)));
            }

            for (int i = 0; i < num_events; i++) {
                struct kevent* event = &events[i];
                if (event->filter == EVFILT_USER) {
                    KqueueUserEventID event_id = static_cast<KqueueUserEventID>(event->ident);
                    switch (event_id) {
                        case kSHUTDOWN:
                            shutdown = true;
                            break;

                        case kMESSAGE:
                            break;

                        default:
                            cerr << "Unknown event id: " << event_id << endl;
                            abort();
                    }
                } else {
                    int ready_fd = event->ident;
                    if (ready_fd == listen_fd) {
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
                                BufferedNetworkConnection* connection;
                                try {
                                    connection = new BufferedNetworkConnection(fd);
                                } catch (...) {
                                    IOUtils::Close(fd);
                                    throw;
                                }

                                try {
                                    connections.insert(connection);
                                } catch (...) {
                                    delete connection;
                                }

                                try {
                                    AddFD(fd, EVFILT_READ, connection);
                                } catch (...) {
                                    connections.erase(connection);
                                    delete connection;
                                    throw;
                                }
                            }
                        }
                    } else {
                        // client or server connection.
                    }
                }
            }
        }

        for (BufferedNetworkConnection* connection : connections) {
            // Deleting the BufferedNetworkConnection will close the underlying fd, which internally will
            // also remove it from the kq (whereas with epoll, closing the underlying fd leaves the fd
            // still a member of any epoll fds to which it was added).
            delete connection;
        }

    } catch (...) {
        IOUtils::Close(listen_fd);
        throw;
    }

    IOUtils::Close(listen_fd);
}


Server::~Server(void) {
    struct kevent event;
    EV_SET(&event, kSHUTDOWN/*ident*/, EVFILT_USER/*filter*/, EV_ADD/*flags*/, NOTE_TRIGGER/*fflags*/, 0/*data*/, nullptr/*user data*/);

    int err = kevent(kq, &event, 1, nullptr, 0, nullptr);
    if (err == -1) {
        cerr << "Fatal: problem notifying event loop about shutdown: " << strerror(err) << endl;
        abort();
    }

    if (event.flags & EV_ERROR) {
        cerr << "Fatal: problem notifying event loop about shutdown: " << strerror(event.data) << endl;
        abort();
    }

    err = pthread_join(thread, nullptr);
    if (err != 0) {
        cerr << "Fatal: problem joining server thread: " << strerror(err) << endl;
        abort();
    }

    IOUtils::Close(kq);
}
