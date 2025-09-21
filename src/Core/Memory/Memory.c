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