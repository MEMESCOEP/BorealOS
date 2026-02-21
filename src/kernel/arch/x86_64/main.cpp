#include <Definitions.h>
#include <Kernel.h>

#include "KernelData.h"

extern "C" [[noreturn]] void kmain() {
    // Initialize architecture-specific kernel parameters
    Architecture::KernelSize = reinterpret_cast<uintptr_t>(Architecture::KernelEnd) - reinterpret_cast<uintptr_t>(Architecture::KernelBase);
    Architecture::KernelStackSize = reinterpret_cast<uintptr_t>(Architecture::KernelStackTop) - reinterpret_cast<uintptr_t>(Architecture::KernelStackBottom);

    auto kernel = Kernel<KernelData>::GetInstance();
    kernel->Initialize();
    kernel->Start();

    LOG_ERROR("Clearly, something is not working correctly but whatever");
    asm volatile ("cli");
    while (true) {
        asm ("hlt");
    }
}
