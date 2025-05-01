/* LIBRARIES */
#include <stdint.h>
#include "Core/IO/RegisterIO.h"
#include "Core/Devices/PIT.h"


/* FUNCTIONS */
void PCSpeakerPlayTone(uint32_t FrequencyHz)
{
    SetPITFrequency(FrequencyHz);
 
    // Play the sound.
    uint8_t TReg = InB(0x61);

  	if (TReg != (TReg | 3))
    {
 		    OutB(0x61, TReg | 3);
 	  }
}

void TurnOffPCSpeaker()
{
    OutB(0x61, InB(0x61) & 0xFC);
}