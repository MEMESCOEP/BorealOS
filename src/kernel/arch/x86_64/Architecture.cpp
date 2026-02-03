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