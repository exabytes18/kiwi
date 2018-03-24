#ifndef KIWI_BUFFERED_NETWORK_CONNECTION_H_
#define KIWI_BUFFERED_NETWORK_CONNECTION_H_

#include <stddef.h>
#include "buffer.h"


class BufferedNetworkConnection {
public:
    BufferedNetworkConnection(int fd);
    ~BufferedNetworkConnection(void);
    int GetFD(void);

    /*
     * Return true if we had enough bytes already or we could read enough
     * from the socket to fill the specified buffer; false otherwise (need
     * to wait for socket to be readable again before trying to fill the
     * buffer again).
     */
    bool Fill(Buffer& buffer);

    /*
     * Return true if there was enough buffer space to fully consume the
     * provided buffer; false otherwise (need to try the call again; note:
     * some of the buffer may have already been consumed).
     */
    bool Write(Buffer& buffer);

    /*
     * Returns true if the entire write buffer has been flushed to the
     * network stack; false otherwise (you'll need to wait for the socket to
     * be writable again before trying to flush the data again).
     */
    bool Flush(void);

private:
    int fd;
    Buffer read_buffer;
    Buffer write_buffer;
    bool flushing_in_progress;
};

#endif  // KIWI_BUFFERED_NETWORK_CONNECTION_H_
