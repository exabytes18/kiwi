#include "io_utils.h"
#include "socket.h"


Socket::Socket(int domain, int type, int protocol) :
        Socket(IOUtils::OpenSocketFD(domain, type, protocol)) {}


Socket::Socket(int fd) noexcept :
        AbstractSocket(fd) {}


Socket::Socket(Socket&& other) noexcept :
        AbstractSocket(other.fd) {
    other.fd = -1;
}


Socket& Socket::operator=(Socket&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (fd != -1) {
        IOUtils::Close(fd);
    }

    fd = other.fd;
    other.fd = -1;
    return *this;
}


Socket::~Socket(void) noexcept {
}
