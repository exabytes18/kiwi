#ifndef KIWI_MUTEX_H_
#define KIWI_MUTEX_H_

#include <pthread.h>


class Mutex {
public:
    Mutex(void);
    ~Mutex(void);
    void Lock(void) noexcept;
    void Unlock(void) noexcept;

    // Movable
    Mutex(Mutex&& mutex)                 = default;
    Mutex& operator=(Mutex&& mutex)      = default;

    // Non-copyable
    Mutex(const Mutex& mutex)            = delete;
    Mutex& operator=(const Mutex& mutex) = delete;

private:
    pthread_mutex_t mutex;
};

#endif  // KIWI_MUTEX_H_
