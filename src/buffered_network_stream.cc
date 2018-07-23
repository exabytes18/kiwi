#include <errno.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include "buffered_network_stream.h"
#include "exceptions.h"
#include "io_utils.h"


using namespace std;

BufferedNetworkStream::BufferedNetworkStream(int fd) :
        fd(fd),
        read_buffer(64 * 1024),
        write_buffer(64 * 1024),
        flushing_in_progress(false) {
    read_buffer.Flip();
}


BufferedNetworkStream::~BufferedNetworkStream(void) {
    // TODO: verify that the read_buffer gets deleted automatically
    IOUtils::Close(fd);
}


int BufferedNetworkStream::GetFD(void) {
    return fd;
}


BufferedNetworkStream::Status BufferedNetworkStream::Fill(Buffer& buffer) {
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
                    return Status::closed;
                } else {
                    throw IOException("Problem reading data from socket: " + string(strerror(errno)));
                }
            } else if (bytes_read == 0) {
                return Status::closed;
            } else {
                read_buffer.Position(0);
                read_buffer.Limit(bytes_read);
            }
        }
    }

    if (buffer.Remaining() == 0) {
        return Status::complete;
    } else {
        return Status::incomplete;
    }
}


BufferedNetworkStream::Status BufferedNetworkStream::Write(Buffer& buffer) {
    if (flushing_in_progress) {
        return Status::incomplete;
    } else {
        while (buffer.Remaining() > 0) {
            write_buffer.FillFrom(buffer);
            if (write_buffer.Remaining() == 0) {
                Status status = Flush();
                // TODO: I don't like this logic.. could break too easily if we add more enum values or something.
                if (status == Status::closed || status == Status::incomplete) {
                    return status;
                }
            }
        }

        if (buffer.Remaining() == 0) {
            return Status::complete;
        } else {
            return Status::incomplete;
        }
    }
}


BufferedNetworkStream::Status BufferedNetworkStream::Flush(void) {
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
                return Status::closed;
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
        return Status::complete;
    } else {
        return Status::incomplete;
    }
}