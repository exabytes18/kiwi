#ifndef KIWI_IO_UTILS_H_
#define KIWI_IO_UTILS_H_

#include <netdb.h>
#include <string>
#include "socket_address.h"


class IOUtils {
public:
    static void Close(int fd) noexcept;
    static void ForceWriteByte(int fd, char c) noexcept;
    static void SetNonBlocking(int fd);
    static int BindSocket(SocketAddress const& address, bool use_ipv4, bool use_ipv6);
    static void Listen(int fd, int backlog);

private:
    IOUtils() = delete;
    static int BindSocket(struct addrinfo* addrs);
};

#endif  // KIWI_IO_UTILS_H_
