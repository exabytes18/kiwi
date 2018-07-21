#ifndef KIWI_EXCEPTIONS_H_
#define KIWI_EXCEPTIONS_H_

#include <stdexcept>
#include <string>


class IOException : public std::runtime_error {
public:
    IOException(std::string const& msg) : runtime_error(msg) {}
};

class ConnectionClosedException : public IOException {
public:
    ConnectionClosedException(std::string const& msg) : IOException(msg) {}
};

class DnsResolutionException : public IOException {
public:
    DnsResolutionException(std::string const& msg) : IOException(msg) {}
};

class FileNotFoundException : public IOException {
public:
    FileNotFoundException(std::string const& msg) : IOException(msg) {}
};

class ConfigurationException : public std::runtime_error {
public:
    ConfigurationException(std::string const& msg) : runtime_error(msg) {}
};

class ServerException : public std::runtime_error {
public:
    ServerException(std::string const& msg) : runtime_error(msg) {}
};

class ServerConfigurationException : public std::runtime_error {
public:
    ServerConfigurationException(std::string const& msg) : runtime_error(msg) {}
};

class StorageException : public std::runtime_error {
public:
    StorageException(std::string const& msg) : runtime_error(msg) {}
};

#endif  // KIWI_EXCEPTIONS_H_
