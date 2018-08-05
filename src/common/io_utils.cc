#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "exceptions.h"
#include "io_utils.h"


void IOUtils::Close(int fd) noexcept {
    // It's not necessarily safe to call close() a second time if the first
    // call returned an error, as operating systems are permitted to handle
    // this as they see fit.
    //
    // TL;DR: for true portability, we might need to retry the close() on some OS.
    int err = close(fd);
    if (err == -1) {
        std::cerr << "Problem closing file descriptor: " << strerror(errno) << std::endl;
    }
}


void IOUtils::ForceWriteByte(int fd, char c) noexcept {
    char buf[1] = {c};
    for (;;) {
        ssize_t bytes_written = write(fd, buf, 1);
        switch (bytes_written) {
            case 1:
                return;

            case 0:
                continue;

            case -1:
                if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                } else {
                    std::cerr << "Failed to write byte: " << strerror(errno) << std::endl;
                    abort();
                }

            default:
                std::cerr << "Expected to write 1 byte, but wrote " << bytes_written << " bytes! How did this happen?" << std::endl;
                abort();
        }
    }
}


static IOUtils::AutoCloseableSocket OpenAndBindSocket(IOUtils::AutoCloseableAddrInfo& addrs) {
    while (addrs.HasNext()) {
        struct addrinfo* addr = addrs.Next();

        IOUtils::AutoCloseableSocket socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        socket.SetReuseAddr(true);
        if (socket.Bind(addr->ai_addr, addr->ai_addrlen) == -1) {
            throw IOException("Problem calling bind(2): " + std::string(strerror(errno)));
        }

        return socket;
    }

    throw IOException("Unable to create socket: " + std::string(strerror(errno)));
}


static IOUtils::AutoCloseableSocket OpenAndBindSocket(SocketAddress const& address, bool use_ipv4, bool use_ipv6) {
    int ai_family;
    if (use_ipv4 && use_ipv6) {
        ai_family = AF_UNSPEC;
    } else if (use_ipv4) {
        ai_family = AF_INET;
    } else if (use_ipv6) {
        ai_family = AF_INET6;
    } else {
        ai_family = AF_UNSPEC;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_STREAM;

    IOUtils::AutoCloseableAddrInfo addrs(address, hints);
    return OpenAndBindSocket(addrs);
}


IOUtils::AutoCloseableSocket IOUtils::CreateListenSocket(SocketAddress const& address, bool use_ipv4, bool use_ipv6, int backlog) {
    IOUtils::AutoCloseableSocket socket = OpenAndBindSocket(address, use_ipv4, use_ipv6);
    if (listen(socket.GetFD(), backlog) != 0) {
        throw IOException("Problem listening to socket: " + std::string(strerror(errno)));
    }
    return socket;
}


IOUtils::AutoCloseableSocket::AutoCloseableSocket(int domain, int type, int protocol) {
    fd = socket(domain, type, protocol);
    if (fd == -1) {
        throw IOException("Problem opening socket: " + std::string(strerror(errno)));
    }
}


IOUtils::AutoCloseableSocket::AutoCloseableSocket(int fd) noexcept : fd(fd) {
}


IOUtils::AutoCloseableSocket::~AutoCloseableSocket(void) noexcept {
    if (fd != -1) {
        Close(fd);
    }
}


IOUtils::AutoCloseableSocket::AutoCloseableSocket(IOUtils::AutoCloseableSocket&& other) noexcept : fd(other.fd) {
    other.fd = -1;
}


IOUtils::AutoCloseableSocket& IOUtils::AutoCloseableSocket::operator=(IOUtils::AutoCloseableSocket&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (fd != -1) {
        Close(fd);
    }

    fd = other.fd;
    other.fd = -1;
    return *this;
}


int IOUtils::AutoCloseableSocket::GetFD(void) noexcept {
    return fd;
}


void IOUtils::AutoCloseableSocket::SetNonBlocking(bool nonblocking) noexcept {
    for (;;) {
        int existing_flags = fcntl(fd, F_GETFL);
        if (existing_flags == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                std::cerr << "Problem fetching fd flags: " << strerror(errno) << std::endl;
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
                std::cerr << "Problem setting fd flags: " << strerror(errno) << std::endl;
                abort();
            }
        }
        return;
    }
}


void IOUtils::AutoCloseableSocket::SetReuseAddr(bool reuse_addr) noexcept {
    int optval = (reuse_addr) ? (1) : (0);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
        std::cerr << "WARNING: problem setting SO_REUSEADDR: " << strerror(errno) << std::endl;
        abort();
    }
}


int IOUtils::AutoCloseableSocket::Bind(struct sockaddr const* addr, socklen_t addrlen) noexcept {
    return bind(fd, addr, addrlen);
}


int IOUtils::AutoCloseableSocket::Connect(struct sockaddr const* addr, socklen_t addrlen) noexcept {
    return connect(fd, addr, addrlen);
}


int IOUtils::AutoCloseableSocket::GetErrorCode(void) noexcept {
    int optval;
    socklen_t optsize;
    int err = getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optsize);
    if (err == -1) {
        std::cerr << "WARNING: problem getting socket error status: " << strerror(errno) << std::endl;
        abort();
    }

    return optval;
}


IOUtils::AutoCloseableAddrInfo::AutoCloseableAddrInfo(SocketAddress const& address, struct addrinfo const& hints) {
    char bind_port_str[16];
    sprintf(bind_port_str, "%d", address.Port());

    int err = getaddrinfo(address.Address().c_str(), bind_port_str, &hints, &addrs);
    if (err != 0) {
        throw DnsResolutionException("getaddrinfo: " + std::string(gai_strerror(err)));
    }

    next = addrs;
}


bool IOUtils::AutoCloseableAddrInfo::HasNext(void) noexcept {
    return next != nullptr;
}


struct addrinfo* IOUtils::AutoCloseableAddrInfo::Next(void) noexcept {
    struct addrinfo* tmp = next;
    next = next->ai_next;
    return tmp;
}


IOUtils::AutoCloseableAddrInfo::~AutoCloseableAddrInfo(void) noexcept {
    freeaddrinfo(addrs);
}
