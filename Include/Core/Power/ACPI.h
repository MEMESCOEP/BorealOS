#ifndef ACPI_H
#define ACPI_H

/* DEFINITIONS */
#define MAX_MEMORY_REGIONS 128
#define RSDP_RANGE_BOTTOM 0x000E0000
#define RSDP_RANGE_TOP 0x000FFFFF
#define E820_MEMORY_MAP 0xE820


/* VARIABLES */
extern uint64_t RSDPAddress;
extern uint64_t HHOffset;
extern uint8_t ACPIRevision;
extern bool ACPIInitialized;


/* FUNCTIONS */
void InitACPI(void* MB2InfoPtr);
void ACPIShutdown();
void ACPIReboot();

#endif