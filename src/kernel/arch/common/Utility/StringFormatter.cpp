#include <Utility/StringFormatter.h>

namespace Utility {
    uint32_t StringFormatter::snprintf(char* buf, size_t size, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        uint32_t result = vsnprintf(buf, size, fmt, args);
        va_end(args);
        return result;
    }

    uint32_t StringFormatter::vsnprintf(char* buf, size_t size, const char* fmt, va_list args) {
        if (size == 0)
            return 0;

        size_t pos = 0;
        size_t max_pos = size - 1; // Reserve space for null terminator

        const char digits[] = "0123456789abcdef";
        const char prefix_0x[] = "0x";
        const char bad_fmt_i[] = "[BAD FORMAT SPECIFIER FOR SIGNED INTEGER]";
        const char bad_fmt_u[] = "[BAD FORMAT SPECIFIER FOR UNSIGNED INTEGER]";
        const char bad_fmt_x[] = "[BAD FORMAT SPECIFIER FOR HEX INTEGER]";
        const char bad_fmt_unknown[] = "[UNKNOWN FORMAT SPECIFIER]";

        auto add_char = [&](char c) {
            if (pos < max_pos)
                buf[pos++] = c;
        };

        auto add_str = [&](const char* s) {
            while (*s)
                add_char(*s++);
        };

        auto add_uint = [&](uint64_t v, uint32_t base) {
            char tmp[32];
            size_t n = 0;

            if (v == 0) {
                add_char('0');
                return;
            }

            while (v > 0) {
                tmp[n++] = digits[v % base];
                v /= base;
            }

            while (n > 0)
                add_char(tmp[--n]);
        };

        auto add_int = [&](int64_t v) {
            if (v < 0) {
                add_char('-');
                add_uint((uint64_t)(-v), 10);
            } else {
                add_uint((uint64_t)v, 10);
            }
        };

        auto parse_width = [&](const char*& p) -> uint32_t {
            uint32_t w = 0;
            while (*p >= '0' && *p <= '9') {
                w = w * 10 + (*p - '0');
                ++p;
            }
            return w;
        };

        while (*fmt && pos < max_pos) {
            if (*fmt != '%') {
                add_char(*fmt++);
                continue;
            }

            ++fmt; // skip '%'

            if (*fmt == '%') {
                add_char('%');
                ++fmt;
                continue;
            }

            // pointer
            if (*fmt == 'p') {
                ++fmt;
                auto ptr = reinterpret_cast<uintptr_t>(va_arg(args, void*));
                add_str(prefix_0x);
                add_uint(ptr, 16);
                continue;
            }

            // signed integer: %iXX
            if (*fmt == 'i') {
                ++fmt;
                uint32_t w = parse_width(fmt);

                switch (w) {
                    case 8:
                    case 16:
                    case 32: {
                        const int v = va_arg(args, int);
                        add_int((int32_t)v);
                        break;
                    }
                    case 64: {
                        const long long v = va_arg(args, long long);
                        add_int((int64_t)v);
                        break;
                    }
                    default:
                        add_str(bad_fmt_i);
                        break;
                }
                continue;
            }

            // unsigned integer: %uXX
            if (*fmt == 'u') {
                ++fmt;
                uint32_t w = parse_width(fmt);

                switch (w) {
                    case 8:
                    case 16:
                    case 32: {
                        const uint32_t v = va_arg(args, uint32_t);
                        add_uint((uint64_t)v, 10);
                        break;
                    }
                    case 64: {
                        const uint64_t v =
                            va_arg(args, uint64_t);
                        add_uint(v, 10);
                        break;
                    }
                    default:
                        add_str(bad_fmt_u);
                        break;
                }
                continue;
            }

            // hex integer: %xXX
            if (*fmt == 'x') {
                ++fmt;
                uint32_t w = parse_width(fmt);

                switch (w) {
                    case 8:
                    case 16:
                    case 32: {
                        const uint32_t v = va_arg(args, uint32_t);
                        add_uint((uint32_t)v, 16);
                        break;
                    }
                    case 64: {
                        const uint64_t v =
                            va_arg(args, uint64_t);
                        add_uint((uint64_t)v, 16);
                        break;
                    }
                    default:
                        add_str(bad_fmt_x);
                        break;
                }
                continue;
            }

            if (*fmt == 'c') {
                ++fmt;
                char c = static_cast<char>(va_arg(args, int));
                add_char(c);
                continue;
            }

            if (*fmt == 's') {
                ++fmt;
                const char* s = va_arg(args, const char*);
                if (s == nullptr)
                    s = bad_fmt_unknown;
                add_str(s);
                continue;
            }

            // unknown specifier
            add_str(bad_fmt_unknown);
            if (*fmt)
                ++fmt;
        }

        buf[pos] = '\0'; // Null-terminate the string
        return static_cast<uint32_t>(pos);
    }

    uint32_t StringFormatter::strlen(const char* str) {
        uint32_t len = 0;
        while (str[len] != '\0') {
            len++;
        }
        return len;
    }
} // Utility