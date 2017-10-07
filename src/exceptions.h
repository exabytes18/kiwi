#ifndef KIWI_EXCEPTIONS_H_
#define KIWI_EXCEPTIONS_H_

#include <string>

using namespace std;

class IOException : public runtime_error {
public:
    IOException(string const& msg) : runtime_error(msg) {}
};

class FileNotFoundException : public IOException {
public:
    FileNotFoundException(string const& msg) : IOException(msg) {}
};

class ConfigurationException : public runtime_error {
public:
    ConfigurationException(string const& msg) : runtime_error(msg) {}
};

class ServerException : public runtime_error {
public:
    ServerException(string const& msg) : runtime_error(msg) {}
};

class ServerConfigurationException : public runtime_error {
public:
    ServerConfigurationException(string const& msg) : runtime_error(msg) {}
};

class StorageException : public runtime_error {
public:
    StorageException(string const& msg) : runtime_error(msg) {}
};

#endif  // KIWI_EXCEPTIONS_H_
