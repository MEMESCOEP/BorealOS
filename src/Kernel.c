/* [== BOREALOS KERNEL ==] */
// This is the main kernel file, responsible for initializing and configuring everything.
// Think of it as the "master" controller of the OS; it directs everything and acts
// as the glue that holds everything together.


/* LIBRARIES */
#include <xmmintrin.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <Limine.h>
#include "Utilities/StrUtils.h"
#include "Drivers/HID/Keyboard.h"
#include "Drivers/HID/Mouse.h"
#include "Drivers/IO/Serial.h"
#include "Core/Applications/Shell.h"
#include "Core/Interrupts/IDT_ISR.h"
#include "Core/Interrupts/GDT.h"
#include "Core/Graphics/Terminal.h"
#include "Core/Graphics/Graphics.h"
#include "Core/Devices/DeviceControllers/PS2Controller.h"
#include "Core/Devices/PIC.h"
#include "Core/Devices/FPU.h"
#include "Core/Power/ACPI.h"
#include "Core/IO/RegisterIO.h"
#include "Core/IO/Memory.h"
#include "Kernel.h"


/* VARIABLES */
struct limine_framebuffer* FBInfo;
char* FirmwareTypes[4] = {
    "BIOS",
    "32-bit UEFI",
    "64-bit UEFI",
    "SBI"
};

char* FirmwareType = "Unknown";
char FBResWidthBuffer[8] = "";
char FBResHeightBuffer[8] = "";
char FBResBPPBuffer[8] = "";
char FBResBSBuffer[8] = "?";
char RegBuffer[256] = "";
int ProcessorCount = 0;


