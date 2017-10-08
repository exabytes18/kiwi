#include "socket_address.h"
#include "string_utils.h"

SocketAddress SocketAddress::FromString(std::string const& input, int defaultPort) {
    auto colon_idx = input.find(":");
    if (colon_idx == std::string::npos) {
        return SocketAddress(input, defaultPort);
    } else {
        auto addr = input.substr(0, colon_idx);
        auto port_str = input.substr(colon_idx + 1);
        auto port = StringUtils::ParseInt(port_str);
        return SocketAddress(addr, port);
    }
}


SocketAddress::SocketAddress(std::string const& address, int port) :
        address(address),
        port(port) {}


std::string SocketAddress::Address() const {
    return address;
}


int SocketAddress::Port() const {
    return port;
}
