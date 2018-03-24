#include <errno.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include "buffered_network_connection.h"
#include "exceptions.h"
#include "io_utils.h"

using namespace std;


BufferedNetworkConnection::BufferedNetworkConnection(int fd) :
        fd(fd),
        read_buffer(32*1024),
        write_buffer(32*1024),
        flushing_in_progress(false) {
    IOUtils::SetNonBlocking(fd);
    read_buffer.Flip();
}


BufferedNetworkConnection::~BufferedNetworkConnection(void) {
    // TODO: verify that the read_buffer gets deleted automatically
    IOUtils::Close(fd);
}


int BufferedNetworkConnection::GetFD(void) {
    return fd;
}


bool BufferedNetworkConnection::Fill(Buffer& buffer) {
    while (buffer.Remaining() > 0) {
        buffer.Put(read_buffer);
        if (read_buffer.Remaining() == 0) {
            auto data = read_buffer.Data();
            auto capacity = read_buffer.Capacity();
            auto bytes_read = recv(fd, data, capacity, 0);
            if (bytes_read == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                } else if (errno == ECONNREFUSED) {
                    throw ConnectionClosedException("Connection closed: " + string(strerror(errno)));
                } else {
                    throw IOException("Problem reading data from socket: " + string(strerror(errno)));
                }
            } else if (bytes_read == 0) {
                throw ConnectionClosedException("Connection closed");
            } else {
                read_buffer.Position(0);
                read_buffer.Limit(bytes_read);
            }
        }
    }
    return buffer.Remaining() == 0;
}


bool BufferedNetworkConnection::Write(Buffer& buffer) {
    if (flushing_in_progress) {
        return false;
    } else {
        while (buffer.Remaining() > 0) {
            write_buffer.Put(buffer);
            if (write_buffer.Remaining() == 0) {
                if (!Flush()) {
                    break;
                }
            }
        }
        return buffer.Remaining() == 0;
    }
}


bool BufferedNetworkConnection::Flush(void) {
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
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS/*unsure if needed*/) {
                break;
            } else if (errno == ECONNRESET) {
                throw ConnectionClosedException("Connection closed: " + string(strerror(errno)));
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
        return true;
    } else {
        return false;
    }
}
