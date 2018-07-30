#ifndef KIWI_SERVER_CONFIG_H_
#define KIWI_SERVER_CONFIG_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include "common/socket_address.h"


class ServerConfig {
public:
    ServerConfig(std::string const& cluster_name, uint32_t server_id, SocketAddress const& bind_address, std::unordered_map<uint32_t, SocketAddress> const& hosts, std::string const& data_dir, bool use_ipv4, bool use_ipv6);
    static ServerConfig ParseFromFile(char const* config_path);
    std::string const& ClusterName(void) const;
    uint32_t ServerId(void) const;
    SocketAddress BindAddress(void) const;
    std::unordered_map<uint32_t, SocketAddress> const& Hosts(void) const;
    std::string const& DataDir(void) const;
    bool UseIPV4(void) const;
    bool UseIPV6(void) const;

private:
    std::string cluster_name;
    uint32_t server_id;
    SocketAddress bind_address;
    std::unordered_map<uint32_t, SocketAddress> hosts;
    std::string data_dir;
    bool use_ipv4;
    bool use_ipv6;
};

#endif  // KIWI_SERVER_CONFIG_H_
