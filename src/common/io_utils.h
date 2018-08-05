#ifndef KIWI_IO_UTILS_H_
#define KIWI_IO_UTILS_H_

#include <netdb.h>
#include <string>
#include "socket_address.h"


namespace IOUtils {
    class AutoCloseableSocket {
    public:
        AutoCloseableSocket(int domain, int type, int protocol);
        AutoCloseableSocket(int fd) noexcept;
        ~AutoCloseableSocket(void) noexcept;
        int GetFD(void) noexcept;
        void SetNonBlocking(bool nonblocking) noexcept;
        void SetReuseAddr(bool reuse_addr) noexcept;
        int Bind(struct sockaddr const* addr, socklen_t addrlen) noexcept;
        int Connect(struct sockaddr const* addr, socklen_t addrlen) noexcept;
        int GetErrorCode(void) noexcept;

        // Use default move constructor and move assignment operator
        AutoCloseableSocket(AutoCloseableSocket&& other) noexcept;
        AutoCloseableSocket& operator=(AutoCloseableSocket&& other) noexcept;

        // Delete copy constructor and copy assignment operator
        AutoCloseableSocket(AutoCloseableSocket const& other) = delete;
        AutoCloseableSocket& operator=(AutoCloseableSocket const& other) = delete;

    private:
        int fd;
    };

    class AutoCloseableAddrInfo {
    public:
        AutoCloseableAddrInfo(SocketAddress const& address, struct addrinfo const& hints);
        ~AutoCloseableAddrInfo(void) noexcept;

        bool HasNext(void) noexcept;
        struct addrinfo* Next(void) noexcept;

    private:
        struct addrinfo* addrs;
        struct addrinfo* next;
    };

    void Close(int fd) noexcept;
    void ForceWriteByte(int fd, char c) noexcept;
    AutoCloseableSocket CreateListenSocket(SocketAddress const& address, bool use_ipv4, bool use_ipv6, int backlog);
}

#endif  // KIWI_IO_UTILS_H_
