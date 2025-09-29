#ifndef BOREALOS_MEMORY_H
#define BOREALOS_MEMORY_H

#include <Definitions.h>

/// Set a block of memory to a specific 32-bit value using optimized 8086 instructions.
void* memset32_8086(void* bufptr, uint32_t value, size_t count);

/// Set a block of memory to a specific byte value.
void* memset(void* bufptr, int value, size_t size);

/// Move a block of memory from source to destination, handling overlapping regions correctly.
void* memmove(void* destptr, const void* srcptr, size_t size);

/// Compare two blocks of memory. Returns the same as memcmp from libc (0 if equal, <0 if buf1<buf2, >0 if buf1>buf2).
int memcmp(const void* buf1, const void* buf2, size_t size);

/// Memory copy function. Copies 'size' bytes from 'srcptr' to 'destptr'.
void* memcpy(void* destptr, const void* srcptr, size_t size);

#endif //BOREALOS_MEMORY_H