#include <iostream>
#include <unistd.h>
#include "connection_dispatcher.h"
#include "exceptions.h"
#include "io_utils.h"
#include "lock_guard.h"
#include "protocol.h"


ConnectionDispatcher::ConnectionDispatcher(Server& server) :
        server(server),
        event_loop(),
        managed_connections_mutex(),
        managed_connections() {

    if (pipe(shutdown_pipe) != 0) {
        throw ServerException("Error creating shutdown pipe: " + string(strerror(errno)));
    }

    try {
        IOUtils::SetNonBlocking(shutdown_pipe[0]);
        IOUtils::SetNonBlocking(shutdown_pipe[1]);
        event_loop.Add(shutdown_pipe[0], EventLoop::EVENT_READ, nullptr);

        int err = pthread_create(&dispatch_thread, nullptr, DispatchThreadWrapper, this);
        if (err != 0) {
            throw ServerException("Error creating dispatch thread: " + string(strerror(err)));
        }

    } catch (...) {
        IOUtils::Close(shutdown_pipe[0]);
        IOUtils::Close(shutdown_pipe[1]);
        throw;
    }
}


ConnectionDispatcher::~ConnectionDispatcher(void) {
    IOUtils::ForceWriteByte(shutdown_pipe[1], 0);
    int err = pthread_join(dispatch_thread, nullptr);
    if (err != 0) {
        cerr << "Fatal: problem joining dispatch thread: " << strerror(err) << endl;
        abort();
    }

    // Close any connections that still remain in our managed_connections set.
    for (auto connection : managed_connections) {
        delete connection;
    }

    IOUtils::Close(shutdown_pipe[0]);
    IOUtils::Close(shutdown_pipe[1]);
}


void* ConnectionDispatcher::DispatchThreadWrapper(void* ptr) {
    ConnectionDispatcher* connection_dispatcher = static_cast<ConnectionDispatcher*>(ptr);
    try {
        connection_dispatcher->DispatchThreadMain();
    } catch (exception const& e) {
        cerr << "Dispatch thread crashed: " << e.what() << endl;
        abort();
    } catch (...) {
        cerr << "Dispatch thread crashed" << endl;
        abort();
    }
    return nullptr;
}


void ConnectionDispatcher::CloseAndDeregisterConnection(ConnectionContext* ctx) {
    BufferedNetworkConnection* connection = ctx->GetBufferedNetworkConnection();

    // Remove fd from event loop, then close the fd (so kqueue is happy). Inverting the order will trigger an error.
    event_loop.Remove(connection->GetFD());
    DeregisterConnection(connection);
    delete connection;
    delete ctx;
}


void ConnectionDispatcher::DispatchThreadMain(void) {
    for (;;) {
        EventLoop::Notification notification = event_loop.GetReadyFD();

        int fd = notification.GetFD();
        if (fd == shutdown_pipe[0]) {
            break;
        }

        ConnectionContext* ctx = static_cast<ConnectionContext*>(notification.GetData());
        try {
            int events = notification.GetEvents();
            if (events & EventLoop::EVENT_READ) {
                ctx->ReadFromSocketAndProcessData();
            }
            if (events & EventLoop::EVENT_WRITE) {
                ctx->WriteToSocket();
            }
        } catch (ConnectionClosedException const& e) {
            CloseAndDeregisterConnection(ctx);
        } catch (...) {
            CloseAndDeregisterConnection(ctx);
            throw;
        }
    }
}


void ConnectionDispatcher::HandleIncomingConnection(BufferedNetworkConnection* connection) {
    RegisterConnection(connection);
    try {
        ConnectionContext* ctx = new ConnectionContext(*this, connection);
        try {
            event_loop.Add(connection->GetFD(), EventLoop::EVENT_READ, ctx);
        } catch (...) {
            delete ctx;
            throw;
        }

    } catch (...) {
        DeregisterConnection(connection);
        throw;
    }
}


void ConnectionDispatcher::RegisterConnection(BufferedNetworkConnection* connection) {
    LockGuard lock_guard(managed_connections_mutex);
    managed_connections.insert(connection);
}


void ConnectionDispatcher::DeregisterConnection(BufferedNetworkConnection* connection) {
    LockGuard lock_guard(managed_connections_mutex);
    managed_connections.erase(connection);
}


void ConnectionDispatcher::DispatchClientConnection(BufferedNetworkConnection* connection) {
    server.HandleIncomingClientConnection(connection);
}


void ConnectionDispatcher::DispatchClusterNodeConnection(BufferedNetworkConnection* connection) {
    server.HandleIncomingClusterNodeConnection(connection);
}


ConnectionDispatcher::ConnectionContext::ConnectionContext(
    ConnectionDispatcher& connection_dispatcher,
    BufferedNetworkConnection* connection) :
        connection_dispatcher(connection_dispatcher),
        connection(connection),
        read_state(ReadState::READING_FIRST_REQUEST_MESSAGE_TYPE),
        first_request_message_type_buffer(4),
        magic_number_buffer(4),
        protocol_version_buffer(4),
        server_id_buffer(4),
        cluster_name_length_buffer(2),
        cluster_name_buffer(nullptr) {}


ConnectionDispatcher::ConnectionContext::~ConnectionContext(void) {
    if (cluster_name_buffer != nullptr) {
        delete cluster_name_buffer;
    }
}


