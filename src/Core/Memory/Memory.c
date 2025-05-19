/* LIBRARIES */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "Utilities/StrUtils.h"
#include "Core/Graphics/Terminal.h"
#include "Memory.h"
#include "Kernel.h"


/* LIMINE */
__attribute__((used, section(".limine_requests")))
static struct limine_efi_memmap_request UEFIMEMMapRequest = {
    .id = LIMINE_EFI_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static struct limine_memmap_request MEMMapRequest = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};


/* VARIABLES */
uint64_t BootLDRReclaimableRAMKB = 0;
uint64_t ACPIReclaimableRAMKB = 0;
uint64_t DeclaredUsableRAMKB = 0;
uint64_t EXECAndModsRAMKB = 0;
uint64_t ReservedRAMKB = 0;
uint64_t ACPINVSRAMKB = 0;
uint64_t TotalRAMKB = 0;
uint64_t BadRAMKB = 0;
uint64_t FBRAMKB = 0;


/* FUNCTIONS */
// Parse the memory map (also gets the total installed & useable ram) There are different implementations for BIOS and UEFI here because
// the "BIOS" version doesn't correctly report installed memory under UEFI environments (reports ~4-8KB on a VM with 128MB)
void ParseMEMMap()
{
    if (FirmwareTypeID == LIMINE_FIRMWARE_TYPE_X86BIOS)
    {
        // Loop through all the memory entries
        for (uint64_t EntryIndex = 0; EntryIndex < MEMMapRequest.response->entry_count; EntryIndex++)
        {
            struct limine_memmap_entry* MapEntry = MEMMapRequest.response->entries[EntryIndex];
            uint64_t MapEntrySizeKB = MapEntry->length / 1024;

            switch(MapEntry->type)
            {
                // Memory regions to count
                case BIOS_MEMMAP_USABLE:
                    DeclaredUsableRAMKB = MapEntrySizeKB;
                    break;

                case BIOS_MEMMAP_BOOTLOADER_RECLAIMABLE:
                    BootLDRReclaimableRAMKB = MapEntrySizeKB;
                    break;

                case BIOS_MEMMAP_ACPI_RECLAIMABLE:
                    ACPIReclaimableRAMKB = MapEntrySizeKB;
                    break;

                case BIOS_MEMMAP_EXECUTABLE_AND_MODULES:
                    EXECAndModsRAMKB = MapEntrySizeKB;
                    break;

                case BIOS_MEMMAP_RESERVED:
                    ReservedRAMKB = MapEntrySizeKB;
                    break;

                case BIOS_MEMMAP_ACPI_NVS:
                    ACPINVSRAMKB = MapEntrySizeKB;
                    break;

                // Memory regions to not count
                case BIOS_MEMMAP_BAD_MEMORY:
                    BadRAMKB = MapEntrySizeKB;
                    break;

                case BIOS_MEMMAP_FRAMEBUFFER:
                    FBRAMKB = MapEntrySizeKB;
                    break;

                default:
                    char TypeBuf[4];
                    char PanicStr[64];

                    IntToStr(MapEntry->type, TypeBuf, 10);
                    StrCat("Invalid BIOS MEMMap entry type ", TypeBuf, PanicStr);
                    KernelPanic(0, PanicStr);
                    break;
            }
        }
    }
    else
    {
        EFI_MEMORY_DESCRIPTOR* MEMDescriptor = (EFI_MEMORY_DESCRIPTOR*)UEFIMEMMapRequest.response->memmap;

        for (uint64_t EntryIndex = 0; EntryIndex < (UEFIMEMMapRequest.response->memmap_size / UEFIMEMMapRequest.response->desc_size); EntryIndex++)
        {
            uint64_t PageRAMSize = (MEMDescriptor->NumberOfPages * 4096) / 1024;
            
            switch (MEMDescriptor->Type)
            {
                // Tracked in usable RAM counter
                case EFI_MEMMAP_USABLE:
                    DeclaredUsableRAMKB += PageRAMSize;
                    break;

                case EFI_MEMMAP_ACPI_RECLAIMABLE:
                    ACPIReclaimableRAMKB += PageRAMSize;
                    break;

                case EFI_MEMMAP_ACPI_NVS:
                    ACPINVSRAMKB += PageRAMSize;
                    break;

                case EFI_MEMMAP_BOOT_SERVICES_CODE:
                case EFI_MEMMAP_BOOT_SERVICES_DATA:
                case EFI_MEMMAP_LOADER_CODE:
                case EFI_MEMMAP_LOADER_DATA:
                    BootLDRReclaimableRAMKB += PageRAMSize;
                    break;

                case EFI_MEMMAP_RUNTIME_SERVICES_CODE:
                case EFI_MEMMAP_RUNTIME_SERVICES_DATA:
                case EFI_MEMMAP_PERSISTENT_MEMORY:
                case EFI_MEMMAP_RESERVED:
                    ReservedRAMKB += PageRAMSize;
                    break;

                // NOT tracked in installed RAM counter
                case EFI_MEMMAP_MMIO_PORT_SPACE:
                case EFI_MEMMAP_MMIO:
                    FBRAMKB += PageRAMSize;
                    break;

                case EFI_MEMMAP_UNUSABLE:
                    BadRAMKB += PageRAMSize;
                    break;

                default:
                    char TypeBuf[4];
                    char PanicStr[64];

                    IntToStr(MEMDescriptor->Type, TypeBuf, 10);
                    StrCat("Invalid UEFI MEMMap entry type ", TypeBuf, PanicStr);
                    KernelPanic(0, PanicStr);
                    break;
            }

            // Move to next descriptor
            MEMDescriptor = (EFI_MEMORY_DESCRIPTOR*)((uint8_t*)MEMDescriptor + UEFIMEMMapRequest.response->desc_size);
        }
    }

    // Calculate the total installed RAM
    TotalRAMKB = BootLDRReclaimableRAMKB + ACPIReclaimableRAMKB + DeclaredUsableRAMKB + EXECAndModsRAMKB + ReservedRAMKB + ACPINVSRAMKB;
}

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.
void *MemCPY(void *dest, const void *src, size_t n)
{
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *MemSet(void *s, int c, size_t n)
{
    uint8_t *p = (uint8_t *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *MemMove(void *dest, const void *src, size_t n)
{
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

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

int MemCMP(const void *s1, const void *s2, size_t n)
{
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}