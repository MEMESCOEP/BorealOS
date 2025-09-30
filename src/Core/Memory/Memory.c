#include "Memory.h"

void* memset32_8086(void* bufptr, uint32_t value, size_t count)
{
    uint32_t* dest = (uint32_t*) bufptr;
    ASM (
        "rep stosl"
        : "+D" (dest), "+c" (count)     // outputs: EDI/RDI (dest), ECX/RCX (count)
        : "a" (value)                   // input: EAX/RAX (value)
        : "memory"                      // clobbers
    );
    return bufptr;
}

void* memset(void* bufptr, int value, size_t size)
{
    unsigned char* buf = (unsigned char*) bufptr;
    for (size_t i = 0; i < size; i++)
        buf[i] = (unsigned char) value;
    return bufptr;
}

void* memmove(void* destptr, const void* srcptr, size_t size) {
    unsigned char* dest = (unsigned char*) destptr;
    const unsigned char* src = (const unsigned char*) srcptr;

    if (dest == src || size == 0)
        return destptr;

    if (dest < src) {
        // Non-overlapping or forward-safe copy
        ASM (
            "rep movsb"
            : "+D"(dest), "+S"(src), "+c"(size)
            :
            : "memory"
        );
    } else {
        // Overlap requires reverse copy
        dest += size - 1;
        src  += size - 1;
        ASM (
            "std\n\t"            // set direction flag (backwards)
            "rep movsb\n\t"
            "cld"                // clear direction flag (restore forward mode)
            : "+D"(dest), "+S"(src), "+c"(size)
            :
            : "memory"
        );
    }

    return destptr;
}

int memcmp(const void *buf1, const void *buf2, size_t size) {
    if (buf1 == buf2 || size == 0) return true;
    const unsigned char* b1 = (const unsigned char*) buf1;
    const unsigned char* b2 = (const unsigned char*) buf2;

    for (size_t i = 0; i < size; i++) {
        if (b1[i] != b2[i]) {
            return (b1[i] < b2[i]) ? -1 : 1;
        }
    }

    return 0;
}

void* memcpy(void *destptr, const void *srcptr, size_t size) {
    if (destptr == srcptr || size == 0) return destptr;
    unsigned char* dest = (unsigned char*) destptr;
    const unsigned char* src = (const unsigned char*) srcptr;

    ASM (
        "rep movsb"
        : "+D"(dest), "+S"(src), "+c"(size)
        :
        : "memory"
    );

    return destptr;
}