/* LIBRARIES */
#include <stdint.h>
#include "Core/IO/RegisterIO.h"
#include "PIT.h"


/* FUNCTIONS */
void SetPITFrequency(uint32_t FrequencyHz)
{
    // Calculate the divisor.
    uint32_t Divisor = 1193180 / FrequencyHz; 	
 
    //Set the PIT to the desired frequency.
 	OutB(0x43, 0xB6);
 	OutB(0x42, (uint8_t) (Divisor) );
 	OutB(0x42, (uint8_t) (Divisor >> 8));
}