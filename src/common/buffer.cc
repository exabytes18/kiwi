#include <arpa/inet.h>
#include <cstring>
#include "buffer.h"


Buffer::Buffer(size_t capacity) :
        position(0),
        limit(capacity),
        capacity(capacity),
        data(new char[capacity]) {
}


Buffer::~Buffer(void) noexcept {
    delete[] data;
}


Buffer::Buffer(Buffer const& other) :
        position(other.position),
        limit(other.limit),
        capacity(other.capacity),
        data(new char[capacity]) {
    std::memcpy(data, other.data, capacity);
}


Buffer& Buffer::operator=(Buffer const& other) {
    if (this == &other) {
        return *this;
    }

    if (data != nullptr) {
        delete[] data;
    }

    position = other.position;
    limit = other.limit;
    capacity = other.capacity;
    data = new char[capacity];
    std::memcpy(data, other.data, capacity);
    return *this;
}


Buffer::Buffer(Buffer&& other) noexcept :
        position(other.position),
        limit(other.limit),
        capacity(other.capacity),
        data(other.data) {
    other.data = nullptr;
}


Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (data != nullptr) {
        delete[] data;
    }

    position = other.position;
    limit = other.limit;
    capacity = other.capacity;
    data = other.data;

    other.data = nullptr;
    return *this;
}


size_t Buffer::Position(void) {
    return position;
}

void Buffer::Position(size_t position) {
    this->position = position;
}


size_t Buffer::Limit(void) {
    return limit;
}


void Buffer::Limit(size_t limit) {
    this->limit = limit;
}


size_t Buffer::Capacity(void) {
    return capacity;
}


void Buffer::ResetAndGrow(size_t new_capacity) {
    delete[] data;
    data = nullptr;

    data = new char[new_capacity];
    capacity = new_capacity;
    position = 0;
    limit = new_capacity;
}


char* Buffer::Data(void) {
    return data;
}


size_t Buffer::Remaining(void) {
    return limit - position;
}


void Buffer::Clear(void) {
    position = 0;
    limit = capacity;
}


void Buffer::Flip(void) {
    limit = position;
    position = 0;
}


void Buffer::FillFrom(Buffer& src) {
    size_t bytes_to_copy = src.Remaining();
    size_t space_available = Remaining();
    if (space_available < bytes_to_copy) {
        bytes_to_copy = space_available;
    }

    std::memcpy(data + position, src.data + src.position, bytes_to_copy);
    position += bytes_to_copy;
    src.position += bytes_to_copy;
}


uint32_t Buffer::UnsafeGetInt(void) {
    uint32_t network_order_value;
    std::memcpy(&network_order_value, data + position, 4);
    position += 4;
    return ntohl(network_order_value);
}


uint16_t Buffer::UnsafeGetShort(void) {
    uint16_t network_order_value;
    std::memcpy(&network_order_value, data + position, 2);
    position += 2;
    return ntohs(network_order_value);
}


std::string Buffer::UnsafeGetString(size_t length) {
    auto value = std::string(data + position, length);
    position += length;
    return value;
}


void Buffer::UnsafePutInt(uint32_t value) {
    uint32_t network_order_value = htonl(value);
    std::memcpy(data + position, &network_order_value, 4);
    position += 4;
}


void Buffer::UnsafePutShort(uint16_t value) {
    uint16_t network_order_value = htons(value);
    std::memcpy(data + position, &network_order_value, 2);
    position += 2;
}


void Buffer::UnsafePutString(std::string value) {
    size_t length = value.length();
    std::memcpy(data + position, value.data(), length);
    position += length;
}
