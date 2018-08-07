#include <errno.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>

#include "buffered_socket.h"
#include "exceptions.h"
#include "io_utils.h"


using namespace std;

BufferedSocket::BufferedSocket(int domain, int type, int protocol) :
        BufferedSocket(IOUtils::OpenSocketFD(domain, type, protocol)) {}


BufferedSocket::BufferedSocket(int fd) noexcept :
        AbstractSocket(fd),
        read_buffer(64 * 1024),
        write_buffer(64 * 1024),
        flushing_in_progress(false) {
    read_buffer.Flip();
}


BufferedSocket::BufferedSocket(BufferedSocket&& other) noexcept :
        AbstractSocket(other.fd),
        read_buffer(move(other.read_buffer)),
        write_buffer(move(other.write_buffer)),
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
    read_buffer = move(other.read_buffer);
    write_buffer = move(other.write_buffer);
    flushing_in_progress = other.flushing_in_progress;

    other.fd = -1;
    return *this;
}


BufferedSocket::~BufferedSocket(void) noexcept {
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
                    throw IOException("Problem reading data from socket: " + string(strerror(errno)));
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
                throw IOException("Problem reading data from socket: " + string(strerror(errno)));
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
