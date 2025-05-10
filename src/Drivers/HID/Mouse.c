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

		TerminalDrawMessage("PS/2 mouse did not respond (BBIT)\n\r", WARNING);
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

		TerminalDrawMessage("PS/2 mouse did not respond (ABIT)\n\r", WARNING);
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
    TerminalDrawMessage("Changing PS/2 mouse ID to 0x03 from 0x00...\n\r\tM_ID STAGE 1/1.\n\r", INFO);
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
    TerminalDrawString("\n\r");
    TerminalDrawMessage("Mouse ID is: 0x", INFO);
    TerminalDrawString(MIDBuffer);
    TerminalDrawString("\n\r");

    if (MouseID == 0x03)
    {
        // Now change the mouse ID from 0x03 to 0x04 with 4 1-byte packets.
        TerminalDrawMessage("Changing PS/2 mouse ID to 0x04 form 0x03...\n\r", INFO);
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
        TerminalDrawString("\n\r");
        TerminalDrawMessage("Mouse ID is: 0x", INFO);
        TerminalDrawString(MIDBuffer);
        TerminalDrawString("\n\r");

        if (MouseID != 0x04)
        {
            TerminalDrawMessage("PS/2 Mouse ID did not change from 0x03 to 0x04, this is probably a 3-button mouse.\n\r", WARNING);
        }
    }
    else
    {
        TerminalDrawMessage("PS/2 Mouse ID did not change from 0x00 to 0x03, this mouse probably has no scroll wheel.\n\r", WARNING);
    }

    // Set the mouse sample rate to 60/s.
    TerminalDrawMessage("Setting mouse sample rate to 60/s...\n\r", INFO);
    MouseWrite(0xF3);
    MouseRead();
    MouseWrite(60);
    MouseRead();

    // Set the mouse resolution to 4 counts per millimeter.
    TerminalDrawMessage("Setting mouse resolution to 4 counts/mm...\n\r", INFO);
    MouseWrite(0xE8);
    MouseRead();
    MouseWrite(0x02);
    MouseRead();

    // Enable the mouse's data reporting (command 0xF4) and check for ACK response.
    TerminalDrawMessage("Enabling PS/2 mouse packet streaming...\n\r", INFO);
    MouseWrite(0xF4);

    ACKResponse = MouseRead();

    if (ACKResponse != PS2_MOUSE_ACK)
    {
        TerminalDrawMessage("PS/2 mouse failed to acknowledge packet streaming command (0xF4).\n\r", INFO);
        return;
    }

    TerminalDrawMessage("Unmasking PS/2 mouse IRQs (#2 & #12)...\n\r", INFO);
    PICClearIRQMask(2);
    PICClearIRQMask(12);
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