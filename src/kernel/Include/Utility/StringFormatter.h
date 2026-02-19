#ifndef BOREALOS_STRINGFORMATTER_H
#define BOREALOS_STRINGFORMATTER_H

#include <Definitions.h>
#include <stdarg.h>

namespace Utility {
    class StringFormatter {
    public:
        static uint32_t snprintf(char* buf, size_t size, const char* fmt, ...);
        static uint32_t vsnprintf(char* buf, size_t size, const char* fmt, va_list args);
        static uint32_t strlen(const char* str);
        static size_t HexToSize(const char* hexStr, size_t length);
    };
}


#endif //BOREALOS_STRINGFORMATTER_H