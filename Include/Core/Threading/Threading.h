#ifndef THREADING_H
#define THREADING_H

#include <stdint.h>

typedef struct {
    volatile uint8_t lock;
} spinlock_t;

void spinlock_init(spinlock_t *lock);
void spinlock_lock(spinlock_t *lock);
void spinlock_unlock(spinlock_t *lock);

#endif //THREADING_H
