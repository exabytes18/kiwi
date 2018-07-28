#ifndef KIWI_PROTOCOL_H_
#define KIWI_PROTOCOL_H_

#include <sstream>
#include <string>


namespace Protocol {
    const uint32_t MAGIC_NUMBER = 0xE6955EBF;
    const uint32_t PROTOCOL_VERSION = 1;

    enum MessageType {
        UNDEFINED =              0x00000000,
        ERROR =                  0x00000001,

        CLIENT_HELLO =           0x40000000,
        CLIENT_HELLO_REPLY =     0x40000001,

        SERVER_HELLO =           0x80000000,
        SERVER_HELLO_REPLY =     0x80000001,
    };

    enum ErrorCode {
        OK = 0,
        INVALID_MAGIC_NUMBER = 1,
        UNSUPPORTED_PROTOCOL_VERSION = 2,
    };

    std::string InvalidMagicNumberErrorMessage(uint32_t magic_number);
    std::string UnsupportedProtocolVersionErrorMessage(uint32_t protocol_version);
}

#endif  // KIWI_PROTOCOL_H_
