#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "buffered_socket.h"
#include "exceptions.h"
#include "io_utils.h"


static int OpenSocket(int domain, int type, int protocol) {
    int fd = socket(domain, type, protocol);
    if (fd == -1) {
        throw IOException("Problem opening socket: " + std::string(strerror(errno)));
    }

    return fd;
}


BufferedSocket::BufferedSocket(int domain, int type, int protocol) :
        BufferedSocket(OpenSocket(domain, type, protocol)) {}


BufferedSocket::BufferedSocket(int fd) noexcept :
        fd(fd),
        read_buffer(64 * 1024),
        write_buffer(64 * 1024),
        flushing_in_progress(false) {
    read_buffer.Flip();
}


BufferedSocket::~BufferedSocket(void) noexcept {
    if (fd != -1) {
        IOUtils::Close(fd);
    }
}


BufferedSocket::BufferedSocket(BufferedSocket&& other) noexcept :
        fd(other.fd),
        read_buffer(other.read_buffer),
        write_buffer(other.write_buffer),
        flushing_in_progress(other.flushing_in_progress) {
    other.fd = -1;
}


BufferedSocket& BufferedSocket::operator=(BufferedSocket&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (fd != -1) {
        IOUtils::Close(fd);
    }

    fd = other.fd;
    read_buffer = other.read_buffer;
    write_buffer = other.write_buffer;
    flushing_in_progress = other.flushing_in_progress;

    other.fd = -1;
    return *this;
}


int BufferedSocket::GetFD(void) noexcept {
    return fd;
}


void BufferedSocket::SetNonBlocking(bool nonblocking) noexcept {
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


void BufferedSocket::SetReuseAddr(bool reuse_addr) noexcept {
    int optval = (reuse_addr) ? (1) : (0);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
        std::cerr << "WARNING: problem setting SO_REUSEADDR: " << strerror(errno) << std::endl;
        abort();
    }
}


int BufferedSocket::Bind(struct sockaddr const* addr, socklen_t addrlen) noexcept {
    return bind(fd, addr, addrlen);
}


int BufferedSocket::Connect(struct sockaddr const* addr, socklen_t addrlen) noexcept {
    return connect(fd, addr, addrlen);
}


int BufferedSocket::GetErrorCode(void) noexcept {
    int optval;
    socklen_t optsize;
    int err = getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optsize);
    if (err == -1) {
        std::cerr << "WARNING: problem getting socket error status: " << strerror(errno) << std::endl;
        abort();
    }

    return optval;
}


BufferedSocket::RecvStatus BufferedSocket::Fill(Buffer& buffer) {
    while (buffer.Remaining() > 0) {
        buffer.FillFrom(read_buffer);
        if (read_buffer.Remaining() == 0) {
            auto data = read_buffer.Data();
            auto capacity = read_buffer.Capacity();
            auto bytes_read = recv(fd, data, capacity, 0);
            if (bytes_read == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                } else if (errno == ECONNREFUSED || errno == ECONNRESET) {
                    return RecvStatus::closed;
                } else {
                    throw IOException("Problem reading data from socket: " + std::string(strerror(errno)));
                }
            } else if (bytes_read == 0) {
                return RecvStatus::closed;
            } else {
                read_buffer.Position(0);
                read_buffer.Limit(bytes_read);
            }
        }
    }

    if (buffer.Remaining() == 0) {
        return RecvStatus::complete;
    } else {
        return RecvStatus::incomplete;
    }
}


BufferedSocket::SendStatus BufferedSocket::Write(Buffer& buffer) {
    if (flushing_in_progress) {
        SendStatus status = Flush();
        if (status != SendStatus::complete) {
            return status;
        }
    }

    while (buffer.Remaining() > 0) {
        write_buffer.FillFrom(buffer);
        if (write_buffer.Remaining() == 0) {
            SendStatus status = Flush();
            if (status != SendStatus::complete) {
                return status;
            }
        }
    }

    if (buffer.Remaining() == 0) {
        return SendStatus::complete;
    } else {
        return SendStatus::incomplete;
    }
}


BufferedSocket::SendStatus BufferedSocket::Flush(void) {
    if (!flushing_in_progress) {
        write_buffer.Flip();
        flushing_in_progress = true;
    }

    while (write_buffer.Remaining() > 0) {
        auto data  = write_buffer.Data();
        auto position = write_buffer.Position();
        auto remaining = write_buffer.Remaining();
        auto bytes_written = send(fd, data + position, remaining, 0);
        if (bytes_written == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else if (errno == ECONNREFUSED || errno == ECONNRESET) {
                return SendStatus::closed;
            } else {
                throw IOException("Problem reading data from socket: " + std::string(strerror(errno)));
            }
        } else {
            write_buffer.Position(position + bytes_written);
        }
    }

    if (write_buffer.Remaining() == 0) {
        write_buffer.Clear();
        flushing_in_progress = false;
        return SendStatus::complete;
    } else {
        return SendStatus::incomplete;
    }
}
