/* LIBRARIES */
#include <Utilities/ArchUtils.h>
#include <Utilities/StrUtils.h>
#include <Drivers/HID/Keyboard.h>
#include <Drivers/HID/Mouse.h>
#include <Drivers/IO/Serial.h>
#include <Core/Interrupts/GDT.h>
#include <Core/Interrupts/IDT.h>
#include <Core/Multiboot/MB2Parser.h>
#include <Core/Multiboot/multiboot.h>
#include <Core/Graphics/Graphics.h>
#include <Core/Graphics/Console.h>
#include <Core/Hardware/Firmware.h>
#include <Core/Memory/PhysicalMemoryManager.h>
#include <Core/Memory/Memory.h>
#include <Core/Kernel/Panic.h>
#include <Core/Power/ACPI.h>
#include <Core/IO/PS2Controller.h>
#include <Core/IO/PIC.h>
#include <stdint.h>


/* VARIABLES */
typedef struct {
    uint32_t Type;
    uint32_t Size;
    uint64_t Addr;
    uint32_t Pitch;
    uint32_t Width;
    uint32_t Height;
    uint8_t  Bpp;
    uint8_t  TypeInfo;
    uint16_t Reserved;
} __attribute__((packed)) MB2Framebuffer_t;

extern void *_KernelStartMarker;
extern void *_KernelEndMarker;


/* FUNCTIONS */
void KernelStart(uint32_t Magic, uint32_t InfoPtr)
{
    // Initialize serial port COM1 before graphics, so kernel panic messages are visible via a serial console.
    InitSerialPort(SERIAL_COM1);

    // Make sure the bootloader actually runs the kernel using multiboot2 (the magic number should be 0x36D76289)
    if (Magic != MULTIBOOT2_BOOTLOADER_MAGIC)
    {
        SendStringSerial(ActiveSerialPort, "Bootloader provided an invalid magic number!");
        asm volatile("cli");

        while(true)
        {
            asm volatile("hlt");
        }
    }

    // Initialize the Physical Memory Manager
    PhysicalMemoryManagerInit((void*)InfoPtr);

    // Find the framebuffer and initialize graphics with the specified address, dimension, BPP and pitch
    MB2Framebuffer_t* MB2FramebufferTag = FindMB2Tag(MB2_TAG_FRAMEBUFFER, (void*)InfoPtr);

    GfxInit(
        (void *)(uintptr_t)MB2FramebufferTag->Addr,
        MB2FramebufferTag->Width,
        MB2FramebufferTag->Height,
        MB2FramebufferTag->Bpp,
        MB2FramebufferTag->Pitch
    );

    // Get the firmware type (BIOS, UEFI, other)
    GetFirmwareType((void*)InfoPtr);

    // Initialize the console so it can fill the entire framebuffer
    ConsoleInit(MB2FramebufferTag->Width / FONT_WIDTH, MB2FramebufferTag->Height / FONT_HEIGHT);
    ConsoleSetColor(LIGHT_GRAY, BLACK);
    ConsolePutString("[== BOREALOS ==]\n\r");

    LOG_KERNEL_MSG("Kernel architecure: \"", INFO);
    ConsolePutString(ARCH_NAME);
    ConsolePutString("\".\n\r");
    
    LOG_KERNEL_MSG("Kernel C language version (__STDC_VERSION__): \"", INFO);
    ConsolePutString(PREPROCESSOR_TOSTRING(__STDC_VERSION__));
    ConsolePutString("\".\n\r");

    LOG_KERNEL_MSG("Firmware type is \"", INFO);
    ConsolePutString(FirmwareType == 0 ? "BIOS": "UEFI");
    ConsolePutString("\".\n\r");

    LOG_KERNEL_MSG("Framebuffer at address 0x", INFO);
    PrintUnsignedNum(MB2FramebufferTag->Addr, 16);
    ConsolePutString(" is running at ");
    PrintSignedNum(MB2FramebufferTag->Width, 10);
    ConsolePutChar('x');
    PrintSignedNum(MB2FramebufferTag->Height, 10);
    ConsolePutChar('@');
    PrintSignedNum(MB2FramebufferTag->Bpp, 10);
    ConsolePutString("bpp.\n\r");

    LOG_KERNEL_MSG("Console size is ", INFO);
    PrintUnsignedNum(MB2FramebufferTag->Width / FONT_WIDTH, 10);
    ConsolePutString("x");
    PrintUnsignedNum(MB2FramebufferTag->Height / FONT_HEIGHT, 10);
    ConsolePutString(" (");
    PrintSignedNum((MB2FramebufferTag->Width / FONT_WIDTH) * (MB2FramebufferTag->Height / FONT_HEIGHT), 10);
    ConsolePutString(" characters).\n\r");

    // Initialize the PICs so we can use interupts
    LOG_KERNEL_MSG("Initializing PIC...\n\r", INFO);
    PICInit(0x20, 0x28);
    IoDelay();
    PICDisable();

    // Initialize the interrupts so the system can handle input and error events. The GDT is set up in multiboot.asm, so we assume it's already working correctly
    LOG_KERNEL_MSG("Initializing IDT...\n\r", INFO);
    IDTInit();

    // Initialize the PS/2 controller, keyboard, and mouse for user input
    LOG_KERNEL_MSG("Initializing PS/2 controller...\n\r", INFO);
    InitPS2Controller();

    LOG_KERNEL_MSG("Initializing PS/2 keyboard...\n\r", INFO);
    InitPS2Keyboard();

    LOG_KERNEL_MSG("Initializing PS/2 mouse...\n\r", INFO);
    InitPS2Mouse();
    
    // Initialize ACPI
    LOG_KERNEL_MSG("Initializing ACPI...\n\r", INFO);
    InitACPI((void*)InfoPtr);

    // The core init steps have finished, now the shell can be started
    LOG_KERNEL_MSG("Init finished, starting shell process...\n\r", INFO);

    // Ideally the shell should never exit, but we don't live in an ideal world. We want to panic here to prevent undefined behavior from causing the machine to explode and inflict 1000 damage on my sanity
    KernelPanic(-2, "Shell process exited!");
}

void KernelPanic(int ErrorCode, char* ErrorMessage)
{
    SendStringSerial(SERIAL_COM1, "Kernel panic!\n\r");
    SendStringSerial(SERIAL_COM1, "Reason: ");
    SendStringSerial(SERIAL_COM1, ErrorMessage);
    SendStringSerial(SERIAL_COM1, "\n\rCode: ");
    char ErrorCodeStr[25];
    IntToStr(ErrorCode, ErrorCodeStr, 10);
    SendStringSerial(SERIAL_COM1, ErrorCodeStr);
    SendStringSerial(SERIAL_COM1, "\n\r");

    uint8_t X, Y;

    // Move the kernel panic down if the cursor is not at the start of the line
    ConsoleGetCursorPos(&X, &Y);
    
    if (X != 0)
    {
        ConsoleSetCursor(0, Y + 1);
    }

    // Print the panic details
    ConsolePutString("[==              ==]\n\r");
    ConsoleChangeCursorPos(4, Y);
    ConsoleSetColor(RED, BLACK);
    ConsolePutString("KERNEL PANIC\n\r");
    ConsoleResetColor();
    ConsolePutString("Reason: ");
    ConsolePutString(ErrorMessage);
    ConsolePutString("\n\rCode: ");
    PrintSignedNum(ErrorCode, 10);
    
    // We don't want the kernel to keep doing other things. Disable all interrupts before dropping into an infinite loop
    asm volatile("cli");

    while(true)
    {
        asm volatile("hlt");
    }
}