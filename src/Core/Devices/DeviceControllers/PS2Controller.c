/* LIBRARIES */
#include <stdint.h>
#include "Utilities/StrUtils.h"
#include "Drivers/HID/Keyboard.h"
#include "Core/Graphics/Terminal.h"
#include "Core/Power/ACPI.h"
#include "Core/IO/RegisterIO.h"
#include "PS2Controller.h"
#include "Kernel.h"


/* FUNCTIONS */
void PS2Wait(uint8_t BitToCheck, bool WaitForSet)
{
    if (WaitForSet == true)
    {
        while (!(InB(0x64) & BitToCheck));
    }
    else
    {
        while (InB(0x64) & BitToCheck);
    }
}

void InitPS2Controller()
{
    // --- STEP 1 ---
    // Initialize USB controllers



    // --- STEP 2 ---
    // Determine if the PS/2 controller actually exists



    // --- STEPS 3 & 4 ---
    // Disable ports (1 and 2 in order) and then read from port 0x60 to flush the
    // PS/2 output buffer
    TerminalDrawString("[INFO] >> Disabling PS/2 ports and flushing PS/2 output buffer...\n\r");
    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0xAD);

    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0xA7);

    while (InB(0x64) & 0x01)
    {
        InB(PS2_DATA_PORT);
    }



    // --- STEP 5 ---
    // Set the PS/2 controller's command byte
    TerminalDrawString("[INFO] >> Configuring PS/2 controller's command byte...\n\r");
    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0x20);

    PS2Wait(0x01, true);
    uint8_t CommandByte = InB(PS2_DATA_PORT);
    CommandByte &= ~((1 << 0) | (1 << 4) | (1 << 6));

    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0x60);

    PS2Wait(0x02, false);
    OutB(PS2_DATA_PORT, CommandByte);



    // --- STEP 6 ---
    // Run a self test for the PS/2 controller
    TerminalDrawString("[INFO] >> Running PS/2 controller self test...\n\r");
    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0xAA);

    PS2Wait(0x01, true);
    uint8_t TestResult = InB(PS2_DATA_PORT);

    if (TestResult != 0x55)
    {
        KernelPanic(0, "PS/2 Controller self test failed: got 0x");
    }



    // --- STEP 7 ---
    // Perform interface tests for ports (1 and 2 in order, assume a dual-channel controller)
    // Port 1
    TerminalDrawString("[INFO] >> Testing PS/2 port 1...\n\r");
    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0xAB);

    PS2Wait(0x01, true);
    TestResult = InB(PS2_DATA_PORT);

    if (TestResult != 0x00)
    {
        KernelPanic(0, "PS/2 Controller port 1 test failed: got 0x");
    }

    // Port 2
    TerminalDrawString("[INFO] >> Testing PS/2 port 2...\n\r");
    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0xA9);

    PS2Wait(0x01, true);
    TestResult = InB(PS2_DATA_PORT);

    if (TestResult != 0x00)
    {
        KernelPanic(0, "PS/2 Controller port 2 test failed: got 0x");
    }



    // --- STEP 8 ---
    // Enable ports and IRQs (1 and 2 in order)
    // Ports
    TerminalDrawString("[INFO] >> Enabling PS/2 ports...\n\r");
    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0xAE);

    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0xA8);

    // IRQs
    TerminalDrawString("[INFO] >> Enabling PS/2 IRQs...\n\r");
    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0x20);

    PS2Wait(0x01, true);
    CommandByte = InB(PS2_DATA_PORT);
    CommandByte |= (1 << 0) | (1 << 1);

    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0x60);

    PS2Wait(0x02, false);
    OutB(PS2_DATA_PORT, CommandByte);



    // --- STEP 9 ---
    // Reset PS/2 devices
    // Port 1
    TerminalDrawString("[INFO] >> Resetting PS/2 device on port 1...\n\r");
    PS2Wait(0x02, false);
    OutB(PS2_DATA_PORT, 0xFF);

    PS2Wait(0x01, true);
    uint8_t ResetResult = InB(0x60);

    // Sometimes keyboards will send a 0xFA first before the result of the reset
    if (ResetResult == 0xFA)
    {
        PS2Wait(0x01, true);
        ResetResult = InB(0x60);
    }

    if (ResetResult != 0xAA)
    {
        char RRBuffer[4];
        char RRBString[48];

        IntToStr(ResetResult, RRBuffer, 16);
        StrCat("PS/2 KBINIT failed: got 0x", RRBuffer, RRBString);
        KernelPanic(0, RRBString);
    }

    InB(0x60);

    // Port 2
    TerminalDrawString("[INFO] >> Resetting PS/2 device on port 2...\n\r");
    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0xD4);
    
    PS2Wait(0x02, false);
    OutB(PS2_DATA_PORT, 0xFF);

    PS2Wait(0x01, true);
    ResetResult = InB(0x60);

    if (ResetResult == 0xFA)
    {
        PS2Wait(0x01, true);
        ResetResult = InB(0x60);
    }

    if (ResetResult != 0xAA)
    {
        char RRBuffer[4];
        char RRBString[48];

        IntToStr(ResetResult, RRBuffer, 16);
        StrCat("PS/2 MOUSEINIT failed: got 0x", RRBuffer, RRBString);
        KernelPanic(0, RRBString);
    }

    InB(0x60);
}

/*bool PS2ControllerExists()
{
    if (ACPIInitialized == false)
    {
        KernelPanic(0, "ACPI must be properly initialized before checking if the PS/2 controller exists!");
    }

    return false;
}*/