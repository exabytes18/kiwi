/* No header guard. This header is intended to be included multiple times. */
#include "config.h"

#ifdef DARWIN_SPINLOCKS
#include <libkern/OSAtomic.h>
#endif

#ifdef PTHREAD_SPINLOCKS
#include <pthread.h>
#endif

/*
 * Many platforms provide their own implementations of spinlocks. While
 * implementing our own spinlocks is doable, writing spinlocks that are optimal
 * for all platforms and all processors is really hard. Need to worry about:
 *     - memory ordering
 *     - inserting pause / nop instructions
 *
 * Rather than implementing our own spinlocks, it's easier to just use platform
 * libraries where available.
 */
struct spinlock {
#ifdef DARWIN_SPINLOCKS
	OSSpinLock lock;
#elif defined(PTHREAD_SPINLOCKS)
	pthread_spinlock_t lock;
#else
	pthread_mutex_t lock;
#endif
};


static inline void spinlock_init(struct spinlock* spinlock) {
#ifdef DARWIN_SPINLOCKS
	spinlock->lock = 0;
#elif defined(PTHREAD_SPINLOCKS)
	pthread_spin_init(&spinlock->lock, PTHREAD_PROCESS_PRIVATE);
#else
	pthread_mutex_init(&spinlock->lock, NULL);
#endif
}


static inline void spinlock_lock(struct spinlock* spinlock) {
#ifdef DARWIN_SPINLOCKS
	OSSpinLockLock(&spinlock->lock);
#elif defined(PTHREAD_SPINLOCKS)
	pthread_spin_lock(&spinlock->lock);
#else
	pthread_mutex_lock(&spinlock->lock);
#endif
}


static inline void spinlock_unlock(struct spinlock* spinlock) {
#ifdef DARWIN_SPINLOCKS
	OSSpinLockUnlock(&spinlock->lock);
#elif defined(PTHREAD_SPINLOCKS)
	pthread_spin_unlock(&spinlock->lock);
#else
	pthread_mutex_unlock(&spinlock->lock);
#endif
}


static inline void spinlock_destroy(struct spinlock* spinlock) {
#ifdef DARWIN_SPINLOCKS
	/* nothing to destroy */
#elif defined(PTHREAD_SPINLOCKS)
	pthread_spin_destroy(&spinlock->lock);
#else
	pthread_mutex_destroy(&spinlock->lock);
#endif
}
