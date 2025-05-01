/* LIBRARIES */
#include <stdint.h>
#include "Utilities/StrUtils.h"
#include "Core/Graphics/Terminal.h"
#include "Core/Power/ACPI.h"
#include "Core/IO/RegisterIO.h"
#include "PS2Controller.h"
#include "Kernel.h"


/* FUNCTIONS */
void InitPS2Controller()
{
    // Reinitialize the PS/2 controller.
    TerminalDrawString("[INFO] >> Disabling PS/2 devices...\n\r");
    // Disable both devices
    OutB(0x64, 0xAD);
    OutB(0x64, 0xA7);

    // Clear the buffer
    TerminalDrawString("[INFO] >> Clearing PS/2 buffer...\n\r");
    InB(0x60);

    // Set the controller's config byte
    TerminalDrawString("[INFO] >> Setting PS/2 controller's config byte...\n\r");
    uint8_t ConfigByte = InB(0x20) & 0xFB;
    OutB(0x60, ConfigByte);

    // Perform a self-test on the PS/2 controller; any value other than 0x55 means the test failed
    TerminalDrawString("[INFO] >> Performing self-test on PS/2 controller (MUST RETURN 0x55)...\n\r");
    OutB(0x64, 0xAA);
    
    while ((InB(0x64) & 0x01) == 0)
    {
        TerminalDrawString("Waiting for response...\r");
        // The controller is not ready to respond, continue waiting
        // Optionally, add a timeout or handle this scenario as needed
    }

    uint8_t TestResult = InB(0x60);
    
    // Some PS/2 controllers send 0xFA, 0xAA
    if (TestResult == 0xFA)
    {
        TestResult = InB(0x60);
    }

    if (TestResult != 0x55)
    {
        char Response[4];
        char EXCBuffer[32];
        IntToStr(TestResult, Response, 16);
        StrCat("PS/2 self test failed, got 0x", Response, EXCBuffer);
        KernelPanic(0, EXCBuffer);
    }

    // Check if this is a dual-channel PS/2 controller
    TerminalDrawString("[INFO] >> Checking if the PS/2 controller has 2 channels...\n\r");
    OutB(0x64, 0xA8);

    uint8_t DualChannelController = InB(0x20);
    
    if (DualChannelController & (1 << 5))
    {
        KernelPanic(0, "This is not a dual-channel PS/2 controller!");
    }

    // Test each port
    // Port 1
    TerminalDrawString("[INFO] >> Testing PS/2 port 1...\n\r");
    OutB(0x64, 0xAB);

    while ((InB(0x64) & 0x01) == 0)
    {
        TerminalDrawString("Waiting for response...\r");
        // The controller is not ready to respond, continue waiting
        // Optionally, add a timeout or handle this scenario as needed
    }

    if (InB(0x60) != 0x00)
    {
        KernelPanic(0, "PS/2 Port 1 test failed!");
    }

    // Port 2
    TerminalDrawString("[INFO] >> Testing PS/2 port 2...\n\r");
    OutB(0x64, 0xA9);

    while ((InB(0x64) & 0x01) == 0)
    {
        TerminalDrawString("Waiting for response...\r");
        // The controller is not ready to respond, continue waiting
        // Optionally, add a timeout or handle this scenario as needed
    }

    if (InB(0x60) != 0x00)
    {
        KernelPanic(0, "PS/2 Port 2 test failed!");
    }










    // Enable both ports again (1 and 2 in order)
    TerminalDrawString("[INFO] >> Enabling PS/2 ports...\n\r");
    OutB(0x64, 0xAE);
    //OutB(0x64, 0xA8);

    //while ((InB(0x64) & 0x01) == 0);
    uint8_t Response = InB(0x60);
    
    if (Response != 0xFA) {
        // Handle error: no ACK response for enabling port 1
        //KernelPanic(-1, "Failed to enable PS/2 keyboard port.");
        char ResponseBuffer[4];
        char EXCBuffer[32];
        IntToStr(Response, ResponseBuffer, 16);
        StrCat("PS/2 KBINIT failed, got 0x", ResponseBuffer, EXCBuffer);
        KernelPanic(0, EXCBuffer);
    }

    OutB(0x64, 0xA8);
    //while ((InB(0x64) & 0x01) == 0);
    Response = InB(0x60);
    
    if (Response != 0xFA) {
        // Handle error: no ACK response for enabling port 1
        //KernelPanic(-1, "Failed to enable PS/2 keyboard port.");
        char ResponseBuffer[4];
        char EXCBuffer[32];
        IntToStr(Response, ResponseBuffer, 16);
        StrCat("PS/2 MouseINIT failed, got 0x", ResponseBuffer, EXCBuffer);
        KernelPanic(0, EXCBuffer);
    }






    





    // Enable IRQs for both ports by settings bits 0 (first port) and 1 (second port) in the controller's config byte
    TerminalDrawString("[INFO] >> Enabling PS/2 IRQs...\n\r");
    ConfigByte = InB(0x20) | 0x03;
    OutB(0x60, ConfigByte);

    // Reset all PS/2 devices (1 and 2 in order)
    TerminalDrawString("[INFO] >> Resetting all PS/2 devices...\n\r");
    OutB(0x60, 0xFF);

    //while ((InB(0x64) & (1 << 1)) != 0);
    while ((InB(0x64) & 0x01) == 0)
    {
        TerminalDrawString("Waiting for response...\r");
        // The controller is not ready to respond, continue waiting
        // Optionally, add a timeout or handle this scenario as needed
    }

    uint8_t ResetResponse = InB(0x64);

    if (ResetResponse == 0xFC)
    {
        KernelPanic(0, "Failed to reset PS/2 device at port 1!");
    }
    else if (ResetResponse == 0x00)
    {
        TerminalDrawString("[WARN] >> No devices responded to the reset command on PS/2 port 1, there may not be anything connected.");
    }

    OutB(0x64, 0xD4);
    OutB(0x60, 0xFF);

    while ((InB(0x64) & (1 << 1)) != 0);

    ResetResponse = InB(0x64);

    if (ResetResponse == 0xFC)
    {
        KernelPanic(0, "Failed to reset PS/2 device at port 2!");
    }
    else if (ResetResponse == 0x00)
    {
        TerminalDrawString("[WARN] >> No devices responded to the reset command on PS/2 port 2, there may not be anything connected.");
    }
}

/*bool PS2ControllerExists()
{
    if (ACPIInitialized == false)
    {
        KernelPanic(0, "ACPI must be properly initialized before checking if the PS/2 controller exists!");
    }

    return false;
}*/

void PS2Wait()
{
    while ((InB(0x64) & 1 << 1) != 0);
}