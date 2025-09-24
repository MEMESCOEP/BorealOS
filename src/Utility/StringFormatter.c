#include "StringFormatter.h"

#include <stdarg.h>

void putchar_buf(char c, char* buffer, size_t bufferSize, size_t* pos) {
    if (*pos < bufferSize - 1) { // Leave space for null terminator
        buffer[*pos] = c;
        (*pos)++;
    }
}

void putstr_buf(const char* str, char* buffer, size_t bufferSize, size_t* pos) {
    while (*str) {
        putchar_buf(*str++, buffer, bufferSize, pos);
    }
}

void putdec_buf(int32_t value, char* buffer, size_t bufferSize, size_t* pos) {
    char tmp[32];
    int i = 0;
    if (value == 0) {
        putchar_buf('0', buffer, bufferSize, pos);
        return;
    }
    if (value < 0) {
        putchar_buf('-', buffer, bufferSize, pos);
        value = -value;
    }
    while (value > 0) {
        tmp[i++] = '0' + (value % 10);
        value /= 10;
    }
    while (i > 0) {
        putchar_buf(tmp[--i], buffer, bufferSize, pos);
    }
}

void putunsigned_buf(int32_t value, char* buffer, size_t bufferSize, size_t* pos) {
    char tmp[32];
    int i = 0;
    if (value == 0) {
        putchar_buf('0', buffer, bufferSize, pos);
        return;
    }
    while (value > 0) {
        tmp[i++] = '0' + (value % 10);
        value /= 10;
    }
    while (i > 0) {
        putchar_buf(tmp[--i], buffer, bufferSize, pos);
    }
}

void puthex_buf(uint64_t value, char* buffer, size_t bufferSize, size_t* pos) {
    const char* hex = "0123456789ABCDEF";
    char tmp[32];
    int i = 0;
    if (value == 0) {
        putchar_buf('0', buffer, bufferSize, pos);
        return;
    }
    while (value > 0) {
        tmp[i++] = hex[value & 0xF];
        value >>= 4;
    }
    while (i > 0) {
        putchar_buf(tmp[--i], buffer, bufferSize, pos);
    }
}

void putsize_buf(size_t value, char* buffer, size_t bufferSize, size_t* pos) {
    char tmp[32];
    int i = 0;
    if (value == 0) {
        putchar_buf('0', buffer, bufferSize, pos);
        return;
    }
    while (value > 0) {
        tmp[i++] = '0' + (value % 10);
        value /= 10;
    }
    while (i > 0) {
        putchar_buf(tmp[--i], buffer, bufferSize, pos);
    }
}

size_t VStringFormat(char *buffer, size_t bufferSize, const char *format, va_list args) {
    size_t pos = 0;
    const char *fmt = format;
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 's':
                    putstr_buf(va_arg(args, const char *), buffer, bufferSize, &pos);
                    break;
                case 'c':
                    putchar_buf((char)va_arg(args, int), buffer, bufferSize, &pos);
                    break;
                case 'd':
                    putdec_buf(va_arg(args, int64_t), buffer, bufferSize, &pos);
                    break;
                case 'u':
                    putunsigned_buf(va_arg(args, uint64_t), buffer, bufferSize, &pos);
                    break;
                case 'x':
                    puthex_buf(va_arg(args, uint64_t), buffer, bufferSize, &pos);
                    break;
                case 'p':
                    putstr_buf("0x", buffer, bufferSize, &pos);
                    puthex_buf((uint64_t)(uintptr_t)va_arg(args, void *), buffer, bufferSize, &pos);
                    break;
                case 'z':
                    putsize_buf(va_arg(args, size_t), buffer, bufferSize, &pos);
                    break;
                case '%':
                    putchar_buf('%', buffer, bufferSize, &pos);
                    break;
                default:
                    // Unknown format specifier, just print it as is
                    putchar_buf('%', buffer, bufferSize, &pos);
                    putchar_buf(*fmt, buffer, bufferSize, &pos);
                    break;
            }
        } else {
            putchar_buf(*fmt, buffer, bufferSize, &pos);
        }
        fmt++;
    }
    if (bufferSize > 0) {
        buffer[pos < bufferSize ? pos : bufferSize - 1] = '\0';
    }
    return pos;
}

size_t StringFormat(char *buffer, size_t bufferSize, const char *format, ...) {
    va_list args;
    va_start(args, format);
    size_t written = VStringFormat(buffer, bufferSize, format, args);
    va_end(args);
    return written;
}
