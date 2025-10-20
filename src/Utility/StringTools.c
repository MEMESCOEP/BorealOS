#include "StringTools.h"

#include <Core/Kernel.h>
#include <Core/Memory/HeapAllocator.h>
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

int strcmp(const char *s1, const char *s2) {
    for (;;) {
        unsigned char c1 = (unsigned char)*s1++;
        unsigned char c2 = (unsigned char)*s2++;

        if (c1 != c2) {
            return c1 - c2;
        }

        if (c1 == '\0') {
            return 0;
        }
    }
}

size_t StringHexToSize(const char *hexStr, size_t length) {
    size_t val = 0;
    for (size_t i = 0; i < length; i++) {
        char c = hexStr[i];
        val <<= 4; // Shift left by 4 bits to make room for the next hex digit

        if (c >= '0' && c <= '9') val |= (c - '0');
        else if (c >= 'A' && c <= 'F') val |= (c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') val |= (c - 'a' + 10);
    }
    return val;
}

char ** StringSplit(const char *str, char delimiter, size_t *outCount) {
    size_t count = 0;
    const char *temp = str;
    while (*temp) {
        if (*temp == delimiter) {
            count++;
        }
        temp++;
    }
    count++; // For the last segment

    // Allocate array for pointers
    char **result = (char **)HeapAlloc(&Kernel.Heap, sizeof(char *) * count);
    if (!result) {
        *outCount = 0;
        return nullptr; // Allocation failed
    }
    size_t index = 0;
    const char *start = str;
    temp = str;
    while (*temp) {
        if (*temp == delimiter) {
            size_t len = temp - start;
            char *segment = (char *)HeapAlloc(&Kernel.Heap, len + 1);
            if (!segment) {
                // Free previously allocated segments
                for (size_t j = 0; j < index; j++) {
                    HeapFree(&Kernel.Heap, result[j]);
                }
                HeapFree(&Kernel.Heap, result);
                *outCount = 0;
                return nullptr; // Allocation failed
            }
            for (size_t j = 0; j < len; j++) {
                segment[j] = start[j];
            }
            segment[len] = '\0';
            result[index++] = segment;
            start = temp + 1;
        }
        temp++;
    }
    // Last segment
    size_t len = temp - start;
    char *segment = (char *)HeapAlloc(&Kernel.Heap, len + 1);
    if (!segment) {
        // Free previously allocated segments
        for (size_t j = 0; j < index; j++) {
            HeapFree(&Kernel.Heap, result[j]);
        }
        HeapFree(&Kernel.Heap, result);
        *outCount = 0;
        return nullptr; // Allocation failed
    }
    for (size_t j = 0; j < len; j++) {
        segment[j] = start[j];
    }
    segment[len] = '\0';
    result[index++] = segment;
    *outCount = count;
    return result;
}

size_t StringLastIndexOf(const char *str, char ch) {
    for (size_t i = strlen(str); i > 0; i--) {
        if (str[i - 1] == ch) {
            return i - 1;
        }
    }
    return (size_t)-1; // Not found
}

size_t StringIndexOf(const char *str, char ch) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == ch) {
            return i;
        }
    }
    return (size_t)-1; // Not found
}

char * StringDuplicate(HeapAllocatorState *heap, const char *str) {
    size_t len = strlen(str);
    if (len == (size_t)-1) {
        return nullptr; // String too long
    }

    char *dup = (char *)HeapAlloc(heap, len + 1);
    if (!dup) {
        return nullptr; // Allocation failed
    }

    for (size_t i = 0; i <= len; i++) {
        dup[i] = str[i];
    }

    return dup;
}

char * StringSubstring(HeapAllocatorState *heap, const char *str, size_t start, size_t length) {
    auto strLen = strlen(str);
    if (strLen == (size_t)-1 || start >= strLen) {
        return nullptr; // Invalid parameters
    }
    if (start + length > strLen) {
        length = strLen - start; // Adjust length to fit within string
    }
    char *substr = (char *)HeapAlloc(heap, length + 1);
    if (!substr) {
        return nullptr; // Allocation failed
    }
    for (size_t i = 0; i < length; i++) {
        substr[i] = str[start + i];
    }
    substr[length] = '\0';
    return substr;
}

char * StringConcat(HeapAllocatorState *heap, const char *str1, const char *str2) {
    auto len1 = strlen(str1);
    auto len2 = strlen(str2);
    if (len1 == (size_t)-1 || len2 == (size_t)-1) {
        return nullptr; // String too long
    }
    char *concat = (char *)HeapAlloc(heap, len1 + len2 + 1);
    if (!concat) {
        return nullptr; // Allocation failed
    }
    for (size_t i = 0; i < len1; i++) {
        concat[i] = str1[i];
    }
    for (size_t i = 0; i < len2; i++) {
        concat[len1 + i] = str2[i];
    }
    concat[len1 + len2] = '\0';
    return concat;
}

char * StringConcat3(HeapAllocatorState *heap, const char *str1, const char *str2, const char *str3) {
    auto len1 = strlen(str1);
    auto len2 = strlen(str2);
    auto len3 = strlen(str3);
    if (len1 == (size_t)-1 || len2 == (size_t)-1 || len3 == (size_t)-1) {
        return nullptr; // String too long
    }
    char *concat = (char *)HeapAlloc(heap, len1 + len2 + len3 + 1);
    if (!concat) {
        return nullptr; // Allocation failed
    }
    for (size_t i = 0; i < len1; i++) {
        concat[i] = str1[i];
    }
    for (size_t i = 0; i < len2; i++) {
        concat[len1 + i] = str2[i];
    }
    for (size_t i = 0; i < len3; i++) {
        concat[len1 + len2 + i] = str3[i];
    }
    concat[len1 + len2 + len3] = '\0';
    return concat;
}

bool StringStartsWith(const char *str, const char *prefix) {
    for (size_t i = 0; prefix[i] != '\0'; i++) {
        if (str[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}
