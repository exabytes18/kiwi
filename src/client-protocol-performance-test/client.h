#ifndef KIWI_CLIENT_H_
#define KIWI_CLIENT_H_

#include <stdint.h>
#include <string>
#include "common/socket_address.h"


class Client {
public:
    Client(SocketAddress const& server_address);
    ~Client(void) noexcept;

private:
    SocketAddress const& server_address;
    int kq;
    pthread_t thread;

    static void* ThreadWrapper(void* ptr);
    void ThreadMain(void);
    void AddEventInterest(int ident, short filter, void* data);
};

#endif  // KIWI_CLIENT_H_
