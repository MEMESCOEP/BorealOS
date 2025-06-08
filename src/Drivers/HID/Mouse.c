/* LIBRARIES */
#include <Utilities/StrUtils.h>
#include <Drivers/HID/Mouse.h>
#include <Core/Graphics/Console.h>
#include <Core/IO/PS2Controller.h>
#include <Core/IO/RegisterIO.h>


/* VARIABLES */
struct MouseState CurrentMouseState;
uint8_t ACKResponse;
uint8_t MouseCycle = 0;
uint8_t MaxDelta = 200;
uint8_t MouseID = 0;
int ScreenDimensions[2] = {0, 0};
int MousePos[2] = {0, 0};
int XSensitivity = 1;
int YSensitivity = 1;


/* FUNCTIONS */
uint8_t GetMouseID()
{
    MouseWrite(0xF2);
    MouseWait(1);
    MouseRead();
    return MouseRead();
}

uint8_t MouseRead()
{
	MouseWait(0);
	return InB(PS2_MOUSE_DATA_REG);
}

void PS2MouseHandler()
{
    uint8_t Status = InB(PS2_MOUSE_STATUS_REG);

    // Make sure data is actually available by checking bit 0; it should always be 1 if the packet has data.
    if ((Status & 0x01) != 1)
    {
        return;
    }

    // Validate the packet by checking if bit 5 is set, as it always should be.
    // See the status byte section of the osdev mouse input wiki page:
    // https://wiki.osdev.org/Mouse_Input#Additional_Useless_Mouse_Commands
    if ((Status & 0x20) == 0)
    {
        return;
    }

    switch (MouseCycle)
    {
        // Mouse byte #1, state byte.
        case 0:
            uint8_t State = MouseRead();

            //CurrentMouseState.ScrollDelta = 0;
            CurrentMouseState.DeltaX = 0;
            CurrentMouseState.DeltaY = 0;

            // Verify the state byte.
            if (State == 0x00)
            {
                break;
            }

            if (!(State & 0x08))
            {
                /*DrawString(0, 0, "PS/2 mouse state byte verification failure:", 0xFFFFFF, 0x161616);
                
                for (int i = sizeof(State) - 1; i >= 0; i--)
                {
                    // Check the ith bit of num
                    if ((State >> i) & 1)
                    {
                        DrawString(i * 8, 16, "1", 0xFFFFFF, 0x161616);
                    }
                    else
                    {
                        DrawString(i * 8, 16, "0", 0xFFFFFF, 0x161616);
                    }
                }*/

                break;
            }

            // Discard the packet if the X and/or Y overflow bits are set.
            if ((State & 0x80) != 0 || (State & 0x40) != 0)
            {
                break;
            }

            // Check bits 00000111 (In order from right to left: left, right, middle).
            CurrentMouseState.LeftButtonPressed = (State & 0x01) != 0;
            CurrentMouseState.RightButtonPressed = (State & 0x02) != 0;
            CurrentMouseState.MiddleButtonPressed = (State & 0x04) != 0;
            MouseCycle++;

            break;

        // Mouse byte #2, X delta.
        case 1:
            CurrentMouseState.DeltaX = MouseRead();
            CurrentMouseState.DeltaSigns[0] = (CurrentMouseState.DeltaX > 0) - (CurrentMouseState.DeltaX < 0);
            MouseCycle++;
            break;

        // Mouse byte #3, Y delta.
        case 2:
            CurrentMouseState.DeltaY = MouseRead();
            CurrentMouseState.DeltaSigns[1] = (CurrentMouseState.DeltaY > 0) - (CurrentMouseState.DeltaY < 0);

            // Clamp the mouse delta to any int between -MaxDelta and MaxDelta, and add it to the current mouse position
            MousePos[0] += IntMin(IntMax(CurrentMouseState.DeltaX, -MaxDelta), MaxDelta) * XSensitivity;
            MousePos[1] -= IntMin(IntMax(CurrentMouseState.DeltaY, -MaxDelta), MaxDelta) * YSensitivity;

            // Ensure the mouse position is inside of the screen boundaries.
            MousePos[0] = IntMin(IntMax(MousePos[0], 0), ScreenDimensions[0]);
            MousePos[1] = IntMin(IntMax(MousePos[1], 0), ScreenDimensions[1]);
            
            if (MouseID != 3)
            {
                MouseCycle = 0;
            }
            else
            {
                MouseCycle++;
            }

            GfxDrawChar('#', MousePos[0], MousePos[1], 0xFFFFFF, 0x000000);
            break;

        // Optional mouse byte #4.
        case 3:
            CurrentMouseState.ScrollDelta = MouseRead();
            MouseCycle = 0;
            break;
    }
}

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

		//LOG_KERNEL_MSG("PS/2 mouse did not respond (BBIT)\n\r", WARN);
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

		//LOG_KERNEL_MSG("PS/2 mouse did not respond (ABIT)\n\r", WARN);
	}
}

