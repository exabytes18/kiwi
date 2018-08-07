#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "abstract_socket.h"
#include "exceptions.h"
#include "io_utils.h"


using namespace std;

AbstractSocket::AbstractSocket(int fd) noexcept :
        fd(fd) {}


AbstractSocket::AbstractSocket(AbstractSocket&& other) noexcept : fd(other.fd) {
    other.fd = -1;
}


AbstractSocket& AbstractSocket::operator=(AbstractSocket&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (fd == -1) {
        IOUtils::Close(fd);
    }

    fd = other.fd;
    other.fd = -1;
    return *this;
}


AbstractSocket::~AbstractSocket(void) noexcept {
    if (fd != -1) {
        IOUtils::Close(fd);
    }
}


int AbstractSocket::GetFD(void) noexcept {
    return fd;
}


void AbstractSocket::SetNonBlocking(bool nonblocking) noexcept {
    for (;;) {
        int existing_flags = fcntl(fd, F_GETFL);
        if (existing_flags == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                cerr << "Problem fetching fd flags: " << strerror(errno) << endl;
                abort();
            }
        }

        int updated_flags;
        if (nonblocking) {
            updated_flags = existing_flags | O_NONBLOCK;
        } else {
            updated_flags = existing_flags & ~O_NONBLOCK;
        }

        int result = fcntl(fd, F_SETFL, updated_flags);
        if (result == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                cerr << "Problem setting fd flags: " << strerror(errno) << endl;
                abort();
            }
        }
        return;
    }
}


void AbstractSocket::SetReuseAddr(bool reuse_addr) noexcept {
    int optval = (reuse_addr) ? (1) : (0);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
        cerr << "WARNING: problem setting SO_REUSEADDR: " << strerror(errno) << endl;
        abort();
    }
}


int AbstractSocket::Bind(struct sockaddr const* addr, socklen_t addrlen) noexcept {
    return bind(fd, addr, addrlen);
}


int AbstractSocket::Connect(struct sockaddr const* addr, socklen_t addrlen) noexcept {
    return connect(fd, addr, addrlen);
}


int AbstractSocket::Listen(int backlog) noexcept {
    return listen(fd, backlog);
}


int AbstractSocket::GetErrorCode(void) noexcept {
    int optval;
    socklen_t optsize;
    int err = getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optsize);
    if (err == -1) {
        cerr << "WARNING: problem getting socket error status: " << strerror(errno) << endl;
        abort();
    }

    return optval;
}
