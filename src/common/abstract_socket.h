#ifndef KIWI_ABSTRACT_SOCKET_H_
#define KIWI_ABSTRACT_SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>


class AbstractSocket {
public:
    int GetFD(void) noexcept;
    void SetNonBlocking(bool nonblocking) noexcept;
    void SetReuseAddr(bool reuse_addr) noexcept;
    int Bind(struct sockaddr const* addr, socklen_t addrlen) noexcept;
    int Connect(struct sockaddr const* addr, socklen_t addrlen) noexcept;
    int GetErrorCode(void) noexcept;

protected:
    int fd;

    AbstractSocket(int domain, int type, int protocol) noexcept;
    AbstractSocket(int fd) noexcept;
    ~AbstractSocket(void) noexcept;

    // Move constructor + move assignment operator
    AbstractSocket(AbstractSocket&& other) noexcept;
    AbstractSocket& operator=(AbstractSocket&& other) noexcept;

    // Delete copy constructor and copy assignment operator
    AbstractSocket(AbstractSocket const& other) = delete;
    AbstractSocket& operator=(AbstractSocket const& other) = delete;
};

#endif  // KIWI_ABSTRACT_SOCKET_H_
