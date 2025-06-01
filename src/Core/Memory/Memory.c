#include <Core/Memory/Memory.h>

void *MemSet(void *ptr, int value, size_t num)
{
    unsigned char *p = ptr;
    while (num--)
    {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

void *MemCpy(void *dest, const void *src, size_t num)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (num--)
    {
        *d++ = *s++;
    }
    return dest;
}

void *MemMove(void *dest, const void *src, size_t num)
{
    unsigned char *d = dest;
    const unsigned char *s = src;

    if (d < s)
    {
        while (num--)
        {
            *d++ = *s++;
        }
    }
    else
    {
        d += num;
        s += num;
        while (num--)
        {
            *(--d) = *(--s);
        }
    }
    return dest;
}

bool MemCmp(const void *ptr1, const void *ptr2, size_t num)
{
    const unsigned char *p1 = ptr1;
    const unsigned char *p2 = ptr2;
    while (num--)
    {
        if (*p1++ != *p2++)
        {
            return false;
        }
    }
    
    return true;
}