/* ATTRIBUTES */
// Set the base revision to 3; this is recommended as this is the latest base revision described by the
// Limine boot protocol specification (see specification for further info).
__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
struct limine_bootloader_info_request BootloaderInfoRequest = {
    .id = LIMINE_BOOTLOADER_INFO_REQUEST,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
struct limine_firmware_type_request FWTypeRequest = {
    .id = LIMINE_FIRMWARE_TYPE_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
struct limine_mp_request MPRequest = {
    .id = LIMINE_MP_REQUEST,
    .revision = 0,
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;


/* FUNCTIONS */
// Prototypes so GCC stops complaining
void DisplaySystemInfo();
void KernelLoop();

// Halts the system in case of a failure, this will usually only be called by KernelPanic.
void HaltSystem(void)
{
    for (;;)
        asm ("hlt");
}

// Displays a kernel panic with an error code and message, and halts the system. The panic will be
// sent to both the framebuffer and serial console. Each device will only be used if they exist, with
// serial being checked first in case some framebuffer operation goes wrong.
void KernelPanic(uint64_t ErrorCode, char* Message)
{
    // Disable interrupts so nothing will trigger any more interrupts.
    // This prevents a bad situation from becoming worse.
	__asm__ volatile ("cli");
    
    // Convert the error code into a string so it can be displayed.
    char ECBuffer[64] = "";
    
    IntToStr(ErrorCode, ECBuffer, 16);

    // If a framebuffer was provided by the bootloader,clear the screen and display
    // the panic information.
    if (FBExists == true)
    {
        bool PrevSerial = SerialExists;

        TerminalDrawCursor = false;
        SerialExists = false;
        TerminalFGColor = 0xFF0000;
        TerminalBGColor = 0x000000;

        ClearTerminal();

        // Draw the error triangle.
        DrawFilledTriangle(68, 8, 128, 136, 8, 136, 0xFF0000, 0xFF0000);
        DrawFilledRect(60, 108, 16, 16, 0x000000);
        DrawFilledRect(60, 48, 16, 48, 0x000000);

        // Display panic information.
        MoveCursor(0, 9);
        TerminalDrawString("[== KERNEL PANIC ==]\n\rSYSMESG = <SYSHALT_REQUIRED:SFEXEC_KILL>\n\rERRMESG = ");
        TerminalDrawString(Message);
        TerminalDrawString("\n\rERRCODE = 0x");
        TerminalDrawString(ECBuffer);

        // The other registers are useful for paging. That isn't implemented yet, so there's no point in including them until it is.
        TerminalDrawString("\n\n\r[== STACK STATE ==]\n\r");

        if (CrashStackState != NULL)
        {
            if (CrashStackState->InterruptNum != 0x00 || CrashStackState->InterruptNum == 0x00)
            {
                TerminalDrawString("INTERRUPT_NUM = 0x");
                IntToStr(CrashStackState->InterruptNum, RegBuffer, 16);
                TerminalDrawString(RegBuffer);
            }
            else
            {
                TerminalDrawString("INTERRUPT_NUM = NULL");
            }
            
            if (CrashStackState->ErrorCode != 0x00 || CrashStackState->ErrorCode == 0x00)
            {
                TerminalDrawString("\n\rERROR_CODE = 0x");
                IntToStr(CrashStackState->ErrorCode, RegBuffer, 16);
                TerminalDrawString(RegBuffer);
            }
            else
            {
                TerminalDrawString("\n\rERROR_CODE = NULL");
            }
            
            if (CrashStackState->RIP != 0x00 || CrashStackState->RIP == 0x00)
            {
                TerminalDrawString("\n\rRIP = 0x");
                IntToStr(CrashStackState->RIP, RegBuffer, 16);
                TerminalDrawString(RegBuffer);
            }
            else
            {
                TerminalDrawString("\n\rRIP = NULL");
            }

            if (CrashStackState->CS != 0x00 || CrashStackState->CS == 0x00)
            {
                TerminalDrawString("\n\rCS = 0x");
                IntToStr(CrashStackState->CS, RegBuffer, 16);
                TerminalDrawString(RegBuffer);
            }
            else
            {
                TerminalDrawString("\n\rCS = NULL");
            }

            if (CrashStackState->RFLAGS != 0x00 || CrashStackState->RFLAGS == 0x00)
            {
                TerminalDrawString("\n\rRFLAGS = 0x");
                IntToStr(CrashStackState->RFLAGS, RegBuffer, 16);
                TerminalDrawString(RegBuffer);
            }
            else
            {
                TerminalDrawString("\n\rRFLAGS = NULL");
            }

            if (CrashStackState->SS_RSP != 0x00 || CrashStackState->SS_RSP == 0x00)
            {
                TerminalDrawString("\n\rSS_RSP = 0x");
                IntToStr(CrashStackState->SS_RSP, RegBuffer, 16);
                TerminalDrawString(RegBuffer);
            }
            else
            {
                TerminalDrawString("\n\rSS_RSP = NULL");
            }
        }
        else
        {
            TerminalDrawString("No stack state was provided.");
        }
        
        SerialExists = PrevSerial;
    }

    // If a serial port is available and initialized, send the panic information out to whatever device may be connected to the port.
    if (SerialExists == true)
    {
        SendStringSerial(SERIAL_COM1, "\n\n\n\r[===== KERNEL PANIC =====]\rERRMESG = ");
        SendStringSerial(SERIAL_COM1, Message);
        SendStringSerial(SERIAL_COM1, "\rERRCODE = 0x");
        SendStringSerial(SERIAL_COM1, ECBuffer);

        if (CrashStackState != NULL)
        {
            if (CrashStackState->InterruptNum != 0x00 || CrashStackState->InterruptNum == 0x00)
            {
                SendStringSerial(SERIAL_COM1, "\rINTERRUPT_NUM = 0x");
                IntToStr(CrashStackState->InterruptNum, RegBuffer, 16);
                SendStringSerial(SERIAL_COM1, RegBuffer);
            }
            else
            {
                SendStringSerial(SERIAL_COM1, "\rINTERRUPT_NUM = NULL");
            }
            
            if (CrashStackState->ErrorCode != 0x00 || CrashStackState->ErrorCode == 0x00)
            {
                SendStringSerial(SERIAL_COM1, "\rERROR_CODE = 0x");
                IntToStr(CrashStackState->ErrorCode, RegBuffer, 16);
                SendStringSerial(SERIAL_COM1, RegBuffer);
            }
            else
            {
                SendStringSerial(SERIAL_COM1, "\rERROR_CODE = NULL");
            }
            
            if (CrashStackState->RIP != 0x00 || CrashStackState->RIP == 0x00)
            {
                SendStringSerial(SERIAL_COM1, "\rRIP = 0x");
                IntToStr(CrashStackState->RIP, RegBuffer, 16);
                SendStringSerial(SERIAL_COM1, RegBuffer);
            }
            else
            {
                SendStringSerial(SERIAL_COM1, "\rRIP = NULL");
            }

            if (CrashStackState->CS != 0x00 || CrashStackState->CS == 0x00)
            {
                SendStringSerial(SERIAL_COM1, "\rCS = 0x");
                IntToStr(CrashStackState->CS, RegBuffer, 16);
                SendStringSerial(SERIAL_COM1, RegBuffer);
            }
            else
            {
                SendStringSerial(SERIAL_COM1, "\rCS = NULL");
            }

            if (CrashStackState->RFLAGS != 0x00 || CrashStackState->RFLAGS == 0x00)
            {
                SendStringSerial(SERIAL_COM1, "\rRFLAGS = 0x");
                IntToStr(CrashStackState->RFLAGS, RegBuffer, 16);
                SendStringSerial(SERIAL_COM1, RegBuffer);
            }
            else
            {
                SendStringSerial(SERIAL_COM1, "\rRFLAGS = NULL");
            }

            if (CrashStackState->SS_RSP != 0x00 || CrashStackState->SS_RSP == 0x00)
            {
                SendStringSerial(SERIAL_COM1, "\rSS_RSP = 0x");
                IntToStr(CrashStackState->SS_RSP, RegBuffer, 16);
                SendStringSerial(SERIAL_COM1, RegBuffer);
            }
            else
            {
                SendStringSerial(SERIAL_COM1, "\rSS_RSP = NULL");
            }
        }
        else
        {
            SendStringSerial(SERIAL_COM1, "\rNo stack state was provided.");
        }

        SendStringSerial(SERIAL_COM1, "\n\rThe system has been halted and will no longer respond to serial commands.");
    }
    
    // Play a short (low frequency) tone for 0.5 seconds.
    //PCSpeakerPlayTone(405);

    HaltSystem();
}

// KernelStart is the entry point for the entire OS.
void KernelStart(void)
{
    TerminalDrawCursor = false;

    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false)
    {
        KernelPanic(-1, "LIMINE_BASE_REVISION_SUPPORTED returned false!");
    }

