#include <Boot/multiboot.h>

#include "Definitions.h"
#include "Core/Kernel.h"

NORETURN
void EntryPoint(uint32_t Magic, uint32_t InfoPtr) {
    Status result = KernelInit(InfoPtr);

    if (Magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        PANIC("Bootloader is not Multiboot2 compliant!\n");
    }

    if (result != STATUS_SUCCESS) {
        PANIC("Failed to load kernel!\n");
    }

    PANIC("No more instructions to run!\n");
}