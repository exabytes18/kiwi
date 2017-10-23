#ifndef KIWI_EVENTS_H_
#define KIWI_EVENTS_H_

#include <cstdint>
#include <string>


class CreateRowEvent {
private:
    uint32_t database;
    uint32_t table;
    std::string key;
    std::string value;
};

class DeleteRowEvent {
    std::string database;
    std::string table;
    std::string key;
};

#endif  // KIWI_EVENTS_H_
