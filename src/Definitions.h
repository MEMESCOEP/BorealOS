#ifndef BOREALOS_DEFINITIONS_H
#define BOREALOS_DEFINITIONS_H

// These are practically always needed, so just include them here
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Basic definitions, some macros for attributes and the version of the OS
// Version is MAJOR.MINOR.PATCH

#define BOREALOS_MAJOR_VERSION 0
#define BOREALOS_MINOR_VERSION 1
#define BOREALOS_PATCH_VERSION 0

// Okay yes technically these are unnecessary, but i'm not going to type __asm__ __volatile__ every time
// And because I have disabled all extensions, i cant just say asm volatile (why's this not standard?)

#define ASM __asm__ __volatile__
#define INLINE inline __attribute__((always_inline))
#define NORETURN __attribute__((noreturn))
#define PACKED __attribute__((packed))
#define UNUSED __attribute__((unused))
#define ALIGNED(x) __attribute__((aligned(x)))

#define BIT(n) (1ULL << (n))
#define BITMASK(h, l) (((1ULL << ((h) - (l) + 1)) - 1) << (l))

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// byte definitions (1024, and 1000) for both binary and decimal
#define KiB 1024ULL
#define MiB (1024ULL * KiB)
#define GiB (1024ULL * MiB)
#define TiB (1024ULL * GiB)

#define KB 1000ULL
#define MB (1000ULL * KB)
#define GB (1000ULL * MB)
#define TB (1000ULL * GB)

typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_FAILURE = -1,
} Status;

// Log structure:
// [TYPE] [FILE:LINE] : message
#define LOG_INFO(kernel, message) (kernel)->Log(kernel, "[INFO] [" __FILE__ ":" STR(__LINE__) "] >> " message)
#define LOG_WARNING(kernel, message) (kernel)->Log(kernel, "[WARNING] [" __FILE__ ":" STR(__LINE__) "] >> " message)
#define LOG_ERROR(kernel, message) (kernel)->Log(kernel, "[ERROR] [" __FILE__ ":" STR(__LINE__) "] >> " message)
#define LOG_CRITICAL(kernel, message) (kernel)->Log(kernel, "[CRITICAL] " __FILE__ ":" STR(__LINE__) "] >> " message)
#define LOG_DEBUG(kernel, message) (kernel)->Log(kernel, "[DEBUG] [" __FILE__ ":" STR(__LINE__) "] >> " message)
#define LOG(kernel, message) LOG_INFO(kernel, message)

#define PANIC(kernel, message) (kernel)->Panic(kernel, "[PANIC] [" __FILE__ ":" STR(__LINE__) "] >> " message)

#endif //BOREALOS_DEFINITIONS_H