#ifndef KIWI_SERVER_CONFIG_H_
#define KIWI_SERVER_CONFIG_H_

#include <cstdint>
#include <string>
#include <unordered_map>

using namespace std;

class ServerConfig {
public:
    ServerConfig(string const& cluster_name, uint32_t server_id, unordered_map<uint32_t, string> const& hosts, string const& data_dir);
    static ServerConfig ParseFromFile(char const * config_path);
    string const& ClusterName() const;
    uint32_t ServerId() const;
    unordered_map<uint32_t, string> const& Hosts() const;
    string const& DataDir() const;

private:
    string cluster_name;
    uint32_t server_id;
    unordered_map<uint32_t, string> hosts;
    string data_dir;
};

#endif  // KIWI_SERVER_CONFIG_H_
