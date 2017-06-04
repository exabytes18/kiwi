#include "file_utils.h"
#include "server_config.h"
#include "yaml-cpp/yaml.h"
#include "yaml-cpp/exceptions.h"

ServerConfig::ServerConfig() :
    cluster_name(""),
    server_id(1),
    hosts({{1,"127.0.0.1"}}),
    data_dir("kiwidb") {}


string const ServerConfig::ClusterName() const {
    return cluster_name;
}


uint32_t ServerConfig::ServerId() const {
    return server_id;
}


unordered_map<uint32_t, string> ServerConfig::Hosts() {
    return hosts;
}


string ServerConfig::DataDir() const {
    return data_dir;
}


template <class T>
static Status Parse(YAML::Node const& yaml, string const& name, bool required, T& result) {
    auto value = yaml[name];
    if (value) {
        try {
            result = value.as<T>();
            return Status();
        
        } catch (YAML::BadConversion& e) {
            stringstream ss;
            ss << "Error parsing \"" << name << "\" @ ";
            ss << "line " << e.mark.line << ", column " << e.mark.column << ".";
            return Status::ParseError(ss.str());
        }
    
    } else {
        if (required) {
            stringstream ss;
            ss << "You must specify \"" << name << "\".";
            return Status::ConfigurationError(ss.str());
        } else {
            return Status();
        }
    }
}


static Status ParseHostMap(YAML::Node const& yaml, string const& name, bool required, unordered_map<uint32_t, string>& result) {
    auto hosts = yaml["hosts"];
    if (!hosts) {
        if (required) {
            stringstream ss;
            ss << "You must specify \"" << name << "\".";
            return Status::ConfigurationError(ss.str());
        } else {
            return Status();
        }
    }

    if (!hosts.IsMap()) {
        stringstream ss;
        ss << "The \"" << name << "\" configuration option must be a map.";
        return Status::ConfigurationError(ss.str());
    }

    try {
        for (YAML::const_iterator it = hosts.begin(); it != hosts.end(); ++it) {
            uint32_t key = it->first.as<uint32_t>();
            string value = it->second.as<string>();
            result[key] = value;
        }
        return Status();
    
    } catch (YAML::BadConversion& e) {
        stringstream ss;
        ss << "Error parsing \"" << name << "\" @ ";
        ss << "line " << e.mark.line << ", column " << e.mark.column << ".";
        return Status::ParseError(ss.str());
    }
}


Status ServerConfig::PopulateFromFile(char const* config_path) {
    Status s;

    string contents;
    s = FileUtils::ReadFile(config_path, contents);
    if (!s.ok()) {
        return s;
    }

    YAML::Node yaml = YAML::Load(contents);

    s = Parse<string>(yaml, "cluster_name", true, this->cluster_name);
    if (!s.ok()) {
        return s;
    }

    s = Parse<uint32_t>(yaml, "server_id", true, this->server_id);
    if (!s.ok()) {
        return s;
    }

    s = ParseHostMap(yaml, "hosts", true, this->hosts);
    if (!s.ok()) {
        return s;
    }

    s = Parse<string>(yaml, "data_dir", true, this->data_dir);
    if (!s.ok()) {
        return s;
    }

    return s;
}
