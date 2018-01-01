#ifndef KIWI_CONNECTION_LISTENER_H_
#define KIWI_CONNECTION_LISTENER_H_

#include <pthread.h>
#include "connection_dispatcher.h"
#include "server_config.h"


class ConnectionListener {
public:
    ConnectionListener(ServerConfig const& config, ConnectionDispatcher& connection_dispatcher);
    ~ConnectionListener(void);

private:
    ServerConfig const& config;
    ConnectionDispatcher& connection_dispatcher;
    int shutdown_pipe[2];
    pthread_t acceptor_thread;

    static void* AcceptorThreadWrapper(void* ptr);
    void AcceptorThreadMain(void);
};

#endif  // KIWI_CONNECTION_LISTENER_H_
