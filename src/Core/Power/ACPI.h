#ifndef ACPI_H
#define ACPI_H

/* DEFINITIONS */
#define LIMINE_RSDP_REQUEST { LIMINE_COMMON_MAGIC, 0xc5e77b6b397e7b43, 0x27637845accdcf3c }
#define MAX_MEMORY_REGIONS 128
#define E820_MEMORY_MAP 0xE820


/* VARIABLES */
extern uint64_t RSDPAddress;
extern bool ACPIInitialized;


/* FUNCTIONS */
void InitACPI();
void ACPIShutdown();
void ACPIReboot();

#endif