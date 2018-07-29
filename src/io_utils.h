#ifndef KIWI_IO_UTILS_H_
#define KIWI_IO_UTILS_H_

#include <netdb.h>
#include <string>
#include "socket_address.h"


namespace IOUtils {
    void Close(int fd) noexcept;
    void ForceWriteByte(int fd, char c) noexcept;
    void SetNonBlocking(int fd);
    int ListenSocket(SocketAddress const& address, bool use_ipv4, bool use_ipv6, int backlog);

    class AutoCloseableFD {
    public:
        AutoCloseableFD(int fd) noexcept;
        ~AutoCloseableFD(void) noexcept;

        // Use default move constructor and move assignment operator
        AutoCloseableFD(AutoCloseableFD&& other) noexcept;
        AutoCloseableFD& operator=(AutoCloseableFD&& other) noexcept;

        // Delete copy constructor and copy assignment operator
        AutoCloseableFD(AutoCloseableFD const& other) = delete;
        AutoCloseableFD& operator=(AutoCloseableFD const& other) = delete;

    private:
        int fd;
    };
}

#endif  // KIWI_IO_UTILS_H_
