#ifndef KIWI_PROTOCOL_H_
#define KIWI_PROTOCOL_H_


namespace Protocol {
    const uint32_t MAGIC_NUMBER = 0xE6955EBF;
    const uint32_t PROTOCOL_VERSION = 1;

    enum MessageType {
        UNDEFINED = 0,
        CLIENT_HELLO = 1,
        CLIENT_HELLO_REPLY = 2,
        SERVER_HELLO = 3,
        SEVER_HELLO_REPLY = 4,
    };
}

#endif  // KIWI_PROTOCOL_H_