void MouseWrite(uint8_t Data)
{
	MouseWait(1);
	OutB(PS2_MOUSE_STATUS_REG, PS2_MOUSE_WRITE);

	MouseWait(1);
	OutB(PS2_MOUSE_DATA_REG, Data);
}

void InitPS2Mouse()
{
    if (PS2Initialized == false)
    {
        LOG_KERNEL_MSG("\tThe PS/2 controller was not initialized properly, the PS/2 mouse cannot be initialized.\n\n\r", WARN);  
        return;
    }

    LOG_KERNEL_MSG("\tConfiguring interrupt handler...\n\r", NONE);
    IDTSetIRQHandler(12, PS2MouseHandler);

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

    // Change the mouse ID to 0x03 from 0x00. This enables the Z-axis (scroll wheel) extension
    LOG_KERNEL_MSG("\tEnabling IntelliMouse Z-axis (scroll wheel) extension...\n\r", NONE);
    LOG_KERNEL_MSG("\t\tChanging mouse ID to 0x03 from 0x00...\n\r", NONE);
    MouseWrite(0xF3);
    MouseRead();
    MouseWrite(200);
    MouseRead();

    MouseWrite(0xF3);
    MouseRead();
    MouseWrite(100);
    MouseRead();

    MouseWrite(0xF3);
    MouseRead();
    MouseWrite(80);
    MouseRead();

    MouseID = GetMouseID();

    if (MouseID == 0x03)
    {
        LOG_KERNEL_MSG("\t\tIntelliMouse Z-axis extension enabled.\n\n\r", NONE);

        // Now change the mouse ID from 0x03 to 0x04 with 4 1-byte packets. This enables the 4th and 5th buttons
        LOG_KERNEL_MSG("\tEnabling IntelliMouse 5-button extension...\n\r", NONE);
        LOG_KERNEL_MSG("\t\tChanging mouse ID to 0x04 from 0x03...\n\r", NONE);
        MouseWrite(0xF3);
        MouseRead();
        MouseWrite(200);
        MouseRead();

        MouseWrite(0xF3);
        MouseRead();
        MouseWrite(200);
        MouseRead();

        MouseWrite(0xF3);
        MouseRead();
        MouseWrite(80);
        MouseRead();

        MouseID = GetMouseID();

        if (MouseID != 0x04)
        {
            LOG_KERNEL_MSG("\t\tPS/2 Mouse ID did not change from 0x03 to 0x04, this is probably a 3-button mouse.\n\n\r", WARN);
        }
        else
        {
            LOG_KERNEL_MSG("\t\tIntelliMouse 5-button extensions enabled.\n\n\r", NONE);
        }
    }
    else
    {
        LOG_KERNEL_MSG("\t\tPS/2 Mouse ID did not change from 0x00 to 0x03, this mouse probably has no scroll wheel.\n\n\r", WARN);
    }

    // Set the mouse sample rate to 60/s.
    LOG_KERNEL_MSG("\tSetting mouse sample rate to 60/s...\n\r", NONE);
    MouseWrite(0xF3);
    MouseRead();
    MouseWrite(60);
    MouseRead();

    // Set the mouse resolution to 4 counts per millimeter.
    LOG_KERNEL_MSG("\tSetting mouse resolution to 4 counts/mm...\n\r", NONE);
    MouseWrite(0xE8);
    MouseRead();
    MouseWrite(0x02);
    MouseRead();

    // Enable the mouse's data reporting (command 0xF4) and check for ACK response.
    LOG_KERNEL_MSG("\tEnabling packet streaming...\n\r", NONE);
    MouseWrite(0xF4);

    ACKResponse = MouseRead();

    if (ACKResponse != PS2_MOUSE_ACK)
    {
        LOG_KERNEL_MSG("\tPS/2 mouse failed to acknowledge packet streaming command (0xF4), init will be stopped.\n\r", WARN);
        return;
    }

    LOG_KERNEL_MSG("\tGetting framebuffer dimensions...\n\r", NONE);
    GfxGetDimensions(&ScreenDimensions[0], &ScreenDimensions[1]);

    LOG_KERNEL_MSG("\tUnmasking mouse IRQs (#2 & #12)...\n\r", NONE);
    PICClearIRQMask(2);
    PICClearIRQMask(12);

    LOG_KERNEL_MSG("\tPS/2 mouse init done.\n\n\r", NONE);
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