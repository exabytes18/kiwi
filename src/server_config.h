#ifndef KIWI_SERVER_CONFIG_H_
#define KIWI_SERVER_CONFIG_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include "status.h"

using namespace std;

class ServerConfig {
public:
    ServerConfig();
    string const ClusterName() const;
    uint32_t ServerId() const;
    unordered_map<uint32_t, string> Hosts();
    string DataDir() const;
    Status PopulateFromFile(char const* config_path);

private:
    // strings use dynamic memory internally, so this is fine
    // (this->cluster_name isn't constrainted or anything).
    string cluster_name;
    uint32_t server_id;
    unordered_map<uint32_t, string> hosts;
    string data_dir;
};

#endif  // KIWI_SERVER_CONFIG_H_
