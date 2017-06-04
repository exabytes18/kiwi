#ifndef KIWI_SERVER_H_
#define KIWI_SERVER_H_

#include "server_config.h"
#include "status.h"
#include "storage.h"

class Server {
public:
    Server(ServerConfig const& server_config);
    Status Start(void);
    void Shutdown(void);

private:
    Storage storage;
};

#endif  // KIWI_SERVER_H_
