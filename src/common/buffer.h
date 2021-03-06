#ifndef KIWI_BUFFER_H_
#define KIWI_BUFFER_H_

#include <stdint.h>
#include <string>


class Buffer {
public:
    Buffer(size_t capacity);
    ~Buffer(void) noexcept;

    // Copy constructor + copy assignment operator
    Buffer(Buffer const& other);
    Buffer& operator=(Buffer const& other);

    // Move constructor + move assignment operator
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    size_t Position(void);
    void Position(size_t position);
    size_t Limit(void);
    void Limit(size_t limit);
    size_t Capacity(void);
    void ResetAndGrow(size_t new_capacity);
    char* Data();

    size_t Remaining(void);
    void Clear(void);
    void Flip(void);
    void FillFrom(Buffer& src);

    /*
     * Reads the bytes from the data array, convert the value from network
     * order to host order, and return the value.
     *
     * Unsafe: these functions do not do any bounds checks. However, they are
     * resilient to unaligned reads.
     */
    uint32_t UnsafeGetInt(void);
    uint16_t UnsafeGetShort(void);
    std::string UnsafeGetString(size_t length);

    void UnsafePutInt(uint32_t value);
    void UnsafePutShort(uint16_t value);
    void UnsafePutString(std::string value);

private:
    size_t position;
    size_t limit;
    size_t capacity;
    char* data;
};

#endif  // KIWI_BUFFER_H_
