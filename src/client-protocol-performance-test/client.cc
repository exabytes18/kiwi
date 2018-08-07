#include "common/config.h"
#if defined(HAVE_EPOLL)
    #include <sys/epoll.h>
#elif defined(HAVE_KQUEUE)
    #include <sys/event.h>
#endif

#include <errno.h>
#include <exception>
#include <iostream>
#include <unistd.h>

#include "common/buffered_socket.h"
#include "common/exceptions.h"
#include "common/io_utils.h"
#include "common/protocol.h"
#include "client.h"


using namespace std;

enum KqueueUserEventID {
    kSHUTDOWN = 1,
};

Client::Client(SocketAddress const& server_address) : server_address(server_address) {
    kq = kqueue();
    if (kq == -1) {
        throw runtime_error("Error creating kqueue: " + string(strerror(errno)));
    }

    try {
        int err = pthread_create(&thread, nullptr, ThreadWrapper, this);
        if (err != 0) {
            throw runtime_error("Error creating IO thread: " + string(strerror(err)));
        }

    } catch (...) {
        IOUtils::Close(kq);
        throw;
    }
}


void* Client::ThreadWrapper(void* ptr) {
    Client* client = static_cast<Client*>(ptr);
    try {
        client->ThreadMain();
    } catch (exception const& e) {
        cerr << "IO thread crashed: " << e.what() << endl;
        abort();
    } catch (...) {
        cerr << "IO thread crashed" << endl;
        abort();
    }
    return nullptr;
}


void Client::AddEventInterest(int ident, short filter, void* data) {
    struct kevent event;
    EV_SET(&event, ident, filter, EV_ADD, 0, 0, data);

    int err = kevent(kq, &event, 1, nullptr, 0, nullptr);
    if (err == -1) {
        throw IOException("Error adding (ident,filter) pair to kqueue: " + string(strerror(errno)));
    }

    if (event.flags & EV_ERROR) {
        throw IOException("Error adding (ident,filter) pair to kqueue: " + string(strerror(event.data)));
    }
}


void Client::ThreadMain(void) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[16];
    sprintf(port_str, "%d", server_address.Port());

    bool shutdown = false;
    while (!shutdown) {
        IOUtils::AutoCloseableAddrInfo addrs(server_address, hints);
        while (!shutdown && addrs.HasNext()) {
            struct addrinfo* addr = addrs.Next();

            BufferedSocket socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            socket.SetNonBlocking(true);

            Buffer sink(64 * 1024);
            Buffer client_hello(12);
            Buffer client_test(4);

            client_hello.UnsafePutInt(Protocol::MessageType::CLIENT_HELLO);
            client_hello.UnsafePutInt(Protocol::MAGIC_NUMBER);
            client_hello.UnsafePutInt(Protocol::PROTOCOL_VERSION);
            client_hello.Flip();

            client_test.UnsafePutInt(Protocol::MessageType::CLIENT_TEST);
            client_test.Flip();

            bool connected;
            if (socket.Connect(addr->ai_addr, addr->ai_addrlen) == 0) {
                connected = true;
                AddEventInterest(socket.GetFD(), EVFILT_READ, nullptr);
            } else {
                if (errno == EINPROGRESS) {
                    connected = false;
                } else if (errno == ECONNREFUSED || errno == ECONNRESET || errno == ENETUNREACH || errno == ETIMEDOUT) {
                    break;
                } else {
                    throw IOException("Problem calling connect(2): " + string(strerror(errno)));
                }
            }

            bool closed = false;
            AddEventInterest(socket.GetFD(), EVFILT_WRITE, nullptr);
            while (!shutdown && !closed) {
                struct kevent events[64];
                int num_events = kevent(kq, nullptr, 0, events, sizeof(events) / sizeof(events[0]), nullptr);
                if (num_events == -1) {
                    cerr << "Problem querying ready events from kqueue: " << strerror(errno) << endl;
                    abort();
                }

                for (int i = 0; i < num_events; i++) {
                    struct kevent* event = &events[i];
                    if (event->filter == EVFILT_USER) {
                        KqueueUserEventID event_id = static_cast<KqueueUserEventID>(event->ident);
                        switch (event_id) {
                            case kSHUTDOWN:
                                shutdown = true;
                                break;

                            default:
                                cerr << "Unknown event id: " << event_id << endl;
                                abort();
                        }
                    } else {
                        switch (event->filter) {
                            case EVFILT_READ:
                                switch (socket.Fill(sink)) {
                                    case BufferedSocket::RecvStatus::complete:
                                        sink.Clear();
                                        break;

                                    case BufferedSocket::RecvStatus::incomplete:
                                        break;

                                    case BufferedSocket::RecvStatus::closed:
                                        closed = true;
                                        break;
                                }
                                break;

                            case EVFILT_WRITE:
                                if (connected) {
                                    switch (socket.Write(client_hello)) {
                                        case BufferedSocket::SendStatus::complete:
                                            break;

                                        case BufferedSocket::SendStatus::incomplete:
                                            break;

                                        case BufferedSocket::SendStatus::closed:
                                            closed = true;
                                            break;
                                    }

                                    switch (socket.Write(client_test)) {
                                        case BufferedSocket::SendStatus::complete:
                                            client_test.Flip();
                                            break;

                                        case BufferedSocket::SendStatus::incomplete:
                                            break;

                                        case BufferedSocket::SendStatus::closed:
                                            closed = true;
                                            break;
                                    }

                                    // TODO: flush?

                                } else {
                                    if (socket.GetErrorCode() == 0) {
                                        connected = true;
                                        AddEventInterest(socket.GetFD(), EVFILT_READ, nullptr);
                                    } else {
                                        closed = true;
                                    }
                                }
                                break;

                            default:
                                cerr << "Unexpected event filter: " << event->filter << endl;
                                abort();
                        }
                    }
                }
            }
        }

        // Sleep and try again later.
    }
}


Client::~Client(void) noexcept {
    struct kevent event;
    EV_SET(&event, kSHUTDOWN/*ident*/, EVFILT_USER/*filter*/, EV_ADD/*flags*/, NOTE_TRIGGER/*fflags*/, 0/*data*/, nullptr/*user data*/);

    int err = kevent(kq, &event, 1, nullptr, 0, nullptr);
    if (err == -1) {
        cerr << "Fatal: problem notifying event loop about shutdown: " << strerror(err) << endl;
        abort();
    }

    if (event.flags & EV_ERROR) {
        cerr << "Fatal: problem notifying event loop about shutdown: " << strerror(event.data) << endl;
        abort();
    }

    err = pthread_join(thread, nullptr);
    if (err != 0) {
        cerr << "Fatal: problem joining IO thread: " << strerror(err) << endl;
        abort();
    }

    IOUtils::Close(kq);
}
