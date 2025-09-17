#include <Boot/multiboot.h>

#include "Definitions.h"
#include "Core/Kernel.h"

NORETURN
void EntryPoint(uint32_t Magic, uint32_t InfoPtr) {
    KernelState kernel;
    Status result = KernelLoad(InfoPtr, &kernel);

    if (Magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        PANIC(&kernel, "Bootloader is not Multiboot2 compliant!\n");
    }

    if (result != STATUS_SUCCESS) {
        PANIC(&kernel, "Failed to load kernel!\n");
    }

    PANIC(&kernel, "No more instructions to run!\n");
}