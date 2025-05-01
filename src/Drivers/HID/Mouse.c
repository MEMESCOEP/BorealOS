/* LIBRARIES */
#include "Utilities/StrUtils.h"
#include "Core/Graphics/Terminal.h"
#include "Core/Devices/PIC.h"
#include "Core/IO/RegisterIO.h"
#include "Kernel.h"
#include "Mouse.h"


/* VARIABLES */
uint8_t ACKResponse;
uint8_t MouseID = 0;
int MousePos[2] = {0, 0};
int XSensitivity = 1;
int YSensitivity = 1;


/* FUNCTIONS */
void MouseWait(uint8_t MType)
{
	uint32_t Timeout = PS2_MOUSE_TIMEOUT;

	if (!MType)
    {
		while (--Timeout)
        {
			if ((InB(PS2_MOUSE_STATUS_REG) & PS2_MOUSE_BBIT) == 1)
            {
				return;
			}
		}

		TerminalDrawString("[WARN] >> PS/2 mouse did not respond (BBIT)\n\r");
	}
    else
    {
		while (--Timeout)
        {
			if (!((InB(PS2_MOUSE_STATUS_REG) & PS2_MOUSE_ABIT)))
            {
				return;
			}
		}

		TerminalDrawString("[WARN] >> PS/2 mouse did not respond (ABIT)\n\r");
	}
}

void MouseWrite(uint8_t Data)
{
	MouseWait(1);
	OutB(PS2_MOUSE_STATUS_REG, PS2_MOUSE_WRITE);

	MouseWait(1);
	OutB(PS2_MOUSE_DATA_REG, Data);
}

uint8_t MouseRead()
{
	MouseWait(0);
	return InB(PS2_MOUSE_DATA_REG);
}

void InitPS2Mouse()
{
    MouseWait(1);
    OutB(PS2_MOUSE_STATUS_REG, 0xA8);

    MouseWait(1);
    OutB(PS2_MOUSE_STATUS_REG, 0x20);

    MouseWait(0);
	uint8_t Status = InB(PS2_MOUSE_DATA_REG) | 2;

    MouseWait(1);
    OutB(PS2_MOUSE_STATUS_REG, PS2_MOUSE_DATA_REG);

    MouseWait(1);
	OutB(PS2_MOUSE_DATA_REG, Status);

    // Change the mouse ID to 0x03 from 0x00.
    TerminalDrawString("[INFO] >> Changing PS/2 mouse ID to 0x03 from 0x00...\n\r\tM_ID STAGE 1/1.\n\r");
    MouseWrite(0xF3);
    MouseRead();
    MouseWrite(200);
    MouseRead();

    TerminalDrawString("\tM_ID STAGE 1/2.\n\r");
    MouseWrite(0xF3);
    MouseRead();
    MouseWrite(100);
    MouseRead();

    TerminalDrawString("\tM_ID STAGE 1/3.\n\r");
    MouseWrite(0xF3);
    MouseRead();
    MouseWrite(80);
    MouseRead();

    TerminalDrawString("\tM_ID STAGE 1/4.\n\r");
    MouseWrite(0xF2);
    MouseWait(1);
    MouseRead();
    MouseID = MouseRead();
    char MIDBuffer[8];

    IntToStr(MouseID, MIDBuffer, 16);
    TerminalDrawString("\n\r[INFO] >> Mouse ID is: 0x");
    TerminalDrawString(MIDBuffer);
    TerminalDrawString("\n\r");

    if (MouseID == 0x03)
    {
        // Now change the mouse ID from 0x03 to 0x04 with 4 1-byte packets.
        TerminalDrawString("[INFO] >> Changing PS/2 mouse ID to 0x04 form 0x03...\n\r");
        TerminalDrawString("\tM_ID STAGE 2/1.\n\r");
        MouseWrite(0xF3);
        MouseRead();
        MouseWrite(200);
        MouseRead();

        TerminalDrawString("\tM_ID STAGE 2/2.\n\r");
        MouseWrite(0xF3);
        MouseRead();
        MouseWrite(200);
        MouseRead();

        TerminalDrawString("\tM_ID STAGE 2/3.\n\r");
        MouseWrite(0xF3);
        MouseRead();
        MouseWrite(80);
        MouseRead();

        //MouseID = 0;
        MouseWrite(0xF2);
        MouseWait(1);
        MouseRead();
        MouseID = MouseRead();

        IntToStr(MouseID, MIDBuffer, 16);
        TerminalDrawString("\n\r[INFO] >> Mouse ID is: 0x");
        TerminalDrawString(MIDBuffer);
        TerminalDrawString("\n\r");

        if (MouseID != 0x04)
        {
            TerminalDrawString("[WARN] >> PS/2 Mouse ID did not change from 0x03 to 0x04, this is probably a 3-button mouse.\n\r");
        }
    }
    else
    {
        TerminalDrawString("[WARN] >> PS/2 Mouse ID did not change from 0x00 to 0x03, this mouse probably has no scroll wheel.\n\r");
    }

    // Set the mouse sample rate to 60/s.
    TerminalDrawString("[INFO] >> Setting mouse sample rate to 60/s...\n\r");
    MouseWrite(0xF3);
    MouseRead();
    MouseWrite(60);
    MouseRead();

    // Set the mouse resolution to 4 counts per millimeter.
    TerminalDrawString("[INFO] >> Setting mouse resolution to 4 counts/mm...\n\r");
    MouseWrite(0xE8);
    MouseRead();
    MouseWrite(0x02);
    MouseRead();

    // Enable the mouse's data reporting (command 0xF4) and check for ACK response.
    TerminalDrawString("[INFO] >> Enabling PS/2 mouse packet streaming...\n\r");
    MouseWrite(0xF4);

    ACKResponse = MouseRead();

    if (ACKResponse != PS2_MOUSE_ACK)
    {
        TerminalDrawString("[INFO] >> PS/2 mouse failed to acknowledge packet streaming command (0xF4).\n\r");
        return;
    }

    TerminalDrawString("[INFO] >> Unmasking PS/2 mouse IRQs (#2 & #12)...\n\r");
    ClearIRQMask(2);
    ClearIRQMask(12);
}

void PS2SetSampleRate(int SampleRate)
{
    OutB(0xD4, 0x64);                    // tell the controller to address the mouse
    OutB(0xF3, 0x60);                    // write the mouse command code to the controller's data port
    MouseWait(1);
    
    int Ack = InB(0x60);                 // read back acknowledge. This should be 0xFA

    if (Ack != 0xFA)
    {
        KernelPanic(Ack, "PS/2 mouse did not acknowledge SR command!");
    }

    OutB(0xD4, 0x64);                    // tell the controller to address the mouse
    OutB(SampleRate, 0x60);              // write the parameter to the controller's data port
    
    MouseWait(1);
    Ack = InB(0x60);                     // read back acknowledge. This should be 0xFA

    if (Ack != 0xFA)
    {
        KernelPanic(Ack, "PS/2 mouse did not acknowledge SR_PARAM command!");
    }
}