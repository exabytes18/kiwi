#ifndef KIWI_IO_UTILS_H_
#define KIWI_IO_UTILS_H_

#include <netdb.h>
#include <string>

#include "buffered_socket.h"
#include "socket_address.h"


namespace IOUtils {
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
    BufferedSocket CreateListenSocket(SocketAddress const& address, bool use_ipv4, bool use_ipv6, int backlog);
}

#endif  // KIWI_IO_UTILS_H_