    // Play a tone for a few milliseconds to indicate that the kernel has loaded properly.
    //PCSpeakerPlayTone(900);

    // Initialize serial port COM1 before graphics, so kernel panic messages are visible via a serial console.
    InitSerialPort(SERIAL_COM1);

    // Initialize the graphics & framebuffer.
    InitGraphics();

    // Draw the OS logo.
    // TODO: implement OS logo drawing
    
    // Get the firmware type.
    if (FWTypeRequest.response->firmware_type <= 3)
    {
        FirmwareType = FirmwareTypes[FWTypeRequest.response->firmware_type];
    }
    else
    {
        IntToStr(FWTypeRequest.response->firmware_type, FirmwareType, 10);
    }

    // Print boot information.
    TerminalDrawString("[== BOREAL OPERATING SYSTEM ==]\n\r[INFO] >> Kernel started (booted from ");
    TerminalDrawString(BootloaderInfoRequest.response->name);
    TerminalDrawChar(' ', true);
    TerminalDrawString(BootloaderInfoRequest.response->version);
    TerminalDrawString(").\n\r");

    TerminalDrawString("[INFO] >> Kernel C language version (__STDC_VERSION__): \"");
    TerminalDrawString(PREPROCESSOR_TOSTRING(__STDC_VERSION__));
    TerminalDrawString("\".\n\r");

    TerminalDrawString("[INFO] >> Initializing GDT...\n\r");
    InitGDT();

    // The PIC must be remapped & disabled before the IDT init. There is a tiny amount of time that WILL
    // cause issues if the PIT manages to fire, which usually leads to a general protection fault. Disabling
    // the PIC will ensure that the PIT can't fire during that time.
    TerminalDrawString("[INFO] >> Initializing PIC...\n\r");
    InitPIC();

    TerminalDrawString("[INFO] >> Initializing IDT...\n\r");
    InitIDT();

    // The IDT can be tested using this for loop.
    /*for (int ExceptionNum = 0; ExceptionNum < 32; ExceptionNum++)
    {
        TestIDT(ExceptionNum);
    }*/

    TerminalDrawString("[INFO] >> Initializing FPU...\n\r");
    InitFPU();

    TerminalDrawString("[INFO] >> Initializing ACPI...\n\r");
    InitACPI();

    TerminalDrawString("[INFO] >> Initializing monitors...\n\r");
    // TODO: Implement monitor EDID checking & other monitor stuff

    // The current SSE init implementation is a bit new, so there will likely still be a few bugs
    TerminalDrawString("[INFO] >> Initializing SSE...\n\r");
    InitSSE();

    // This might look a bit backwards, remember that SSE uses the little endian format
    TerminalDrawString("\n\r[INFO] >> Testing SSE (METHOD=\"_mm_set_ps -> _mm_add_ps\")...\n\r");
    float ExpectedResults[4] = {-4.0f, 4.0f, 8.0f, 6.0f}; // Remember, SSE uses little-endian
    float StoredResult[4];
    char SSEExpectedBuffer[8];
    char SSEResultBuffer[8];
    __m128 SSESet1 = _mm_set_ps(1.0f, 2.0f, 7.0f, 4.0f);
    __m128 SSESet2 = _mm_set_ps(5.0f, 6.0f, -3.0f, -8.0f);

