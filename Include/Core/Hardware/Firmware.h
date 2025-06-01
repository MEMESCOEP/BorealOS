#ifndef FIRMWARE_H
#define FIRMWARE_H

/* LIBRARIES */
#include <Core/Multiboot/MB2Parser.h>


/* DEFINITIONS */
#define FIRMWARE_BIOS 0
#define FIRMWARE_UEFI 1


/* VARIABLES */
extern int FirmwareType;


/* FUNCTIONS */
void GetFirmwareType(void* MB2InfoPtr);

#endif