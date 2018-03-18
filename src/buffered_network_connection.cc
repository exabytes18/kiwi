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
        read_buffer(32*1024) {
    read_buffer.Flip();
}


BufferedNetworkConnection::~BufferedNetworkConnection(void) {
    // TODO: verify that the read_buffer gets deleted automatically
    IOUtils::Close(fd);
}


int BufferedNetworkConnection::GetFD(void) {
    return fd;
}


bool BufferedNetworkConnection::Fill(Buffer* buffer) {
    while (buffer->Remaining() > 0) {
        buffer->Put(read_buffer);
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
    return buffer->Remaining() == 0;
}
