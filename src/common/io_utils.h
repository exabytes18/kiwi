#ifndef KIWI_IO_UTILS_H_
#define KIWI_IO_UTILS_H_

#include <netdb.h>
#include <string>
#include "socket_address.h"


namespace IOUtils {
    class AutoCloseableSocket {
    public:
        AutoCloseableSocket(int fd) noexcept;
        ~AutoCloseableSocket(void) noexcept;
        int GetFD(void) noexcept;
        void SetNonBlocking(void) noexcept;

        // Use default move constructor and move assignment operator
        AutoCloseableSocket(AutoCloseableSocket&& other) noexcept;
        AutoCloseableSocket& operator=(AutoCloseableSocket&& other) noexcept;

        // Delete copy constructor and copy assignment operator
        AutoCloseableSocket(AutoCloseableSocket const& other) = delete;
        AutoCloseableSocket& operator=(AutoCloseableSocket const& other) = delete;

    private:
        int fd;
    };

    void Close(int fd) noexcept;
    void ForceWriteByte(int fd, char c) noexcept;
    AutoCloseableSocket CreateListenSocket(SocketAddress const& address, bool use_ipv4, bool use_ipv6, int backlog);
}

#endif  // KIWI_IO_UTILS_H_
