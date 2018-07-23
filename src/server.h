#ifndef KIWI_SERVER_H_
#define KIWI_SERVER_H_

#include <set>
#include "buffered_network_stream.h"
#include "protocol.h"
#include "server_config.h"
#include "storage.h"


class Server {
public:
    Server(ServerConfig const& config, Storage& storage);
    ~Server(void);

    class Connection {
    public:
        Connection(Server& server, int fd);
        ~Connection(void);
        bool Recv(void);
        bool Send(void);

    private:
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

        Server& server;
        BufferedNetworkStream stream;
        bool watching_for_writability;
        ReadState read_state;

        Buffer message_type_buffer;
        uint32_t message_type_int;
        Protocol::MessageType message_type;

        Buffer magic_number_buffer;
        uint32_t magic_number;

        Buffer protocol_version_buffer;
        uint32_t protocol_version;

        Buffer server_id_buffer;
        uint32_t server_id;

        Buffer cluster_name_length_buffer;
        uint16_t cluster_name_length;

        Buffer cluster_name_buffer;
        std::string cluster_name;

        void WatchForWritability(bool watch_for_writability);
    };

private:
    ServerConfig const& config;
    Storage& storage;
    int kq;
    pthread_t thread;
    std::set<Connection*> connections;

    static void* ThreadWrapper(void* ptr);
    void ThreadMain(void);
    void AddFD(int fd, short filter, void* data);
    void RemoveFD(int fd, short filter);
};

#endif  // KIWI_SERVER_H_
