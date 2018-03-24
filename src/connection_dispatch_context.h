#ifndef KIWI_DISPATCH_CONTEXT_H_
#define KIWI_DISPATCH_CONTEXT_H_

#include "buffered_network_connection.h"
#include "protocol.h"
#include "server.h"

using namespace std;


class ConnectionDispatchContext {
public:
    ConnectionDispatchContext(Server& server, unique_ptr<BufferedNetworkConnection> connection);
    ~ConnectionDispatchContext(void);
    void ReadFromSocketAndProcessData(void);
    void WriteToSocket(void);

    enum class ReadState {
        READING_FIRST_REQUEST_MESSAGE_TYPE,
        READING_CLIENT_HELLO_MAGIC_NUMBER,
        READING_CLIENT_HELLO_PROTOCOL_VERSION,
        READING_SERVER_HELLO_MAGIC_NUMBER,
        READING_SERVER_HELLO_PROTOCOL_VERSION,
        READING_SERVER_HELLO_SERVER_ID,
        READING_SERVER_HELLO_CLUSTER_NAME_LENGTH,
        READING_SERVER_HELLO_CLUSTER_NAME,
        TERMINAL,
    };

private:
    Server& server;
    unique_ptr<BufferedNetworkConnection> connection;

    ReadState read_state;

    Buffer first_request_message_type_buffer;
    uint32_t first_request_message_type_int;
    Protocol::MessageType first_request_message_type;

    Buffer magic_number_buffer;
    uint32_t magic_number;

    Buffer protocol_version_buffer;
    uint32_t protocol_version;

    Buffer server_id_buffer;
    uint32_t server_id;

    Buffer cluster_name_length_buffer;
    uint16_t cluster_name_length;

    Buffer cluster_name_buffer;
    string cluster_name;

    void PrepareStateForSendingSimpleReply(Protocol::MessageType message_type, uint32_t error_code, string error_message);
};

#endif  // KIWI_DISPATCH_CONTEXT_H_
