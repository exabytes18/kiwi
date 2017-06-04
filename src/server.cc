#include "server.h"

Server::Server(ServerConfig const& server_config) : storage(server_config) {
}


Status Server::Start(void) {
    Status s = storage.Open();
    if (!s.ok()) {
        return s;
    }

    return Status();
}


void Server::Shutdown(void) {
}
