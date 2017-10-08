#ifndef KIWI_SOCKET_ADDRESS_H_
#define KIWI_SOCKET_ADDRESS_H_

#include <string>


class SocketAddress {
public:
    static SocketAddress FromString(std::string const& input, int defaultPort);
    SocketAddress(std::string const& address, int port);
    std::string Address() const;
    int Port() const;

private:
    std::string address;
    int port;
};

#endif  // KIWI_SOCKET_ADDRESS_H_
