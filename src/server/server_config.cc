#include "common/constants.h"
#include "common/exceptions.h"
#include "common/file_utils.h"
#include "yaml-cpp/yaml.h"
#include "yaml-cpp/exceptions.h"
#include "server_config.h"


using namespace std;

template <class T>
static T ParseValue(char const* config_path, string const& name, YAML::Node& value) {
    try {
        return value.as<T>();
    } catch (YAML::BadConversion& e) {
        stringstream ss;
        ss << "Error parsing \"" << name << "\" @ ";
        ss << "file " << config_path << ", ";
        ss << "line " << e.mark.line << ", ";
        ss << "column " << e.mark.column << ".";
        throw ConfigurationException(ss.str());
    }
}


template <class T>
static T ParseRequiredParameter(char const* config_path, YAML::Node const& yaml, string const& name) {
    auto value = yaml[name];
    if (value) {
        return ParseValue<T>(config_path, name, value);
    } else {
        stringstream ss;
        ss << "You must specify \"" << name << "\" in " << config_path << ".";
        throw ConfigurationException(ss.str());
    }
}


template <class T>
static T ParseOptionalParameter(char const* config_path, YAML::Node const& yaml, string const& name, T defaultValue) {
    auto value = yaml[name];
    if (value) {
        return ParseValue<T>(config_path, name, value);
    } else {
        return defaultValue;
    }
}


static unordered_map<uint32_t, SocketAddress> ParseHostMap(char const* config_path, YAML::Node const& yaml, string const& name) {
    auto hosts = yaml["hosts"];
    if (!hosts) {
        stringstream ss;
        ss << "You must specify \"" << name << "\" in " << config_path << ".";
        throw ConfigurationException(ss.str());
    }

    if (!hosts.IsMap()) {
        stringstream ss;
        ss << "The \"" << name << "\" configuration option in \"" << config_path << "\" must be a map.";
        throw ConfigurationException(ss.str());
    }

    try {
        unordered_map<uint32_t, SocketAddress> result;
        for (YAML::const_iterator it = hosts.begin(); it != hosts.end(); ++it) {
            uint32_t key = it->first.as<uint32_t>();
            string value = it->second.as<string>();
            result.insert(make_pair(key, SocketAddress::FromString(value, Constants::DEFAULT_PORT)));
        }
        return result;

    } catch (YAML::BadConversion& e) {
        stringstream ss;
        ss << "Error parsing \"" << name << "\" @ ";
        ss << "file " << config_path << ", ";
        ss << "line " << e.mark.line << ", ";
        ss << "column " << e.mark.column << ".";
        throw ConfigurationException(ss.str());
    }
}


ServerConfig ServerConfig::ParseFromFile(char const* config_path) {
    auto contents = FileUtils::ReadFile(config_path);
    auto yaml = YAML::Load(contents);

    auto cluster_name = ParseRequiredParameter<string>(config_path, yaml, "cluster_name");
    if (cluster_name.length() > Constants::MAX_CLUSTER_NAME_LENGTH) {
        stringstream ss;
        ss << "The \"cluster_name\" configuration parameter must be <= " << Constants::MAX_CLUSTER_NAME_LENGTH << " bytes long.";
        throw ConfigurationException(ss.str());
    }

    auto server_id = ParseRequiredParameter<uint32_t>(config_path, yaml, "server_id");
    auto hosts = ParseHostMap(config_path, yaml, "hosts");
    if (hosts.find(server_id) == hosts.end()) {
        stringstream ss;
        ss << "The \"hosts\" configuration parameter must contain an entry matching the \"server_id\" (\"" << server_id << "\").";
        throw ConfigurationException(ss.str());
    }
    
    auto data_dir = ParseRequiredParameter<string>(config_path, yaml, "data_dir");
    auto use_ipv4 = ParseOptionalParameter<bool>(config_path, yaml, "ipv4", false);
    auto use_ipv6 = ParseOptionalParameter<bool>(config_path, yaml, "ipv6", false);
    auto bind_address = ParseRequiredParameter<string>(config_path, yaml, "bind_address");
    auto socket_address = SocketAddress::FromString(bind_address, Constants::DEFAULT_PORT);

    return ServerConfig(cluster_name, server_id, socket_address, hosts, data_dir, use_ipv4, use_ipv6);
}


ServerConfig::ServerConfig(string const& cluster_name, uint32_t server_id, SocketAddress const& bind_address, unordered_map<uint32_t, SocketAddress> const& hosts, string const& data_dir, bool use_ipv4, bool use_ipv6) :
        cluster_name(cluster_name),
        server_id(server_id),
        bind_address(bind_address),
        hosts(hosts),
        data_dir(data_dir),
        use_ipv4(use_ipv4),
        use_ipv6(use_ipv6) {}


string const& ServerConfig::ClusterName(void) const {
    return cluster_name;
}


uint32_t ServerConfig::ServerId(void) const {
    return server_id;
}


SocketAddress ServerConfig::BindAddress(void) const {
    return bind_address;
}


unordered_map<uint32_t, SocketAddress> const& ServerConfig::Hosts(void) const {
    return hosts;
}


string const& ServerConfig::DataDir(void) const {
    return data_dir;
}

bool ServerConfig::UseIPV4(void) const {
    return use_ipv4;
}


bool ServerConfig::UseIPV6(void) const {
    return use_ipv6;
}
