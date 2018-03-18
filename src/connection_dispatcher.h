#ifndef KIWI_CONNECTION_DISPATCHER_H_
#define KIWI_CONNECTION_DISPATCHER_H_

#include <pthread.h>
#include <set>
#include "buffered_network_connection.h"
#include "event_loop.h"
#include "mutex.h"
#include "protocol.h"
#include "server.h"

using namespace std;


class ConnectionDispatcher {
public:
    ConnectionDispatcher(Server& server);
    ~ConnectionDispatcher(void);
    void HandleIncomingConnection(BufferedNetworkConnection* buffered_network_connection);

    class ConnectionContext {
    public:
        ConnectionContext(ConnectionDispatcher& connection_dispatcher, BufferedNetworkConnection* buffered_network_connection);
        ~ConnectionContext(void);
        BufferedNetworkConnection* GetBufferedNetworkConnection(void);
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
        ConnectionDispatcher& connection_dispatcher;
        BufferedNetworkConnection* buffered_network_connection;
    
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

        Buffer* cluster_name_buffer;
        string cluster_name;
    };

private:
    Server& server;
    EventLoop event_loop;
    int shutdown_pipe[2];
    pthread_t dispatch_thread;

    Mutex watched_connections_mutex;
    set<BufferedNetworkConnection*> watched_connections;

    static void* DispatchThreadWrapper(void* ptr);
    void DispatchThreadMain(void);
    void RegisterConnection(BufferedNetworkConnection* buffered_network_connection);
    void DeregisterConnection(BufferedNetworkConnection* buffered_network_connection);
};

#endif  // KIWI_CONNECTION_DISPATCHER_H_
