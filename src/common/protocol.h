#ifndef KIWI_PROTOCOL_H_
#define KIWI_PROTOCOL_H_

#include <sstream>
#include <string>


namespace Protocol {
    const uint32_t MAGIC_NUMBER = 0xE6955EBF;
    const uint32_t PROTOCOL_VERSION = 1;

    enum MessageType {
        UNDEFINED =              0x00000000,
        ERROR_REPLY =            0x00000001,

        CLIENT_HELLO =           0x40000000,
        CLIENT_HELLO_REPLY =     0x40000001,

        CLIENT_TEST =            0x40000002,
        CLIENT_TEST_REPLY =      0x40000003,

        SERVER_HELLO =           0x80000000,
        SERVER_HELLO_REPLY =     0x80000001,
    };

    enum ErrorCode {
        OK = 0,
        INVALID_MAGIC_NUMBER = 1,
        UNSUPPORTED_PROTOCOL_VERSION = 2,
        CLUSTER_NAME_MISMATCH = 3,
    };

    std::string InvalidMagicNumberErrorMessage(uint32_t invalid_magic_number);
    std::string UnsupportedProtocolVersionErrorMessage(uint32_t invalid_protocol_version);
    std::string ClusterNameMismatchErrorMessage(void);
}

#endif  // KIWI_PROTOCOL_H_
