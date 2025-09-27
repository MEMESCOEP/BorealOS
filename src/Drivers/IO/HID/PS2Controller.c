#include "PS2Controller.h"
#include "Utility/SerialOperations.h"
#include "Core/Firmware/ACPI.h"
#include "Core/Kernel.h"

uint8_t testResult = 255;
bool dualChannelController = false;
bool port1Works = true;
bool port2Works = true;

// Poll bit 1 (input buffer full/clear) of the status byte
Status PS2InbufWait(bool waitForClear)
{
    int Timeout = PS2_TRX_TIMEOUT;
    
    while(((inb(PS2_CMD_STATUS_REGISTER) & (1 << PS2_INBUF_CLEAR_BIT)) != 0) == waitForClear)
    {
        if (Timeout <= 0)
        {
            return STATUS_TIMEOUT;
        }

        Timeout--;
    };

    return STATUS_SUCCESS;
}

Status PS2SendDataToDevice(uint8_t data, bool writeToPort2)
{
    // Tell the PS/2 controller to redirect the PS/2 data register (0x60) to the second port
    if (writeToPort2 == true)
    {
        outb(PS2_CMD_STATUS_REGISTER, PS2_WRITE_TO_2ND_PORT_CMD);
    }

    // Poll bit 1 (input buffer full/clear) of the status byte until it's clear (buffer empty), to see if we can start sending data out to connected devices
    if (PS2InbufWait(true) != STATUS_SUCCESS)
    {
        return STATUS_TIMEOUT;
    }

    // Send the data out to the connected device(s)
    outb(PS2_DATA_REGISTER, data);
    return STATUS_SUCCESS;
}

