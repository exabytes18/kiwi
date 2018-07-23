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
        throw ServerException("Error adding (ident,filter) pair to kqueue: " + string(strerror(errno)));
    }

    if (event.flags & EV_ERROR) {
        throw ServerException("Error adding (ident,filter) pair to kqueue: " + string(strerror(event.data)));
    }
}


void Server::RemoveFD(int fd, short filter) {
    struct kevent event;
    EV_SET(&event, fd, filter, EV_DELETE, 0, 0, nullptr);

    int err = kevent(kq, &event, 1, nullptr, 0, nullptr);
    if (err == -1) {
        throw ServerException("Error removing (ident,filter) pair from kqueue: " + string(strerror(errno)));
    }

    if (event.flags & EV_ERROR) {
        throw ServerException("Error removing (ident,filter) pair from kqueue: " + string(strerror(event.data)));
    }
}


void Server::ThreadMain(void) {

    auto server_id = config.ServerId();
    auto hosts = config.Hosts();
    auto bind_address = config.BindAddress();
    auto use_ipv4 = config.UseIPV4();
    auto use_ipv6 = config.UseIPV6();

    int listen_fd = IOUtils::ListenSocket(
        bind_address,
        use_ipv4,
        use_ipv6,
        128);

    IOUtils::SetNonBlocking(listen_fd);
    AddFD(listen_fd, EVFILT_READ, nullptr);

    // Start connection attempts for all higher-numbered peers

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
                         * Accept up to 64 connections before looping again. This ensures that we don't live-lock
                         * during shutdown.
                         */
                        for (int i = 0; i < 64; i++) {
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
                                Connection* connection;
                                try {
                                    connection = new Connection(*this, fd);
                                } catch (...) {
                                    IOUtils::Close(fd);
                                    throw;
                                }

                                try {
                                    connections.insert(connection);
                                } catch (...) {
                                    delete connection;
                                }
                            }
                        }
                    } else {
                        Connection* connection = static_cast<Connection*>(event->udata);
                        switch (event->filter) {
                            case EVFILT_READ:
                                connection->Recv();
                                break;
                            
                            case EVFILT_WRITE:
                                connection->Send();
                                break;

                            default:
                                cerr << "Unexpected event filter: " << event->filter << endl;
                                break;
                        }
                    }
                }
            }
        }

        for (Connection* connection : connections) {
            // Deleting the BufferedNetworkStream will close the underlying fd, which internally will
            // also remove it from the kqueue (whereas with epoll, closing the underlying fd leaves the fd
            // still a member of any epoll fds to which it was added).
            delete connection;
        }

    } catch (...) {
        IOUtils::Close(listen_fd);
        throw;
    }

    IOUtils::Close(listen_fd);
}


Server::Connection::Connection(Server& server, int fd) :
        server(server),
        stream(fd),
        watching_for_writability(false),
        read_state(ReadState::READING_MESSAGE_TYPE),
        message_type_buffer(4),
        magic_number_buffer(4),
        protocol_version_buffer(4),
        server_id_buffer(4),
        cluster_name_length_buffer(2),
        cluster_name_buffer(0) {

    IOUtils::SetNonBlocking(stream.GetFD());
    server.AddFD(stream.GetFD(), EVFILT_READ, this);
}


