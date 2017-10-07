#include "exceptions.h"
#include "file_utils.h"
#include "server_config.h"
#include "yaml-cpp/yaml.h"
#include "yaml-cpp/exceptions.h"


template <class T>
static T Parse(char const * config_path, YAML::Node const& yaml, string const& name) {
    auto value = yaml[name];
    if (value) {
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

    } else {
        stringstream ss;
        ss << "You must specify \"" << name << "\" in " << config_path << ".";
        throw ConfigurationException(ss.str());
    }
}


static unordered_map<uint32_t, string> ParseHostMap(char const * config_path, YAML::Node const& yaml, string const& name) {
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
        unordered_map<uint32_t, string> result;
        for (YAML::const_iterator it = hosts.begin(); it != hosts.end(); ++it) {
            uint32_t key = it->first.as<uint32_t>();
            string value = it->second.as<string>();
            result[key] = value;
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


ServerConfig ServerConfig::ParseFromFile(char const * config_path) {
    string contents = FileUtils::ReadFile(config_path);
    YAML::Node yaml = YAML::Load(contents);

    string cluster_name = Parse<string>(config_path, yaml, "cluster_name");
    uint32_t server_id = Parse<uint32_t>(config_path, yaml, "server_id");
    unordered_map<uint32_t, string> hosts = ParseHostMap(config_path, yaml, "hosts");
    string data_dir = Parse<string>(config_path, yaml, "data_dir");

    return ServerConfig(cluster_name, server_id, hosts, data_dir);
}


ServerConfig::ServerConfig(string const& cluster_name, uint32_t server_id, unordered_map<uint32_t, string> const& hosts, string const& data_dir) :
        cluster_name(cluster_name),
        server_id(server_id),
        hosts(hosts),
        data_dir(data_dir) {}


string const& ServerConfig::ClusterName(void) const {
    return cluster_name;
}


uint32_t ServerConfig::ServerId(void) const {
    return server_id;
}


unordered_map<uint32_t, string> const& ServerConfig::Hosts(void) const {
    return hosts;
}


string const& ServerConfig::DataDir(void) const {
    return data_dir;
}
