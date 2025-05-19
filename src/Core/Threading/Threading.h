#ifndef THREADING_H
#define THREADING_H

#include <stdint.h>

// This header defines stuff such as spinlocks, mutexes, semaphores, etc. But for now just spinlocks.

typedef struct {
    volatile uint8_t lock;
} SPINLOCK;

static inline void SpinlockInit(SPINLOCK *lock) {
    lock->lock = 0;
}

static inline void SpinlockLock(SPINLOCK *lock) {
    while (__atomic_exchange_n(&lock->lock, 1, __ATOMIC_ACQUIRE)) {
        __asm__ volatile("pause");
    }
}

static inline void SpinlockUnlock(SPINLOCK *lock) {
    __atomic_store_n(&lock->lock, 0, __ATOMIC_RELEASE);
}

static inline int SpinlockTryLock(SPINLOCK *lock) {
    return __atomic_exchange_n(&lock->lock, 1, __ATOMIC_ACQUIRE) == 0;
}

static inline bool SpinlockIsLocked(SPINLOCK *lock) {
    return lock->lock != 0;
}

#endif //THREADING_H
