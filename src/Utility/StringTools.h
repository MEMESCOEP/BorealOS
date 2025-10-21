#ifndef BOREALOS_STRINGTOOLS_H
#define BOREALOS_STRINGTOOLS_H

#include <Definitions.h>

// For libc compatibility
size_t strlen(const char* str);
int strncmp(const char* s1, const char* s2, size_t n);
int strcmp(const char* s1, const char* s2);

// Custom:

/// Convert a hexadecimal string to a size_t value.
size_t StringHexToSize(const char* hexStr, size_t length);

/// Split a string into an array of strings based on a delimiter.
/// It is your responsibility to free the returned array and its contents using HeapFree.
char** StringSplit(const char* str, char delimiter, size_t* outCount);

size_t StringLastIndexOf(const char* str, char ch);
size_t StringIndexOf(const char* str, char ch);

typedef struct HeapAllocatorState HeapAllocatorState;
char* StringDuplicate(HeapAllocatorState* heap, const char* str);
char* StringSubstring(HeapAllocatorState* heap, const char* str, size_t start, size_t length);
char* StringConcat(HeapAllocatorState* heap, const char* str1, const char* str2);
char* StringConcat3(HeapAllocatorState* heap, const char* str1, const char* str2, const char* str3);
bool StringStartsWith(const char* str, const char* prefix);

#endif //BOREALOS_STRINGTOOLS_H