BufferedNetworkConnection* ConnectionDispatcher::ConnectionContext::GetBufferedNetworkConnection(void) {
    return connection;
}


void ConnectionDispatcher::ConnectionContext::ReadFromSocketAndProcessData(void) {
    for (;;) {
        switch (read_state) {
            case ReadState::READING_FIRST_REQUEST_MESSAGE_TYPE:
                if (connection->Fill(&first_request_message_type_buffer)) {
                    first_request_message_type_buffer.Flip();
                    first_request_message_type_int = first_request_message_type_buffer.UnsafeGetInt();
                    switch (first_request_message_type_int) {
                        case Protocol::MessageType::CLIENT_HELLO:
                            first_request_message_type = Protocol::MessageType::CLIENT_HELLO;
                            read_state = ReadState::READING_CLIENT_HELLO_MAGIC_NUMBER;
                            break;

                        case Protocol::MessageType::SERVER_HELLO:
                            first_request_message_type = Protocol::MessageType::SERVER_HELLO;
                            read_state = ReadState::READING_SERVER_HELLO_MAGIC_NUMBER;
                            break;

                        default:
                            // TODO: send Error Reply (Unexpected message type)
                            read_state = ReadState::TERMINAL;
                            break;
                    }
                } else {
                    return;
                }
                break;

            case ReadState::READING_CLIENT_HELLO_MAGIC_NUMBER:
                if (connection->Fill(&magic_number_buffer)) {
                    magic_number_buffer.Flip();
                    magic_number = magic_number_buffer.UnsafeGetInt();
                    if (magic_number == Protocol::MAGIC_NUMBER) {
                        read_state = ReadState::READING_CLIENT_HELLO_PROTOCOL_VERSION;
                    } else {
                        // TODO: send Error Reply (Unrecognized magic number)
                        read_state = ReadState::TERMINAL;
                    }
                } else {
                    return;
                }
                break;

            case ReadState::READING_CLIENT_HELLO_PROTOCOL_VERSION:
                if (connection->Fill(&protocol_version_buffer)) {
                    protocol_version_buffer.Flip();
                    protocol_version = protocol_version_buffer.UnsafeGetInt();
                    if (protocol_version == Protocol::PROTOCOL_VERSION) {
                        connection_dispatcher.DispatchClientConnection(connection);
                        read_state = ReadState::TERMINAL;
                    } else {
                        // TODO: send Error Reply (Unsupported Protocol Version)
                        read_state = ReadState::TERMINAL;
                    }
                } else {
                    return;
                }
                break;

            case ReadState::READING_SERVER_HELLO_MAGIC_NUMBER:
                if (connection->Fill(&magic_number_buffer)) {
                    magic_number_buffer.Flip();
                    magic_number = magic_number_buffer.UnsafeGetInt();
                    if (magic_number == Protocol::MAGIC_NUMBER) {
                        read_state = ReadState::READING_SERVER_HELLO_PROTOCOL_VERSION;
                    } else {
                        // TODO: send Error Reply (Unrecognized magic number)
                        read_state = ReadState::TERMINAL;
                    }
                } else {
                    return;
                }
                break;

            case ReadState::READING_SERVER_HELLO_PROTOCOL_VERSION:
                if (connection->Fill(&protocol_version_buffer)) {
                    protocol_version_buffer.Flip();
                    protocol_version = protocol_version_buffer.UnsafeGetInt();
                    if (protocol_version == Protocol::PROTOCOL_VERSION) {
                        read_state = ReadState::READING_SERVER_HELLO_SERVER_ID;
                    } else {
                        // TODO: send Error Reply (Unsupported Protocol Version)
                        read_state = ReadState::TERMINAL;
                    }
                } else {
                    return;
                }
                break;

            case ReadState::READING_SERVER_HELLO_SERVER_ID:
                if (connection->Fill(&server_id_buffer)) {
                    server_id_buffer.Flip();
                    server_id = server_id_buffer.UnsafeGetInt();
                    read_state = ReadState::READING_SERVER_HELLO_CLUSTER_NAME_LENGTH;
                } else {
                    return;
                }
                break;

            case ReadState::READING_SERVER_HELLO_CLUSTER_NAME_LENGTH:
                if (connection->Fill(&cluster_name_length_buffer)) {
                    cluster_name_length_buffer.Flip();
                    cluster_name_length = cluster_name_length_buffer.UnsafeGetShort();
                    cluster_name_buffer = new Buffer(cluster_name_length);
                    read_state = ReadState::READING_SERVER_HELLO_CLUSTER_NAME;
                } else {
                    return;
                }
                break;

            case ReadState::READING_SERVER_HELLO_CLUSTER_NAME:
                if (connection->Fill(cluster_name_buffer)) {
                    cluster_name_buffer->Flip();
                    cluster_name = cluster_name_buffer->UnsafeGetString(cluster_name_length);
                    connection_dispatcher.DispatchClusterNodeConnection(connection);
                    read_state = ReadState::TERMINAL;
                } else {
                    return;
                }
                break;

            case ReadState::TERMINAL:
                return;

            default:
                cerr << "Unknown read state: " << static_cast<int>(read_state) << endl;
                abort();
        }
    }
}


void ConnectionDispatcher::ConnectionContext::WriteToSocket(void) {
    // TODO: implement
}
