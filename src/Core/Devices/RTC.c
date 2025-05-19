/* LIBRARIES */
#include <stdbool.h>
#include "Core/IO/RegisterIO.h"
#include "CMOS.h"
#include "RTC.h"


/* FUNCTIONS */
// Note that it's safer to disable NMIs when reading and/or writing to the RTC
int ReadRTC(int CMOSRegister)
{
    SelectCMOSRegister(CMOSRegister, false);
    return 0;
}

void WriteRTC()
{
    // It is EXTREMELY important the interrupts are disabled before we write to the RTC!
    // See https://wiki.osdev.org/RTC#Avoiding_NMI_and_Other_Interrupts_While_Programming.
    __asm__ volatile ("cli");
    __asm__ volatile ("sti");
}

// Select a CMOS register and set the NMI bit (EnableNMI is inverted because setting the NMI bit disabled NMIs)
void SelectCMOSRegister(int CMOSRegister, bool EnableNMI)
{
    OutB(CMOS_ADDR_REG, (!EnableNMI << 7) | (CMOSRegister));
}