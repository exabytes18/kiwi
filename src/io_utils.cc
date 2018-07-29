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


void IOUtils::SetNonBlocking(int fd) {
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

        int updated_flags = fcntl(fd, F_SETFL, existing_flags | O_NONBLOCK);
        if (updated_flags == -1) {
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


static int OpenAndBindSocket(struct addrinfo* addrs) {
    if (addrs == nullptr) {
        throw IOException("No addresses found");
    }

    int fd;
    struct addrinfo* addr;
    for (addr = addrs; addr != nullptr; addr = addr->ai_next) {
        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (fd == -1) {
            continue;
        }

        int optval = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
            std::cerr << "WARNING: problem setting SO_REUSEADDR: " << strerror(errno) << std::endl;
            abort();
        }

        if (bind(fd, addr->ai_addr, addr->ai_addrlen) == -1) {
            int bind_errno = errno;
            IOUtils::Close(fd);
            throw IOException("Problem binding socket: " + std::string(strerror(bind_errno)));
        }

        return fd;
    }

    throw IOException("Unable to create socket: " + std::string(strerror(errno)));
}


int IOUtils::ListenSocket(SocketAddress const& address, bool use_ipv4, bool use_ipv6, int backlog) {
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
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    char bind_port_str[16];
    sprintf(bind_port_str, "%d", address.Port());

    struct addrinfo* addrs;
    int err = getaddrinfo(address.Address().c_str(), bind_port_str, &hints, &addrs);
    if (err != 0) {
        throw DnsResolutionException("getaddrinfo: " + std::string(gai_strerror(err)));
    }

    int fd;
    try {
        fd = OpenAndBindSocket(addrs);
    } catch (...) {
        freeaddrinfo(addrs);
        throw;
    }
    freeaddrinfo(addrs);

    if (listen(fd, backlog) != 0) {
        Close(fd);
        throw IOException("Problem listening to socket: " + std::string(strerror(errno)));
    }

    return fd;
}


IOUtils::AutoCloseableFD::AutoCloseableFD(int fd) noexcept : fd(fd) {
}


IOUtils::AutoCloseableFD::~AutoCloseableFD(void) noexcept {
    if (fd != -1) {
        Close(fd);
    }
}


IOUtils::AutoCloseableFD::AutoCloseableFD(IOUtils::AutoCloseableFD&& other) noexcept : fd(other.fd) {
    other.fd = -1;
}


IOUtils::AutoCloseableFD& IOUtils::AutoCloseableFD::operator=(IOUtils::AutoCloseableFD&& other) noexcept {
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
