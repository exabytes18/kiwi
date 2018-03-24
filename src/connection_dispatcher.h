#ifndef KIWI_CONNECTION_DISPATCHER_H_
#define KIWI_CONNECTION_DISPATCHER_H_

#include <pthread.h>
#include <unordered_map>
#include "buffered_network_connection.h"
#include "connection_dispatch_context.h"
#include "event_loop.h"
#include "mutex.h"
#include "server.h"

using namespace std;


class ConnectionDispatcher {
public:
    ConnectionDispatcher(Server& server);
    ~ConnectionDispatcher(void);
    void HandleIncomingConnection(unique_ptr<BufferedNetworkConnection> connection);

private:
    Server& server;
    EventLoop event_loop;
    int shutdown_pipe[2];
    pthread_t dispatch_thread;

    Mutex managed_context_mutex;
    unordered_map<int, ConnectionDispatchContext*> managed_context;

    static void* DispatchThreadWrapper(void* ptr);
    void DispatchThreadMain(void);
    void AddToManagedContextMap(int fd, ConnectionDispatchContext* ctx);
    void RemoveFromManagedContextMap(int fd);
};

#endif  // KIWI_CONNECTION_DISPATCHER_H_
