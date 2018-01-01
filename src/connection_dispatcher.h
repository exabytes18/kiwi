#ifndef KIWI_CONNECTION_DISPATCHER_H_
#define KIWI_CONNECTION_DISPATCHER_H_

#include <pthread.h>
#include <set>
#include "event_loop.h"
#include "mutex.h"
#include "server.h"

using namespace std;


class ConnectionDispatcher {
public:
    ConnectionDispatcher(Server& server);
    ~ConnectionDispatcher(void);
    void HandleConnection(int fd);

private:
    Server& server;
    EventLoop event_loop;
    int shutdown_pipe[2];
    pthread_t dispatch_thread;
    set<int> watched_connections;
    Mutex watched_connections_mutex;

    static void* DispatchThreadWrapper(void* ptr);
    void DispatchThreadMain(void);
    void RegisterConnection(int fd);
    void DeregisterConnection(int fd);
};

#endif  // KIWI_CONNECTION_DISPATCHER_H_