bool Server::Connection::Recv(void) {
    for (;;) {
        switch (read_state) {
            case ReadState::READING_MESSAGE_TYPE:
                switch (stream.Fill(message_type_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        message_type_buffer.Flip();
                        message_type_int = message_type_buffer.UnsafeGetInt();
                        switch (message_type_int) {
                            case Protocol::MessageType::CLIENT_HELLO:
                                message_type = Protocol::MessageType::CLIENT_HELLO;
                                read_state = ReadState::READING_CLIENT_HELLO_MAGIC_NUMBER;
                                break;

                            case Protocol::MessageType::SERVER_HELLO:
                                message_type = Protocol::MessageType::SERVER_HELLO;
                                read_state = ReadState::READING_SERVER_HELLO_MAGIC_NUMBER;
                                break;

                            default:
                                // TODO: close the stream... we don't even know how to reply
                                read_state = ReadState::TERMINAL;
                                break;
                        }
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return true;

                    case BufferedNetworkStream::Status::closed:
                        return false;
                }
                break;

            case ReadState::READING_CLIENT_HELLO_MAGIC_NUMBER:
                switch (stream.Fill(magic_number_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        magic_number_buffer.Flip();
                        magic_number = magic_number_buffer.UnsafeGetInt();
                        if (magic_number == Protocol::MAGIC_NUMBER) {
                            read_state = ReadState::READING_CLIENT_HELLO_PROTOCOL_VERSION;
                        } else {
                            read_state = ReadState::TERMINAL;
                            PrepareStateForSendingSimpleReply(
                                Protocol::MessageType::CLIENT_HELLO_REPLY,
                                Protocol::ErrorCode::INVALID_MAGIC_NUMBER,
                                Protocol::InvalidMagicNumberErrorMessage(magic_number));
                        }
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return true;

                    case BufferedNetworkStream::Status::closed:
                        return false;
                }
                break;

            case ReadState::READING_CLIENT_HELLO_PROTOCOL_VERSION:
                switch (stream.Fill(protocol_version_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        protocol_version_buffer.Flip();
                        protocol_version = protocol_version_buffer.UnsafeGetInt();
                        if (protocol_version == Protocol::PROTOCOL_VERSION) {
                            read_state = ReadState::READING_MESSAGE_TYPE;
                        } else {
                            read_state = ReadState::TERMINAL;
                            PrepareStateForSendingSimpleReply(
                                Protocol::MessageType::CLIENT_HELLO_REPLY,
                                Protocol::ErrorCode::UNSUPPORTED_PROTOCOL_VERSION,
                                Protocol::UnsupportedProtocolVersionErrorMessage(protocol_version));
                        }
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return true;

                    case BufferedNetworkStream::Status::closed:
                        return false;
                }
                break;

            case ReadState::READING_SERVER_HELLO_MAGIC_NUMBER:
                switch (stream.Fill(magic_number_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        magic_number_buffer.Flip();
                        magic_number = magic_number_buffer.UnsafeGetInt();
                        if (magic_number == Protocol::MAGIC_NUMBER) {
                            read_state = ReadState::READING_SERVER_HELLO_PROTOCOL_VERSION;
                        } else {
                            read_state = ReadState::TERMINAL;
                            PrepareStateForSendingSimpleReply(
                                Protocol::MessageType::SERVER_HELLO_REPLY,
                                Protocol::ErrorCode::INVALID_MAGIC_NUMBER,
                                Protocol::InvalidMagicNumberErrorMessage(magic_number));
                        }
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return true;

                    case BufferedNetworkStream::Status::closed:
                        return false;
                }
                break;

            case ReadState::READING_SERVER_HELLO_PROTOCOL_VERSION:
                switch (stream.Fill(protocol_version_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        protocol_version_buffer.Flip();
                        protocol_version = protocol_version_buffer.UnsafeGetInt();
                        if (protocol_version == Protocol::PROTOCOL_VERSION) {
                            read_state = ReadState::READING_SERVER_HELLO_SERVER_ID;
                        } else {
                            read_state = ReadState::TERMINAL;
                            PrepareStateForSendingSimpleReply(
                                Protocol::MessageType::SERVER_HELLO_REPLY,
                                Protocol::ErrorCode::UNSUPPORTED_PROTOCOL_VERSION,
                                Protocol::UnsupportedProtocolVersionErrorMessage(protocol_version));
                        }
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return true;

                    case BufferedNetworkStream::Status::closed:
                        return false;
                }
                break;

            case ReadState::READING_SERVER_HELLO_SERVER_ID:
                switch (stream.Fill(server_id_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        server_id_buffer.Flip();
                        server_id = server_id_buffer.UnsafeGetInt();
                        read_state = ReadState::READING_SERVER_HELLO_CLUSTER_NAME_LENGTH;
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return true;

                    case BufferedNetworkStream::Status::closed:
                        return false;
                }
                break;

            case ReadState::READING_SERVER_HELLO_CLUSTER_NAME_LENGTH:
                switch (stream.Fill(cluster_name_length_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        cluster_name_length_buffer.Flip();
                        cluster_name_length = cluster_name_length_buffer.UnsafeGetShort();
                        cluster_name_buffer.ResetAndGrow(cluster_name_length);
                        read_state = ReadState::READING_SERVER_HELLO_CLUSTER_NAME;
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return true;

                    case BufferedNetworkStream::Status::closed:
                        return false;
                }
                break;

            case ReadState::READING_SERVER_HELLO_CLUSTER_NAME:
                switch (stream.Fill(cluster_name_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        cluster_name_buffer.Flip();
                        cluster_name = cluster_name_buffer.UnsafeGetString(cluster_name_length);
                        read_state = ReadState::READING_MESSAGE_TYPE;
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return true;

                    case BufferedNetworkStream::Status::closed:
                        return false;
                }
                break;

            case ReadState::TERMINAL:
                return false;

            default:
                cerr << "Unknown read state: " << static_cast<int>(read_state) << endl;
                abort();
        }
    }
}


bool Server::Connection::Send(void) {
}


void Server::Connection::WatchForWritability(bool watch_for_writability) {
    if (watching_for_writability != watch_for_writability) {
        if (watch_for_writability) {
            server.AddFD(stream.GetFD(), EVFILT_WRITE, this);
        } else {
            server.RemoveFD(stream.GetFD(), EVFILT_WRITE);
        }
        watching_for_writability = watch_for_writability;
    }
}


Server::Connection::~Connection(void) {
}


Server::~Server(void) {
    struct kevent event;
    EV_SET(&event, kSHUTDOWN/*ident*/, EVFILT_USER/*filter*/, EV_ADD | EV_CLEAR/*flags*/, NOTE_TRIGGER/*fflags*/, 0/*data*/, nullptr/*user data*/);

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
