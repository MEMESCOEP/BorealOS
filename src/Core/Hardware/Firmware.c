/* LIBRARIES */
#include <Core/Multiboot/multiboot.h>
#include <Core/Hardware/Firmware.h>
#include <stddef.h>


/* VARIABLES */
int FirmwareType = 0;


/* FUNCTIONS */
// Determine if the system is running BIOS or UEFI firmware by checking for the EFI tables. If they don't exist, we can assume the system is BIOS
void GetFirmwareType(void* MB2InfoPtr)
{
    FirmwareType = FindMB2Tag(MULTIBOOT_TAG_TYPE_EFI64, MB2InfoPtr) != NULL;
}