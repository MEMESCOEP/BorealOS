/* LIBRARIES */
#include <Utilities/MathUtils.h>
#include <Utilities/BitUtils.h>
#include <Utilities/StrUtils.h>
#include <Drivers/HID/Keyboard.h>
#include <Core/Graphics/Console.h>
#include <Core/Power/ACPI.h>
#include <Core/IO/PS2Controller.h>
#include <Core/IO/RegisterIO.h>
#include <stdint.h>


/* VARIABLES */
bool PS2Initialized = false;
bool Port1Useable = false;
bool Port2Useable = false;


/* FUNCTIONS */
bool PS2Wait(uint8_t BitToCheck, bool WaitForSet)
{
    int Timeout = 10000;

    if (WaitForSet == true)
    {
        while (!(InB(0x64) & BitToCheck))
        {
            if (Timeout <= 0)
            {
                return false;
            }

            Timeout--;
        }
    }
    else
    {
        while (InB(0x64) & BitToCheck)
        {
            if (Timeout <= 0)
            {
                return false;
            }

            Timeout--;
        }
    }

    return true;
}

void InitPS2Controller()
{
    // --- STEP 1 ---
    // Initialize USB controllers



    // --- STEP 2 ---
    // Determine if the PS/2 controller actually exists. This check is only valid for ACPI revisions 2.0 and later
    if (FADTStruct->SDTHeader.Length >= OffsetOf(FADT, BootArchitectureFlags) + sizeof(uint16_t))
    {
        LOG_KERNEL_MSG("\tChecking if PS/2 controller exists...\n\r", NONE);

        if (IS_BIT_SET(GetBootArchFlags(), 1) == false)
        {
            LOG_KERNEL_MSG("\tPS/2 controller does not exist according to FADT, attempting to check via probing...\n\r", WARN);

            // Wait for the PS/2 input buffer to be empty
            PS2Wait(0x02, false);

            // Send self-test command
            OutB(PS2_CMD_STATUS_PORT, 0xAA);

            // Wait for the PS/2 output buffer to be full
            PS2Wait(0x01, true);

            int ProbeResult = InB(PS2_DATA_PORT);

            if (ProbeResult != 0x55)
            {
                LOG_KERNEL_MSG("\tPS/2 controller does not exist, it will be skipped. Probing returned 0x", WARN);
                PrintSignedNum(ProbeResult, 10);
                ConsolePutString(".\n\n\r");
                return;
            }
            else
            {
                LOG_KERNEL_MSG("\tPS/2 controller probing succeeded (controller exists).\n\r", NONE);
            }
        }
    }
    else
    {
        LOG_KERNEL_MSG("\tFADT is too old or invalid, assuming PS/2 controller exists...\n\r", WARN);
    }



    // --- STEPS 3 & 4 ---
    // Disable ports (1 and 2 in order) and then read from port 0x60 to flush the
    // PS/2 output buffer
    LOG_KERNEL_MSG("\tDisabling PS/2 ports and flushing output buffer...\n\r", NONE);
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
    LOG_KERNEL_MSG("\tConfiguring command byte...\n\r", NONE);
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
    LOG_KERNEL_MSG("\tRunning PS/2 controller self test...\n\r", NONE);
    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0xAA);

    PS2Wait(0x01, true);
    uint8_t TestResult = InB(PS2_DATA_PORT);

    if (TestResult != 0x55)
    {
        LOG_KERNEL_MSG("\tPS/2 Controller self test failed: got 0x", ERROR);
        PrintUnsignedNum(TestResult, 16);
        ConsolePutString("\n\r");
        return;
    }



    // --- STEP 7 ---
    // Perform interface tests for ports (1 and 2 in order, assume a dual-channel controller)
    // Port 1
    LOG_KERNEL_MSG("\tTesting port 1...\n\r", NONE);
    Port1Useable = true;
    Port2Useable = true;

    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0xAB);

    PS2Wait(0x01, true);
    TestResult = InB(PS2_DATA_PORT);

    if (TestResult != 0x00)
    {
        LOG_KERNEL_MSG("\tPS/2 Controller port 1 test failed: got 0x", ERROR);
        PrintUnsignedNum(TestResult, 16);
        ConsolePutString("\n\r");
        Port1Useable = false;
    }

    // Port 2
    LOG_KERNEL_MSG("\tTesting port 2...\n\r", NONE);
    PS2Wait(0x02, false);
    OutB(PS2_CMD_STATUS_PORT, 0xA9);

    PS2Wait(0x01, true);
    TestResult = InB(PS2_DATA_PORT);

    if (TestResult != 0x00)
    {
        LOG_KERNEL_MSG("\tPS/2 Controller port 2 test failed: got 0x", ERROR);
        PrintUnsignedNum(TestResult, 16);
        ConsolePutString("\n\r");
        Port1Useable = false;
    }



    // --- STEP 8 ---
    // Enable ports and IRQs (1 and 2 in order)
    // Ports
    LOG_KERNEL_MSG("\tEnabling ports...\n\r", NONE);

    if (Port1Useable == true)
    {
        PS2Wait(0x02, false);
        OutB(PS2_CMD_STATUS_PORT, 0xAE);
    }
    else
    {
        LOG_KERNEL_MSG("\tPS/2 Port 1 won't be enabled, it's not useable.\n\r", WARN);
    }
    
    if (Port2Useable == true)
    {
        PS2Wait(0x02, false);
        OutB(PS2_CMD_STATUS_PORT, 0xA8);
    }
    else
    {
        LOG_KERNEL_MSG("\tPS/2 Port 2 won't be enabled, it's not useable.\n\r", WARN);    
    }

    // IRQs
    LOG_KERNEL_MSG("\tEnabling PS/2 IRQs...\n\r", NONE);
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
    if (Port1Useable == true)
    {
        LOG_KERNEL_MSG("\tResetting device on port 1...\n\r", NONE);
        PS2Wait(0x02, false);
        OutB(PS2_DATA_PORT, 0xFF);

        PS2Wait(0x01, true);
        uint8_t ResetResult = InB(0x60);
        bool GotSelfTest = false;
        bool PassedLoop = false;
        bool GotACK = false;

        for (int i = 0; i < 3; i++)
        {
            PS2Wait(0x01, true);  // wait for output buffer full

            ResetResult = InB(0x60);

            if (ResetResult == 0xFA)
            {
                GotACK = true;
            }
            else if (ResetResult == 0xAA)
            {
                GotSelfTest = true;
            }
            else
            {
                LOG_KERNEL_MSG("\tUnexpected response: 0x", WARN);
                PrintUnsignedNum(ResetResult, 16);
                ConsolePutString("\n\r");
            }

            if (GotACK && GotSelfTest)
            {
                PassedLoop = true;
                break;
            }
        }

        if (PassedLoop == false && GotSelfTest == false && GotACK == false)
        {
            LOG_KERNEL_MSG("\tFailed to reset PS/2 keyboard!\n\r", ERROR);
        }
    }
    else
    {
        LOG_KERNEL_MSG("\tCan't reset device on port 1, the port isn't useable.\n\r", WARN);
    }

    // Port 2
    if (Port2Useable == true)
    {
        LOG_KERNEL_MSG("\tResetting device on port 2...\n\r", NONE);
        PS2Wait(0x02, false);
        OutB(PS2_CMD_STATUS_PORT, 0xD4);
        
        PS2Wait(0x02, false);
        OutB(PS2_DATA_PORT, 0xFF);

        PS2Wait(0x01, true);
        uint8_t ResetResult = InB(0x60);

        if (ResetResult == 0xFA)
        {
            PS2Wait(0x01, true);
            ResetResult = InB(0x60);
        }

        if (ResetResult != 0xAA)
        {
            LOG_KERNEL_MSG("\tPS/2 MOUSEINIT failed: got 0x", ERROR);
            PrintUnsignedNum(ResetResult, 16);
            ConsolePutString("\n\r");
        }
        else
        {
            InB(0x60);
        }    
    }
    else
    {
        LOG_KERNEL_MSG("\tCan't reset device on port 2, the port isn't useable.\n\r", WARN);
    }

    PS2Initialized = true;
    LOG_KERNEL_MSG("\tPS/2 controller init finished.\n\n\r", NONE);
}