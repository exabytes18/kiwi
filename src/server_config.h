#ifndef KIWI_SERVER_CONFIG_H_
#define KIWI_SERVER_CONFIG_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include "socket_address.h"

using namespace std;

class ServerConfig {
public:
    ServerConfig(string const& cluster_name, uint32_t server_id, SocketAddress const& bind_address, unordered_map<uint32_t, SocketAddress> const& hosts, string const& data_dir, bool use_ipv4, bool use_ipv6);
    static ServerConfig ParseFromFile(char const* config_path);
    string const& ClusterName(void) const;
    uint32_t ServerId(void) const;
    SocketAddress BindAddress(void) const;
    unordered_map<uint32_t, SocketAddress> const& Hosts(void) const;
    string const& DataDir(void) const;
    bool UseIPV4(void) const;
    bool UseIPV6(void) const;

private:
    string cluster_name;
    uint32_t server_id;
    SocketAddress bind_address;
    unordered_map<uint32_t, SocketAddress> hosts;
    string data_dir;
    bool use_ipv4;
    bool use_ipv6;
};

#endif  // KIWI_SERVER_CONFIG_H_
