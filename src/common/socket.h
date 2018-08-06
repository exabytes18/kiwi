#ifndef KIWI_SOCKET_H_
#define KIWI_SOCKET_H_

#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>


class Socket {
public:
    Socket(int domain, int type, int protocol) noexcept;
    Socket(int fd) noexcept;
    ~Socket(void) noexcept;
    int GetFD(void) noexcept;
    void SetNonBlocking(bool nonblocking) noexcept;
    void SetReuseAddr(bool reuse_addr) noexcept;
    int Bind(struct sockaddr const* addr, socklen_t addrlen) noexcept;
    int Connect(struct sockaddr const* addr, socklen_t addrlen) noexcept;
    int GetErrorCode(void) noexcept;

    // Move constructor + move assignment operator
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    // Delete copy constructor and copy assignment operator
    Socket(Socket const& other) = delete;
    Socket& operator=(Socket const& other) = delete;

protected:
    int fd;
};

#endif  // KIWI_SOCKET_H_
