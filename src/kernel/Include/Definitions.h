#ifndef BOREALOS_DEFINITIONS_H
#define BOREALOS_DEFINITIONS_H

#include <cstdint>
#include <cstddef>

// Some helper macros
#define PACKED __attribute__((packed))
#define ALIGNED(x) __attribute__((aligned(x)))
#define UNUSED __attribute__((unused))
#define SET_BIT(x, n) ((x) |= (1U << (n)))
#define CLEAR_BIT(p, n) ((p) &= (~(1U) << (n)))

enum class LOG_LEVEL {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

namespace Core {
    [[noreturn]]
    void Panic(const char* message);
    void Log(LOG_LEVEL level, const char* fmt, ...);
    void Write(const char* message);
}

namespace Constants{
constexpr uint64_t KiB = 1024;
constexpr uint64_t MiB = KiB * 1024;
constexpr uint64_t GiB = MiB * 1024;
constexpr uint64_t TiB = GiB * 1024;
constexpr uint64_t PiB = TiB * 1024;
}

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)
#define LOG(level, msg, ...) Core::Log(level, "[" __FILE_NAME__ ":" STRINGIFY(__LINE__)"] " msg "\n" __VA_OPT__(,) __VA_ARGS__)
#define PANIC(msg) Core::Panic("[" __FILE_NAME__ ":" STRINGIFY(__LINE__)"] " msg)

#define LOG_INFO(msg, ...) LOG(LOG_LEVEL::INFO, msg, __VA_ARGS__)
#define LOG_WARNING(msg, ...) LOG(LOG_LEVEL::WARNING, msg, __VA_ARGS__)
#define LOG_ERROR(msg, ...) LOG(LOG_LEVEL::ERROR, msg, __VA_ARGS__)
#define LOG_DEBUG(msg, ...) LOG(LOG_LEVEL::DEBUG, msg, __VA_ARGS__)

#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))

enum class STATUS {
    SUCCESS,
    FAILURE
};

// Architecture specific kernel definitions
namespace Architecture {
    extern volatile uintptr_t *KernelStackTop;
    extern volatile uintptr_t *KernelStackBottom;
    extern volatile size_t KernelStackSize;
    extern volatile uintptr_t *KernelBase;
    extern volatile uintptr_t *KernelEnd;
    extern volatile size_t KernelSize;
    extern volatile uint32_t KernelPageSize;
}

extern "C" {
    void *memcpy(void * dest, const void * src, size_t n);
    void *memset(void *s, int c, size_t n);
    void *memmove(void *dest, const void *src, size_t n);
    int memcmp(const void *s1, const void *s2, size_t n);
    int strcmp(const char *s1, const char *s2);
    int strncmp(const char *s1, const char *s2, size_t n);
    char *strchr(const char *s, int c);
}

#endif //BOREALOS_DEFINITIONS_H