Status PS2ControllerInit() {
    LOG(LOG_INFO, "Initializing PS/2 controller...\n");
    // Check if the PS/2 controller exists in this system
    // NOTE: This is necessary because PS/2 is an old and mostly phased out connector; attempts to access the controller on devices where it does not exist
    // may lead to crashes!
    // NOTE: This has been commented out because the ACPI fields haven't been mapped by the memory managers yet, in order to prevent page faults
    /*if (ACPI.Initialized == true)
    {
        LOG(LOG_INFO, "Checking for PS/2 controller in ACPI FADT...\n");

        if ((ACPI.FADT->BootArchitectureFlags & (1 << 1)) == 0)
        {
            LOG(LOG_WARNING, "This system does not have a PS/2 controller, PS/2 controller initialization cannot proceed.\n");
            return STATUS_UNSUPPORTED;
        }
    }
    else
    {
        LOG(LOG_WARNING, "ACPI is not available to check for a PS/2 controller, PS/2 controller initialization cannot proceed.\n");
    }*/



    // Disable PS/2 devices so they can't mess with initialization
    // NOTE: It's okay to send the disable command to port 2 without checking if it exists in this case because single-channel (one port only) PS/2
    // controllers will just ignore the 0xA7 command
    PRINT("\t* Disabling connected PS/2 device(s)...\n");
    outb(PS2_CMD_STATUS_REGISTER, 0xAD);
    outb(PS2_CMD_STATUS_REGISTER, 0xA7);



    // Flush the output buffer so we start in a fresh, known state
    PRINT("\t* Flushing PS/2 controller output buffer...\n");
    inb(PS2_DATA_REGISTER);



    // Disable IRQs and translation and enable the PS/2 clock by clearing bits 0, 6, and 4 in the configuration byte
    PRINT("\t* Disabling IRQs & translation and enabling clock signal for PS/2 port(s)...\n");
    outb(PS2_CMD_STATUS_REGISTER, 0x20);
    uint8_t configByte = inb(PS2_DATA_REGISTER);
    configByte &= ~( (1 << 0) | (1 << 6) | (1 << 4) );

    outb(PS2_CMD_STATUS_REGISTER, 0x60);
    outb(PS2_DATA_REGISTER, configByte);



    // Perform a self test on the PS/2 controller, it should return 0x55 to indicate a successful test
    PRINT("\t* Performing self-test on PS/2 controller...\n");
    outb(PS2_CMD_STATUS_REGISTER, 0xAA);
    testResult = inb(PS2_DATA_REGISTER);

    if (testResult != PS2_CONTROLLER_TEST_SUCCESS)
    {
        LOGF(LOG_WARNING, "PS/2 controller self-test failed! Expected %p but got %p\n", PS2_CONTROLLER_TEST_SUCCESS, testResult);
        return STATUS_FAILURE;
    }



    // Determine if the controller is dual-channel (2 ports) or single-channel (1 port) by enabling the 2nd port and checking if bit 5 is clear in
    // the config byte
    PRINT("\t* Determining PS/2 controller mode...\n");
    outb(PS2_CMD_STATUS_REGISTER, 0xA8);
    outb(PS2_CMD_STATUS_REGISTER, 0x20);
    configByte = inb(PS2_DATA_REGISTER);

    if (!(configByte & (1 << PS2_DUAL_CHANNEL_BIT)))
    {
        // It's a dual-channel controller, we have to disable the 2nd port and IRQs again by issuing command 0xA7 and setting bits 1 & 5 in the config byte
        dualChannelController = true;
        outb(PS2_CMD_STATUS_REGISTER, 0xA7);
        configByte &= ~( (1 << 1) | (1 << 5) );

        outb(PS2_CMD_STATUS_REGISTER, 0x60);
        outb(PS2_DATA_REGISTER, configByte);
    }

    PRINTF("\t\t* PS/2 controller is operating in %s-channel mode.\n\n", dualChannelController == true ? "dual" : "single");



    // Perform tests on each port the controller has
    PRINT("\t* Performing interface tests on working PS/2 port(s)...\n");

    // Port 1
    outb(PS2_CMD_STATUS_REGISTER, 0xAB);
    testResult = inb(PS2_DATA_REGISTER);

    if (testResult != PS2_INTERFACE_TEST_SUCCESS)
    {
        LOGF(LOG_WARNING, "Interface on PS/2 port #1 failed! Expected %p but got %p\n", PS2_INTERFACE_TEST_SUCCESS, testResult);
        port1Works = false;
    }

    if (dualChannelController == true)
    {
        outb(PS2_CMD_STATUS_REGISTER, 0xA9);
        testResult = inb(PS2_DATA_REGISTER);

        if (testResult != PS2_INTERFACE_TEST_SUCCESS)
        {
            LOGF(LOG_WARNING, "Interface on PS/2 port #2 failed! Expected %p but got %p\n", PS2_INTERFACE_TEST_SUCCESS, testResult);
            port2Works = false;
        }
    }

    // Make sure there's at least one working port before we proceed
    if (port1Works == false && port2Works == false)
    {
        LOG(LOG_WARNING, "No PS/2 ports passed the interface tests, PS/2 controller init cannot succeed.\n");
        return STATUS_FAILURE;
    }



    // Enable the working PS/2 port(s)
    PRINT("\t* Enabling working PS/2 port(s)...\n");
    if (port1Works == true)
    {
        outb(PS2_CMD_STATUS_REGISTER, 0xAE);
        configByte |= (1 << 0);
    }

    if (port2Works == true)
    {
        outb(PS2_CMD_STATUS_REGISTER, 0xA8);
        configByte |= (1 << 1);
    }

    outb(PS2_CMD_STATUS_REGISTER, 0x60);
    outb(PS2_DATA_REGISTER, configByte);



    // Reset the devices connected to the working port(s) (if any)
    PRINT("\t* Resetting device(s) on PS/2 port(s)...\n");
    if (port1Works == true)
    {
        // Send the reset command
        Status sendStatus = PS2SendDataToDevice(0xFF, false);

        if (sendStatus != STATUS_SUCCESS)
        {
            LOGF(LOG_WARNING, "Device reset on PS/2 port #1 failed (status=%d)!\n", sendStatus);
        }
        else
        {
            // Check the response
            uint8_t resetResponse = inb(PS2_DATA_REGISTER);

            if (resetResponse != PS2_DEVICE_RESET_SUCCESS_RESPONSE_1 && resetResponse != PS2_DEVICE_RESET_SUCCESS_RESPONSE_2)
            {
                LOGF(LOG_WARNING, "Device reset on PS/2 port #1 failed (status=%p)!\n", resetResponse);
            }
        }
    }

    if (port2Works == true)
    {
        // Send the reset command
        Status sendStatus = PS2SendDataToDevice(0xFF, true);

        if (sendStatus != STATUS_SUCCESS)
        {
            LOGF(LOG_WARNING, "Device reset on PS/2 port #2 failed (status=%d)!\n", sendStatus);
        }
        else
        {
            // Check the response
            uint8_t resetResponse = inb(PS2_DATA_REGISTER);

            if (resetResponse != PS2_DEVICE_RESET_SUCCESS_RESPONSE_1 && resetResponse != PS2_DEVICE_RESET_SUCCESS_RESPONSE_2)
            {
                LOGF(LOG_WARNING, "Device reset on PS/2 port #2 failed (status=%p)!\n", resetResponse);
            }
        }
    }

    PRINT("\n");
    return STATUS_SUCCESS;
}