    // Perform an SSE addition operation and store the result back to a float array
    __m128 SSEAddResult = _mm_add_ps(SSESet1, SSESet2);
    _mm_store_ps(StoredResult, SSEAddResult);

    // Print and compare the results of each SSE test
    for (int I = 0; I < 4; I++)
    {
        FloatToStr(ExpectedResults[I], SSEExpectedBuffer, 2);
        FloatToStr(StoredResult[I], SSEResultBuffer, 2);
        TerminalDrawString("\tSSE value for loop iteration #");
        TerminalDrawChar((I + 1) + '0', true);
        TerminalDrawString(" is: ");
        TerminalDrawString(SSEResultBuffer);
        TerminalDrawString(" (expected ");
        TerminalDrawString(SSEExpectedBuffer);
        TerminalDrawString(")\n\r");

        // We don't simply do a != check here because there can be small float errors
        // that it wouldn't catch. These float errors are generally acceptable
        float Diff = StoredResult[I] - ExpectedResults[I];

        if (Diff < -0.01f || Diff > 0.01f)
        {
            char FailureStr1[64] = "";
            char FailureStr2[64] = "";
            char ReasonStr[64] = "";

            StrCat("SSE test failed, got ", SSEResultBuffer, FailureStr1);
            StrCat(" instead of ", SSEExpectedBuffer, FailureStr2);
            StrCat(FailureStr1, FailureStr2, ReasonStr);
            KernelPanic(0, ReasonStr);
        }
    }

    TerminalDrawString("\n\r[INFO] >> Initializing PS/2 controller, keyboard, and mouse...\n\r");
    InitPS2Controller();
    InitPS2Keyboard();
    InitPS2Mouse();

    TerminalDrawString("[INFO] >> Initializing PCI...\n\r");
    // Init code here
    
    TerminalDrawString("[INFO] >> Initializing USB controller(s)\n\r");
    // Init code here
    
    // Display system information before the init finishes.
    DisplaySystemInfo();
    TerminalDrawString("\n\r[INFO] >> Init process finished.\n\r");

    // The main system initialization has finished, so the kernel loop can be entered.
    KernelLoop();
}

// There should always be something running, if we reach the end of this function, throw a kernel panic because
// there's nothing else that the computer should be doing.
void KernelLoop()
{
    TerminalDrawString("[INFO] >> Kernel loop started, starting shell...\n\r");
    ClearTerminal();
    TerminalDrawString("[== Welcome to BorealOS! ==]\n\r[INFO] >> Type \"help\" for a command list.\n\r");
    StartShell();
    KernelPanic(0, "End of kernel loop reached (no more running processes)");
}

void DisplaySystemInfo()
{
    // Get the number of processors in the system.
    ProcessorCount = MPRequest.response->cpu_count;

    TerminalDrawString("[INFO] >> ");
    TerminalDrawChar(ProcessorCount + '0', true);
    TerminalDrawString(" available processor(s).\n\r");

    TerminalDrawString("[INFO] >> ");
    TerminalDrawChar(FBCount + '0', true);
    TerminalDrawString(" available framebuffer(s):\n\r");

    for (int FBIndex = 0; FBIndex < FBCount; FBIndex++)
    {
        FBInfo = GetFramebufferAtIndex(FBIndex);

        IntToStr(FBInfo->width, FBResWidthBuffer, 10);
        IntToStr(FBInfo->height, FBResHeightBuffer, 10);
        IntToStr(FBInfo->bpp, FBResBPPBuffer, 10);
        FloatToStr((FBInfo->width * FBInfo->height * (FBInfo->bpp / 8.0f)) / (1024.0f * 1024.0f), FBResBSBuffer, 2);

        TerminalDrawString("  Framebuffer #");
        TerminalDrawChar(FBIndex + '0', true);
        TerminalDrawString(": ");
        TerminalDrawString(FBResWidthBuffer);
        TerminalDrawString("x");
        TerminalDrawString(FBResHeightBuffer);
        TerminalDrawString("@");
        TerminalDrawString(FBResBPPBuffer);
        TerminalDrawString("bpp (~");
        TerminalDrawString(FBResBSBuffer);
        TerminalDrawString(" MB VRAM)\n\r");
    }

    TerminalDrawString("\n\r[INFO] >> Firmware type is \"");
    TerminalDrawString(FirmwareType);
    TerminalDrawString("\"\n\r");
}