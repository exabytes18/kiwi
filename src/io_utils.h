#ifndef KIWI_IO_UTILS_H_
#define KIWI_IO_UTILS_H_

#include <netdb.h>
#include <string>
#include "socket_address.h"


namespace IOUtils {
    void Close(int fd) noexcept;
    void ForceWriteByte(int fd, char c) noexcept;
    void SetNonBlocking(int fd);
    int BindSocket(SocketAddress const& address, bool use_ipv4, bool use_ipv6);
    void Listen(int fd, int backlog);
}

#endif  // KIWI_IO_UTILS_H_
