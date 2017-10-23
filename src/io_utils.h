#ifndef KIWI_IO_UTILS_H_
#define KIWI_IO_UTILS_H_

#include <netdb.h>
#include <string>
#include "socket_address.h"


namespace IOUtils {
    void Close(int fd);
    void ClosePipe(int pipe[2]);
    void ForceWriteByte(int fd, char c);
    void SetNonBlocking(int fd);
    int OpenSocket(struct addrinfo* addrs);
    int ListenSocket(SocketAddress const& address, int listen_backlog, bool use_ipv4, bool use_ipv6);
}

#endif  // KIWI_IO_UTILS_H_
