#ifndef KIWI_LOCK_GUARD_H_
#define KIWI_LOCK_GUARD_H_

#include <pthread.h>
#include "mutex.h"


class LockGuard {
public:
    LockGuard(Mutex& mutex);
    ~LockGuard(void);
    // movable, but not copyable

private:
    Mutex& mutex;
};

#endif  // KIWI_LOCK_GUARD_H_
