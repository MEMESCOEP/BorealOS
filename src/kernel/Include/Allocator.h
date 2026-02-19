#ifndef BOREALOS_ALLOCATOR_H
#define BOREALOS_ALLOCATOR_H

#include <Definitions.h>

class Allocator {
public:
    virtual ~Allocator() = default;

    /// Allocates a block of memory of the given size and returns a pointer to it, or nullptr if the allocation failed.
    [[nodiscard]] virtual void* Allocate(size_t size) = 0;

    /// Frees a previously allocated block of memory at the given pointer with the given size. The pointer must have been returned by a previous call to Allocate with the same size.
    virtual void Free(void* ptr, size_t size) = 0;
};

#endif //BOREALOS_ALLOCATOR_H