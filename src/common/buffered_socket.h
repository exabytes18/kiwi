#ifndef KIWI_BUFFERED_SOCKET_H_
#define KIWI_BUFFERED_SOCKET_H_

#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

#include "buffer.h"
#include "socket.h"


class BufferedSocket : public Socket {
public:
    enum class RecvStatus { complete, incomplete, closed };
    enum class SendStatus { complete, incomplete, closed };

    BufferedSocket(int domain, int type, int protocol);
    BufferedSocket(int fd) noexcept;
    ~BufferedSocket(void) noexcept;

    /*
     * Return `complete` if we had enough bytes already or we could read
     * enough from the socket to fill the specified buffer, `incomplete` if we
     * need to wait for socket to be readable again before trying to fill the
     * remainder of the buffer), and `closed` if the stream was closed before
     * the buffer could be fully populated.
     */
    RecvStatus Fill(Buffer& buffer);

    /*
     * Return `complete` if there was enough buffer space to fully consume the
     * provided buffer, `incomplete` if need to try the call again (note:
     * some of the buffer may have already been consumed), and `closed` if the
     * stream was closed before all of the buffer could be written out to the
     * stream.
     */
    SendStatus Write(Buffer& buffer);

    /*
     * Returns `complete` if the entire write buffer has been flushed to the
     * network stack, `incomplete` if we need to wait for the socket to be
     * writable again before trying to flush the remaining buffered data, and
     * `closed` if the stream was closed before all the unflushed data could
     * be written out to the stream.
     */
    SendStatus Flush(void);

    // Move constructor + move assignment operator
    BufferedSocket(BufferedSocket&& other) noexcept;
    BufferedSocket& operator=(BufferedSocket&& other) noexcept;

    // Delete copy constructor and copy assignment operator
    BufferedSocket(BufferedSocket const& other) = delete;
    BufferedSocket& operator=(BufferedSocket const& other) = delete;

private:
    Buffer read_buffer;
    Buffer write_buffer;
    bool flushing_in_progress;
};

#endif  // KIWI_BUFFERED_SOCKET_H_
