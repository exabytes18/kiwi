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

    IOUtils::AutoCloseableFD auto_closeable_listen_fd(listen_fd);
    IOUtils::SetNonBlocking(listen_fd);
    AddFD(listen_fd, EVFILT_READ, nullptr);

    // Start connection attempts for all higher-numbered peers

    bool shutdown = false;
    while (!shutdown) { // We may want to relax this a bit and allow more graceful termination rather than immediately breaking from the loop.
        struct kevent events[64];
        int num_events = kevent(kq, nullptr, 0, events, sizeof(events) / sizeof(events[0]), nullptr);
        if (num_events == -1) {
            cerr << "Problem querying ready events from kqueue: " << strerror(errno) << endl;
            abort();
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
                                connection = new Connection(fd);
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
                            }
                        }
                    }
                } else {
                    Connection* connection = static_cast<Connection*>(event->udata);
                    switch (event->filter) {
                        case EVFILT_READ:
                            RecvData(connection);
                            break;
                        
                        case EVFILT_WRITE:
                            SendData(connection);
                            break;

                        default:
                            cerr << "Unexpected event filter: " << event->filter << endl;
                            abort();
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
}


void Server::RecvData(Connection* connection) {
    for (;;) {
        switch (connection->read_state) {
            case Connection::ReadState::READING_MESSAGE_TYPE:
                cout << "READING_MESSAGE_TYPE" << endl;
                switch (connection->stream.Fill(connection->message_type_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        connection->message_type_buffer.Flip();
                        connection->message_type_int = connection->message_type_buffer.UnsafeGetInt();
                        connection->message_type_buffer.Clear();
                        switch (connection->message_type_int) {
                            case Protocol::MessageType::CLIENT_HELLO:
                                connection->message_type = Protocol::MessageType::CLIENT_HELLO;
                                connection->read_state = Connection::ReadState::READING_CLIENT_HELLO_MAGIC_NUMBER;
                                break;

                            case Protocol::MessageType::SERVER_HELLO:
                                connection->message_type = Protocol::MessageType::SERVER_HELLO;
                                connection->read_state = Connection::ReadState::READING_SERVER_HELLO_MAGIC_NUMBER;
                                break;

                            default:
                                CloseConnection(connection);
                                return;
                        }
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return;

                    case BufferedNetworkStream::Status::closed:
                        CloseConnection(connection);
                        return;
                }
                break;

            case Connection::ReadState::READING_CLIENT_HELLO_MAGIC_NUMBER:
                cout << "READING_CLIENT_HELLO_MAGIC_NUMBER" << endl;
                switch (connection->stream.Fill(connection->magic_number_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        connection->magic_number_buffer.Flip();
                        connection->magic_number = connection->magic_number_buffer.UnsafeGetInt();
                        connection->magic_number_buffer.Clear();
                        if (connection->magic_number == Protocol::MAGIC_NUMBER) {
                            connection->read_state = Connection::ReadState::READING_CLIENT_HELLO_PROTOCOL_VERSION;
                        } else {
                            connection->read_state = Connection::ReadState::TERMINAL;
                            SendErrorReplyAndCloseConnection(
                                connection,
                                Protocol::ErrorCode::INVALID_MAGIC_NUMBER,
                                Protocol::InvalidMagicNumberErrorMessage(connection->magic_number));
                        }
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return;

                    case BufferedNetworkStream::Status::closed:
                        CloseConnection(connection);
                        return;
                }
                break;

            case Connection::ReadState::READING_CLIENT_HELLO_PROTOCOL_VERSION:
                cout << "READING_CLIENT_HELLO_PROTOCOL_VERSION" << endl;
                switch (connection->stream.Fill(connection->protocol_version_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        connection->protocol_version_buffer.Flip();
                        connection->protocol_version = connection->protocol_version_buffer.UnsafeGetInt();
                        connection->protocol_version_buffer.Clear();
                        if (connection->protocol_version == Protocol::PROTOCOL_VERSION) {
                            connection->read_state = Connection::ReadState::READING_MESSAGE_TYPE;
                        } else {
                            connection->read_state = Connection::ReadState::TERMINAL;
                            SendErrorReplyAndCloseConnection(
                                connection,
                                Protocol::ErrorCode::UNSUPPORTED_PROTOCOL_VERSION,
                                Protocol::UnsupportedProtocolVersionErrorMessage(connection->protocol_version));
                        }
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return;

                    case BufferedNetworkStream::Status::closed:
                        CloseConnection(connection);
                        return;
                }
                break;

            case Connection::ReadState::READING_SERVER_HELLO_MAGIC_NUMBER:
                cout << "READING_SERVER_HELLO_MAGIC_NUMBER" << endl;
                switch (connection->stream.Fill(connection->magic_number_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        connection->magic_number_buffer.Flip();
                        connection->magic_number = connection->magic_number_buffer.UnsafeGetInt();
                        connection->magic_number_buffer.Clear();
                        if (connection->magic_number == Protocol::MAGIC_NUMBER) {
                            connection->read_state = Connection::ReadState::READING_SERVER_HELLO_PROTOCOL_VERSION;
                        } else {
                            connection->read_state = Connection::ReadState::TERMINAL;
                            SendErrorReplyAndCloseConnection(
                                connection,
                                Protocol::ErrorCode::INVALID_MAGIC_NUMBER,
                                Protocol::InvalidMagicNumberErrorMessage(connection->magic_number));
                        }
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return;

                    case BufferedNetworkStream::Status::closed:
                        CloseConnection(connection);
                        return;
                }
                break;

            case Connection::ReadState::READING_SERVER_HELLO_PROTOCOL_VERSION:
                cout << "READING_SERVER_HELLO_PROTOCOL_VERSION" << endl;
                switch (connection->stream.Fill(connection->protocol_version_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        connection->protocol_version_buffer.Flip();
                        connection->protocol_version = connection->protocol_version_buffer.UnsafeGetInt();
                        connection->protocol_version_buffer.Clear();
                        if (connection->protocol_version == Protocol::PROTOCOL_VERSION) {
                            connection->read_state = Connection::ReadState::READING_SERVER_HELLO_SERVER_ID;
                        } else {
                            connection->read_state = Connection::ReadState::TERMINAL;
                            SendErrorReplyAndCloseConnection(
                                connection,
                                Protocol::ErrorCode::UNSUPPORTED_PROTOCOL_VERSION,
                                Protocol::UnsupportedProtocolVersionErrorMessage(connection->protocol_version));
                        }
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return;

                    case BufferedNetworkStream::Status::closed:
                        CloseConnection(connection);
                        return;
                }
                break;

            case Connection::ReadState::READING_SERVER_HELLO_SERVER_ID:
                cout << "READING_SERVER_HELLO_SERVER_ID" << endl;
                switch (connection->stream.Fill(connection->server_id_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        connection->server_id_buffer.Flip();
                        connection->server_id = connection->server_id_buffer.UnsafeGetInt();
                        connection->server_id_buffer.Clear();
                        connection->read_state = Connection::ReadState::READING_SERVER_HELLO_CLUSTER_NAME_LENGTH;
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return;

                    case BufferedNetworkStream::Status::closed:
                        CloseConnection(connection);
                        return;
                }
                break;

            case Connection::ReadState::READING_SERVER_HELLO_CLUSTER_NAME_LENGTH:
                cout << "READING_SERVER_HELLO_CLUSTER_NAME_LENGTH" << endl;
                switch (connection->stream.Fill(connection->cluster_name_length_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        connection->cluster_name_length_buffer.Flip();
                        connection->cluster_name_length = connection->cluster_name_length_buffer.UnsafeGetShort();
                        connection->cluster_name_buffer.ResetAndGrow(connection->cluster_name_length);
                        connection->read_state = Connection::ReadState::READING_SERVER_HELLO_CLUSTER_NAME;
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return;

                    case BufferedNetworkStream::Status::closed:
                        CloseConnection(connection);
                        return;
                }
                break;

            case Connection::ReadState::READING_SERVER_HELLO_CLUSTER_NAME:
                cout << "READING_SERVER_HELLO_CLUSTER_NAME" << endl;
                switch (connection->stream.Fill(connection->cluster_name_buffer)) {
                    case BufferedNetworkStream::Status::complete:
                        connection->cluster_name_buffer.Flip();
                        connection->cluster_name = connection->cluster_name_buffer.UnsafeGetString(connection->cluster_name_length);
                        connection->cluster_name_buffer.Clear();
                        connection->read_state = Connection::ReadState::READING_MESSAGE_TYPE;
                        break;

                    case BufferedNetworkStream::Status::incomplete:
                        return;

                    case BufferedNetworkStream::Status::closed:
                        CloseConnection(connection);
                        return;
                }
                break;

            case Connection::ReadState::TERMINAL:
                cout << "TERMINAL" << endl;
                return;

            default:
                cerr << "Unknown read state: " << static_cast<int>(connection->read_state) << endl;
                abort();
        }
    }
}


void Server::SendData(Connection* connection) {
}


void Server::SendErrorReplyAndCloseConnection(
        Connection* connection,
        Protocol::ErrorCode error_code,
        std::string error_message) {
}


void Server::WatchForWritability(Connection* connection, bool watch_for_writability) {
    if (connection->watching_for_writability != watch_for_writability) {
        if (watch_for_writability) {
            AddFD(connection->stream.GetFD(), EVFILT_WRITE, this);
        } else {
            RemoveFD(connection->stream.GetFD(), EVFILT_WRITE);
        }
        connection->watching_for_writability = watch_for_writability;
    }
}


void Server::CloseConnection(Connection* connection) {
    cout << "Closing connection" << endl;
    connections.erase(connection);
    delete connection;
}


Server::Connection::Connection(int fd) :
        stream(fd),
        watching_for_writability(false),
        read_state(ReadState::READING_MESSAGE_TYPE),
        message_type_buffer(4),
        magic_number_buffer(4),
        protocol_version_buffer(4),
        server_id_buffer(4),
        cluster_name_length_buffer(2),
        cluster_name_buffer(0) {

    IOUtils::SetNonBlocking(fd);
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
