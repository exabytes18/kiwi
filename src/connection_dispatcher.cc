#include <iostream>
#include <unistd.h>
#include "connection_dispatcher.h"
#include "exceptions.h"
#include "io_utils.h"
#include "lock_guard.h"


ConnectionDispatcher::ConnectionDispatcher(Server& server) :
        server(server),
        event_loop(),
        watched_connections_mutex(),
        watched_connections() {

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

    // TODO: Close all fd that remain in watched_connections set

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


void ConnectionDispatcher::DispatchThreadMain(void) {
    for (;;) {
        EventLoop::Notification notification = event_loop.Poll();

        int fd = notification.GetFD();
        if (fd == shutdown_pipe[0]) {
            break;
        }

        int events = notification.GetEvents();
        if (events & EventLoop::EVENT_READ) {
            // readable
        }

        if (events & EventLoop::EVENT_WRITE) {
            // writable
        }
    }
}


void ConnectionDispatcher::HandleConnection(int fd) {
    try {
        RegisterConnection(fd);
        ConnectionState* conn_state = new ConnectionState();
        try {
            event_loop.Add(fd, EventLoop::EVENT_READ, conn_state);
        } catch (...) {
            delete conn_state;
            throw;
        }

    } catch (...) {
        IOUtils::Close(fd);
        DeregisterConnection(fd);
        throw;
    }
}


void ConnectionDispatcher::RegisterConnection(int fd) {
    LockGuard lock_guard(watched_connections_mutex);
    watched_connections.insert(fd);
}


void ConnectionDispatcher::DeregisterConnection(int fd) {
    LockGuard lock_guard(watched_connections_mutex);
    watched_connections.erase(fd);
}
