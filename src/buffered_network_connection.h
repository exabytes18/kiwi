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
    bool Fill(Buffer* buffer);

private:
    int fd;
    Buffer read_buffer;
};

#endif  // KIWI_BUFFERED_NETWORK_CONNECTION_H_
