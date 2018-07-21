#include <iostream>
#include "connection_dispatch_context.h"


using namespace std;

/*
ConnectionDispatchContext::ConnectionDispatchContext(
    Server& server,
    unique_ptr<BufferedNetworkConnection> connection) :
        server(server),
        connection(move(connection)),
        read_state(ReadState::READING_FIRST_REQUEST_MESSAGE_TYPE),
        first_request_message_type_buffer(4),
        magic_number_buffer(4),
        protocol_version_buffer(4),
        server_id_buffer(4),
        cluster_name_length_buffer(2),
        cluster_name_buffer(0) {}


ConnectionDispatchContext::~ConnectionDispatchContext(void) {
}


void ConnectionDispatchContext::PrepareStateForSendingSimpleReply(Protocol::MessageType message_type, uint32_t error_code, string error_message) {
}


void ConnectionDispatchContext::ReadFromSocketAndProcessData(void) {
    for (;;) {
        switch (read_state) {
            case ReadState::READING_FIRST_REQUEST_MESSAGE_TYPE:
                if (connection->Fill(first_request_message_type_buffer)) {
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
                            // TODO: close the connection... we don't even know how to reply
                            read_state = ReadState::TERMINAL;
                            break;
                    }
                } else {
                    return;
                }
                break;

            case ReadState::READING_CLIENT_HELLO_MAGIC_NUMBER:
                if (connection->Fill(magic_number_buffer)) {
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
                } else {
                    return;
                }
                break;

            case ReadState::READING_CLIENT_HELLO_PROTOCOL_VERSION:
                if (connection->Fill(protocol_version_buffer)) {
                    protocol_version_buffer.Flip();
                    protocol_version = protocol_version_buffer.UnsafeGetInt();
                    if (protocol_version == Protocol::PROTOCOL_VERSION) {
                        server.HandleIncomingClientConnection(move(connection));
                        read_state = ReadState::TERMINAL;
                    } else {
                        read_state = ReadState::TERMINAL;
                        PrepareStateForSendingSimpleReply(
                            Protocol::MessageType::CLIENT_HELLO_REPLY,
                            Protocol::ErrorCode::UNSUPPORTED_PROTOCOL_VERSION,
                            Protocol::UnsupportedProtocolVersionErrorMessage(protocol_version));
                    }
                } else {
                    return;
                }
                break;

            case ReadState::READING_SERVER_HELLO_MAGIC_NUMBER:
                if (connection->Fill(magic_number_buffer)) {
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
                } else {
                    return;
                }
                break;

            case ReadState::READING_SERVER_HELLO_PROTOCOL_VERSION:
                if (connection->Fill(protocol_version_buffer)) {
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
                } else {
                    return;
                }
                break;

            case ReadState::READING_SERVER_HELLO_SERVER_ID:
                if (connection->Fill(server_id_buffer)) {
                    server_id_buffer.Flip();
                    server_id = server_id_buffer.UnsafeGetInt();
                    read_state = ReadState::READING_SERVER_HELLO_CLUSTER_NAME_LENGTH;
                } else {
                    return;
                }
                break;

            case ReadState::READING_SERVER_HELLO_CLUSTER_NAME_LENGTH:
                if (connection->Fill(cluster_name_length_buffer)) {
                    cluster_name_length_buffer.Flip();
                    cluster_name_length = cluster_name_length_buffer.UnsafeGetShort();
                    cluster_name_buffer.ResetAndGrow(cluster_name_length);
                    read_state = ReadState::READING_SERVER_HELLO_CLUSTER_NAME;
                } else {
                    return;
                }
                break;

            case ReadState::READING_SERVER_HELLO_CLUSTER_NAME:
                if (connection->Fill(cluster_name_buffer)) {
                    cluster_name_buffer.Flip();
                    cluster_name = cluster_name_buffer.UnsafeGetString(cluster_name_length);
                    server.HandleIncomingClusterNodeConnection(move(connection));
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


void ConnectionDispatchContext::WriteToSocket(void) {
    // TODO: implement
}

*/