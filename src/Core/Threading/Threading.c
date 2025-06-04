#include <Core/Threading/Threading.h>

void spinlock_init(spinlock_t *lock) {
    lock->lock = 0; // Initialize the lock to unlocked state
}

void spinlock_lock(spinlock_t *lock) {
    while (__atomic_exchange_n(&lock->lock, 1, __ATOMIC_ACQUIRE)) {
        asm volatile("pause"); // Yield to avoid busy-waiting too aggressively
    }
}

void spinlock_unlock(spinlock_t *lock) {
    __atomic_store_n(&lock->lock, 0, __ATOMIC_RELEASE); // Release the lock
}