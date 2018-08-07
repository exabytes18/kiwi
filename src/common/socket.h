#ifndef KIWI_SOCKET_H_
#define KIWI_SOCKET_H_

#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

#include "abstract_socket.h"
#include "buffer.h"


class Socket : public AbstractSocket {
public:
    Socket(int domain, int type, int protocol);
    Socket(int fd) noexcept;
    ~Socket(void) noexcept;

    // Move constructor + move assignment operator
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    // Delete copy constructor and copy assignment operator
    Socket(Socket const& other) = delete;
    Socket& operator=(Socket const& other) = delete;
};

#endif  // KIWI_SOCKET_H_
