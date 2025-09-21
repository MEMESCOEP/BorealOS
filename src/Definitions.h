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

#define SET_BIT(bitmap, index) ((bitmap)[(index) / 8] |= (1 << ((index) % 8)))
#define CLEAR_BIT(bitmap, index) ((bitmap)[(index) / 8] &= ~(1 << ((index) % 8)))
#define IS_BIT_SET(bitmap, index) ((bitmap)[(index) / 8] & (1 << ((index) % 8)))

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
#define LOG_INFO(message) (Kernel.Log("[INFO] [" __FILE__ ":" STR(__LINE__) "] >> " message))
#define LOG_WARNING(message) (Kernel.Log("[WARNING] [" __FILE__ ":" STR(__LINE__) "] >> " message))
#define LOG_ERROR(message) (Kernel.Log("[ERROR] [" __FILE__ ":" STR(__LINE__) "] >> " message))
#define LOG_CRITICAL(message) (Kernel.Log("[CRITICAL] " __FILE__ ":" STR(__LINE__) "] >> " message))
#define LOG_DEBUG(message) (Kernel.Log("[DEBUG] [" __FILE__ ":" STR(__LINE__) "] >> " message))
#define LOG(message) LOG_INFO(message)

#define PRINTF(format, ...) (Kernel.Printf(format, __VA_ARGS__))
#define PRINT(string) (Kernel.Printf(string))

#define PANIC(message) (Kernel.Panic("[PANIC] [" __FILE__ ":" STR(__LINE__) "] >> " message))

#endif //BOREALOS_DEFINITIONS_H