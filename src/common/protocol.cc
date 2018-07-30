#include <sstream>
#include "protocol.h"


using namespace std;

string Protocol::InvalidMagicNumberErrorMessage(uint32_t invalid_magic_number) {
    stringstream ss;
    ss << "Server received " << invalid_magic_number << " as the magic number; ";
    ss << "expected " << Protocol::MAGIC_NUMBER << ".";
    return ss.str();
}


string Protocol::UnsupportedProtocolVersionErrorMessage(uint32_t invalid_protocol_version) {
    stringstream ss;
    ss << "Server received protocol version " << invalid_protocol_version << " ";
    ss << "but this server only supports the following protocol versions: [" << PROTOCOL_VERSION << "].";
    return ss.str();
}


string Protocol::ClusterNameMismatchErrorMessage(void) {
    stringstream ss;
    ss << "Cluster name mismatch.";
    return ss.str();
}
