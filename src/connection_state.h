#ifndef KIWI_CONNECTION_STATE_H_
#define KIWI_CONNECTION_STATE_H_

#include <cstdlib>


class ConnectionState {
public:
    ConnectionState(void);
    size_t position;
    size_t limit;
    char buf[64*1024];
};

#endif  // KIWI_CONNECTION_STATE_H_
