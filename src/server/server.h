#ifndef KIWI_SERVER_H_
#define KIWI_SERVER_H_

#include <deque>
#include <set>
#include "common/buffered_network_stream.h"
#include "common/io_utils.h"
#include "common/protocol.h"
#include "server_config.h"
#include "storage.h"


class Server {
public:
    Server(ServerConfig const& config, Storage& storage);
    ~Server(void);

private:
    class Connection {
    public:
        Connection(IOUtils::AutoCloseableSocket socket);
        ~Connection(void);

        enum class ReadState {
            READING_MESSAGE_TYPE,
            READING_CLIENT_HELLO_MAGIC_NUMBER,
            READING_CLIENT_HELLO_PROTOCOL_VERSION,
            READING_SERVER_HELLO_MAGIC_NUMBER,
            READING_SERVER_HELLO_PROTOCOL_VERSION,
            READING_SERVER_HELLO_SERVER_ID,
            READING_SERVER_HELLO_CLUSTER_NAME_LENGTH,
            READING_SERVER_HELLO_CLUSTER_NAME,
            TERMINAL,
        };

        // High-level connection state
        BufferedNetworkStream stream;
        bool interested_in_reads;
        bool interested_in_writes;
        ReadState read_state;
        bool close_connection_after_all_buffers_have_been_flushed;

        // Server Connection Data
        uint32_t server_id;

        // Temporary buffers for incoming data
        Buffer incoming_message_type_buffer;
        Buffer incoming_magic_number_buffer;
        Buffer incoming_protocol_version_buffer;
        Buffer incoming_server_id_buffer;
        Buffer incoming_cluster_name_length_buffer;
        Buffer incoming_cluster_name_buffer;

        // Temporary buffers for outgoing data
        std::deque<Buffer> outgoing_buffers;
    };

    ServerConfig const& config;
    Storage& storage;
    int kq;
    pthread_t thread;
    std::set<Connection*> connections;

    static void* ThreadWrapper(void* ptr);
    void ThreadMain(void);
    void AddFD(int fd, short filter, void* data);
    void RemoveFD(int fd, short filter);
    void RecvData(Connection* connection);
    void SendData(Connection* connection);

    void SendClientHelloReply(Connection* connection);
    void SendClientTestReply(Connection* connection);
    void SendServerHelloReply(Connection* connection);    

    void StopReadingAndSendErrorReplyAndClose(Connection* connection, Protocol::ErrorCode error_code, std::string error_message);
    void SetReadInterest(Connection* connection, bool interested_in_reads);
    void SetWriteInterest(Connection* connection, bool interested_in_writes);
    void CloseAndDestroy(Connection* connection);
};

#endif  // KIWI_SERVER_H_
