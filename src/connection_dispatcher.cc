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
        managed_context_mutex(),
        managed_context() {

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

    for (auto it = managed_context.begin(); it != managed_context.end(); ++it) {
        event_loop.Remove(it->first);
        delete it->second;
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


void ConnectionDispatcher::DispatchThreadMain(void) {
    for (;;) {
        EventLoop::Notification notification = event_loop.GetReadyFD();

        int fd = notification.GetFD();
        if (fd == shutdown_pipe[0]) {
            break;
        }

        // TODO: we need to rework this.
        ConnectionDispatchContext* ctx = static_cast<ConnectionDispatchContext*>(notification.GetData());
        try {
            int events = notification.GetEvents();
            if (events & EventLoop::EVENT_READ) {
                // connection->Fill(ctx->read_buffer);
                // ctx->Recv(ctx->read_buffer);
                ctx->ReadFromSocketAndProcessData();
            }
            if (events & EventLoop::EVENT_WRITE) {
                ctx->WriteToSocket();
            }

        } catch (ConnectionClosedException const& e) {
            event_loop.Remove(fd);
            RemoveFromManagedContextMap(fd);
            delete ctx;
        } catch (...) {
            event_loop.Remove(fd);
            RemoveFromManagedContextMap(fd);
            delete ctx;
            throw;
        }
    }
}


void ConnectionDispatcher::HandleIncomingConnection(unique_ptr<BufferedNetworkConnection> connection) {
    int fd = connection->GetFD();
    ConnectionDispatchContext* ctx = new ConnectionDispatchContext(server, move(connection));
    try {
        AddToManagedContextMap(fd, ctx);
        try {
            event_loop.Add(fd, EventLoop::EVENT_READ, ctx);
        } catch (...) {
            RemoveFromManagedContextMap(fd);
            throw;
        }

    } catch (...) {
        delete ctx;
        throw;
    }
}


void ConnectionDispatcher::AddToManagedContextMap(int fd, ConnectionDispatchContext* ctx) {
    LockGuard lock_guard(managed_context_mutex);
    managed_context[fd] = ctx;
}


void ConnectionDispatcher::RemoveFromManagedContextMap(int fd) {
    LockGuard lock_guard(managed_context_mutex);
    managed_context.erase(fd);
}
