/* LIBRARIES */
#include <Core/Multiboot/multiboot.h>
#include <Core/Hardware/Firmware.h>
#include <stddef.h>


/* VARIABLES */
int FirmwareType = 0;


/* FUNCTIONS */
void GetFirmwareType(void* MB2InfoPtr)
{
    FirmwareType = FindMB2Tag(MULTIBOOT_TAG_TYPE_EFI64, MB2InfoPtr) != NULL;
}