#ifndef BOREALOS_DEFINITIONS_H
#define BOREALOS_DEFINITIONS_H

#include <cstdint>
#include <cstddef>

// Some helper macros
#define PACKED __attribute__((packed))
#define ALIGNED(x) __attribute__((aligned(x)))
#define UNUSED __attribute__((unused))

enum class LOG_LEVEL {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

namespace Core {
    [[noreturn]]
    void Panic(const char* message);
    void Log(LOG_LEVEL level, const char* message);
    void Write(const char* message);
}

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)
#define LOG(level, msg) Core::Log(level, "[" __FILE_NAME__ ":" STRINGIFY(__LINE__)"] " msg "\n")
#define PANIC(msg) Core::Panic("[PANIC] " "[" __FILE_NAME__ ":" STRINGIFY(__LINE__)"] " msg)

#define LOG_INFO(msg) LOG(LOG_LEVEL::INFO, msg)
#define LOG_WARNING(msg) LOG(LOG_LEVEL::WARNING, msg)
#define LOG_ERROR(msg) LOG(LOG_LEVEL::ERROR, msg)
#define LOG_DEBUG(msg) LOG(LOG_LEVEL::DEBUG, msg)

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
#endif //BOREALOS_DEFINITIONS_H