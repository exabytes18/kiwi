#ifndef KIWI_BUFFERED_NETWORK_STREAM_H_
#define KIWI_BUFFERED_NETWORK_STREAM_H_

#include <stddef.h>
#include "buffer.h"
#include "io_utils.h"


class BufferedNetworkStream {
public:
    enum class Status { complete, incomplete, closed };

    BufferedNetworkStream(IOUtils::AutoCloseableSocket socket);
    ~BufferedNetworkStream(void);
    int GetFD(void);

    /*
     * Return `complete` if we had enough bytes already or we could read
     * enough from the socket to fill the specified buffer, `incomplete` if we
     * need to wait for socket to be readable again before trying to fill the
     * remainder of the buffer), and `closed` if the stream was closed before
     * the buffer could be fully populated.
     */
    Status Fill(Buffer* buffer);

    /*
     * Return `complete` if there was enough buffer space to fully consume the
     * provided buffer, `incomplete` if need to try the call again (note:
     * some of the buffer may have already been consumed), and `closed` if the
     * stream was closed before all of the buffer could be written out to the
     * stream.
     */
    Status Write(Buffer* buffer);

    /*
     * Returns `complete` if the entire write buffer has been flushed to the
     * network stack, `incomplete` if we need to wait for the socket to be
     * writable again before trying to flush the remaining buffered data, and
     * `closed` if the stream was closed before all the unflushed data could
     * be written out to the stream.
     */
    Status Flush(void);

private:
    IOUtils::AutoCloseableSocket socket;
    Buffer read_buffer;
    Buffer write_buffer;
    bool flushing_in_progress;
};

#endif  // KIWI_BUFFERED_NETWORK_STREAM_H_
