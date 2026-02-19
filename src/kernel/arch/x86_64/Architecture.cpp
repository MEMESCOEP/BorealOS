#include <Definitions.h>

extern "C" {
    extern char _kernel_stack_top[];
    extern char _kernel_stack_bottom[];
    extern char _kernel_base[];
    extern char _kernel_end[];
}

namespace Architecture {
    volatile uintptr_t *KernelStackTop = reinterpret_cast<uintptr_t *>(&_kernel_stack_top[0]);
    volatile uintptr_t *KernelStackBottom = reinterpret_cast<uintptr_t *>(&_kernel_stack_bottom[0]);
    volatile size_t KernelStackSize = 0;
    volatile uintptr_t *KernelBase = reinterpret_cast<uintptr_t *>(&_kernel_base[0]);
    volatile uintptr_t *KernelEnd = reinterpret_cast<uintptr_t *>(&_kernel_end[0]);
    volatile size_t KernelSize = 0;
    volatile uint32_t KernelPageSize = 0x1000; // 4KB
}

// Simple implementations of memcpy, memset, memmove, and memcmp (required by libc)
extern "C" {
    void *memcpy(void * dest, const void * src, size_t n) {
        auto pdest = static_cast<uint8_t *>(dest);
        auto psrc = static_cast<const uint8_t *>(src);

        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }

        return dest;
    }

    void *memset(void *s, int c, size_t n) {
        auto p = static_cast<uint8_t *>(s);

        for (size_t i = 0; i < n; i++) {
            p[i] = static_cast<uint8_t>(c);
        }

        return s;
    }

    void *memmove(void *dest, const void *src, size_t n) {
        auto *pdest = static_cast<uint8_t *>(dest);
        auto *psrc = static_cast<const uint8_t *>(src);

        if (src > dest) {
            for (size_t i = 0; i < n; i++) {
                pdest[i] = psrc[i];
            }
        } else if (src < dest) {
            for (size_t i = n; i > 0; i--) {
                pdest[i-1] = psrc[i-1];
            }
        }

        return dest;
    }

    int memcmp(const void *s1, const void *s2, size_t n) {
        auto p1 = static_cast<const uint8_t *>(s1);
        auto *p2 = static_cast<const uint8_t *>(s2);

        for (size_t i = 0; i < n; i++) {
            if (p1[i] != p2[i]) {
                return p1[i] < p2[i] ? -1 : 1;
            }
        }

        return 0;
    }

    int strcmp(const char *s1, const char *s2) {
        while (*s1 && (*s1 == *s2)) {
            s1++;
            s2++;
        }
        return *reinterpret_cast<const unsigned char *>(s1) - *reinterpret_cast<const unsigned char *>(s2);
    }

    int strncmp(const char *s1, const char *s2, size_t n) {
        for (size_t i = 0; i < n; i++) {
            if (s1[i] != s2[i]) {
                return static_cast<unsigned char>(s1[i]) - static_cast<unsigned char>(s2[i]);
            }
            if (s1[i] == '\0') {
                return 0;
            }
        }
        return 0;
    }

    char* strchr(const char *s, int c) {
        while (*s) {
            if (*s == c) {
                return const_cast<char *>(s);
            }
            s++;
        }
        return nullptr;
    }
}