#ifndef BOREALOS_STRINGFORMATTER_H
#define BOREALOS_STRINGFORMATTER_H

// This is a basic utility for formatting strings, similar to snprintf
// It only supports a very limited custom set of format specifiers for now
// Supported specifiers:
// %s - string
// %c - char
// %d - signed decimal integer
// %u - unsigned decimal integer
// %x - unsigned hexadecimal integer (lowercase)
// %p - pointer (prints as 0x followed by hexadecimal)
// %z - size_t (unsigned decimal integer)
// %% - literal percent sign

#include <Definitions.h>
#include <stdarg.h>

// Usage:
// Allocate a buffer on the stack or heap
// char buffer[128];
// Call the function with the buffer, its size, the format string, and the arguments
// size_t written = StringFormat(buffer, sizeof(buffer), "Hello %s, number: %d", "World", 42);
// If written is above or equal to bufferSize, the output was truncated, and the last character is always null terminator
size_t VStringFormat(char* buffer, size_t bufferSize, const char* format, va_list args);
size_t StringFormat(char* buffer, size_t bufferSize, const char* format, ...);

#endif //BOREALOS_STRINGFORMATTER_H