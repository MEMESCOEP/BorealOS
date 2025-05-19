#ifndef MEMORY_H
#define MEMORY_H

/* LIBRARIES */
#include <Limine.h>


/* DEFINITIONS */
#define BIOS_MEMMAP_USABLE                 0
#define BIOS_MEMMAP_RESERVED               1
#define BIOS_MEMMAP_ACPI_RECLAIMABLE       2
#define BIOS_MEMMAP_ACPI_NVS               3
#define BIOS_MEMMAP_BAD_MEMORY             4
#define BIOS_MEMMAP_BOOTLOADER_RECLAIMABLE 5
#define BIOS_MEMMAP_EXECUTABLE_AND_MODULES 6
#define BIOS_MEMMAP_FRAMEBUFFER            7
#define EFI_MEMMAP_RESERVED                0
#define EFI_MEMMAP_LOADER_CODE             1
#define EFI_MEMMAP_LOADER_DATA             2
#define EFI_MEMMAP_BOOT_SERVICES_CODE      3
#define EFI_MEMMAP_BOOT_SERVICES_DATA      4
#define EFI_MEMMAP_RUNTIME_SERVICES_CODE   5
#define EFI_MEMMAP_RUNTIME_SERVICES_DATA   6
#define EFI_MEMMAP_USABLE                  7
#define EFI_MEMMAP_UNUSABLE                8
#define EFI_MEMMAP_ACPI_RECLAIMABLE        9
#define EFI_MEMMAP_ACPI_NVS                10
#define EFI_MEMMAP_MMIO                    11
#define EFI_MEMMAP_MMIO_PORT_SPACE         12
#define EFI_MEMMAP_PAL_CODE                13
#define EFI_MEMMAP_PERSISTENT_MEMORY       14




/* VARIABLES */
typedef struct {
    uint32_t Type;
    uint32_t Pad;               // Align to 8 bytes (required on x86_64)
    uint64_t PhysicalStart;
    uint64_t VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

extern uint64_t BootLDRReclaimableRAMKB;
extern uint64_t ACPIReclaimableRAMKB;
extern uint64_t DeclaredUsableRAMKB;
extern uint64_t EXECAndModsRAMKB;
extern uint64_t ReservedRAMKB;
extern uint64_t ACPINVSRAMKB;
extern uint64_t TotalRAMKB;


/* FUNCTIONS */
void ParseMEMMap();
void *MemCPY(void *dest, const void *src, size_t n);
void *MemSet(void *s, int c, size_t n);
void *MemMove(void *dest, const void *src, size_t n);
int MemCMP(const void *s1, const void *s2, size_t n);

#endif