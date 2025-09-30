#include "StringTools.h"
//#include <string.h>

// Provided from https://wiki.osdev.org/Meaty_Skeleton, but I've added a length limit
size_t strlen(const char* str) {
	size_t len = 0;

	while (str[len])
    {
        len++;

        if (len > 4096)
            return -1;
    }

	return len;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned char c1 = (unsigned char)s1[i];
        unsigned char c2 = (unsigned char)s2[i];

        if (c1 != c2 || c1 == '\0' || c2 == '\0') {
            return c1 - c2;
        }
    }
    
    return 0;
}