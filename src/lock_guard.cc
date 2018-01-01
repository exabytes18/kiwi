#include "lock_guard.h"


LockGuard::LockGuard(Mutex& mutex) : mutex(mutex) {
    mutex.Lock();
}


LockGuard::~LockGuard(void) {
    mutex.Unlock();
}
