/* LIBRARIES */
#include "RTC.h"


/* FUNCTIONS */
void ReadRTC()
{
    // It is EXTREMELY important the interrupts are disabled before we write to the RTC!
    // See https://wiki.osdev.org/RTC#Avoiding_NMI_and_Other_Interrupts_While_Programming.
    __asm__ volatile ("cli");
    __asm__ volatile ("sti");
}