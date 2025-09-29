#include "Kernel.h"

#include <Definitions.h>
#include <Drivers/Graphics/DefaultFont.h>
#include <Drivers/Graphics/Framebuffer.h>
#include <Drivers/IO/FramebufferConsole.h>
#include <Drivers/IO/HID/PS2Controller.h>
#include <Drivers/IO/Disk/ATA/ATACommon.h>
#include <Utility/StringFormatter.h>
#include <Utility/Color.h>
#include <Core/Firmware/ACPI.h>

KernelState Kernel = {};
uint64_t bootDurationMS = 0;

static NORETURN void KernelPanic(const char* message)
{
    // Serial doesn't usually have color, so we just print the message as-is
    SerialWriteString(&Kernel.Serial, "[== KERNEL PANIC ==]\n");
    SerialWriteString(&Kernel.Serial, message);

    // Our framebuffer *does* have color, so we can make certain text colored!
    FramebufferConsoleWriteString("[== \033[31mKERNEL PANIC\033[0m ==]\n");
    FramebufferConsoleWriteString(message);

    // Disable interrupts so the CPU doesn't try to handle anything while we're halted
    ASM("cli");

    while (true)
    {
        ASM("hlt");
    }
}

static void KernelPrintf(const char* format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    size_t written = VStringFormat(buffer, sizeof(buffer), format, args);
    va_end(args);
    SerialWriteString(&Kernel.Serial, buffer);
    FramebufferConsoleWriteString(buffer);

    if (written >= sizeof(buffer)) {
        SerialWriteString(&Kernel.Serial, "...(truncated)\n");
        FramebufferConsoleWriteString("...(truncated)\n");
    }
}

static void KernelLog(int log_type, const char* format, ...)
{
    // Define a temporary buffer and insert the arguments into the string
    char msg_buffer[512];
    va_list fmt_args;
    va_start(fmt_args, format);
    size_t written = VStringFormat(msg_buffer, sizeof(msg_buffer), format, fmt_args);
    va_end(fmt_args);

    // Write the log type in colored text, and the message in the default terminal foreground color
    FramebufferConsoleWriteString("[");
    SerialWriteChar(&Kernel.Serial, '[');

    switch(log_type)
    {
        case LOG_WARNING:
            FramebufferConsoleWriteString("\033[33mWARN\033[0m");
            SerialWriteString(&Kernel.Serial, "WARN");
            break;

        case LOG_ERROR:
            FramebufferConsoleWriteString("\033[31mERROR\033[0m");
            SerialWriteString(&Kernel.Serial, "ERROR");
            break;

        case LOG_CRITICAL:
            FramebufferConsoleWriteString("\033[38;2;128;0;0mCRITICAL\033[0m");
            SerialWriteString(&Kernel.Serial, "CRITICAL");
            break;

        case LOG_DEBUG:
            FramebufferConsoleWriteString("\033[36mDEBUG\033[0m");
            SerialWriteString(&Kernel.Serial, "DEBUG");
            break;

        case LOG_INFO:
        default:
            FramebufferConsoleWriteString("\033[32mINFO\033[0m");
            SerialWriteString(&Kernel.Serial, "INFO");
            break;
    }

    FramebufferConsoleWriteString("] ");
    FramebufferConsoleWriteString(msg_buffer);
    SerialWriteString(&Kernel.Serial, "] ");
    SerialWriteString(&Kernel.Serial, msg_buffer);

    // If the buffer was too small, notify the user that it's been truncated
    if (written >= sizeof(msg_buffer))
    {
        FramebufferConsoleWriteString("...(truncated)\n");
        SerialWriteString(&Kernel.Serial, "...(truncated)\n");
    }
}

Status KernelInit(uint32_t InfoPtr) {
    ASM ("cli"); // Disable interrupts while we set up

    Kernel.Panic = KernelPanic;
    Kernel.Log = KernelLog;
    Kernel.Printf = KernelPrintf;

    if (SerialInit(SERIAL_COM1) != STATUS_SUCCESS) {
        // We can't use serial, this probably means this is running on a real machine without a serial port.
        // Just ignore it for now, all logging will just be no-ops.
        // It's safe to keep the log & panic functions as they are, they will print nothing, but will still halt on panic.
        // TODO: Decide on something better for this case.
    }

    // Load the framebuffer
    if (FramebufferInit(InfoPtr) != STATUS_SUCCESS) {
        PANIC("Failed to initialize Framebuffer!\n");
    }

    if (FramebufferConsoleInit(KernelFramebuffer.Width / FONT_WIDTH, KernelFramebuffer.Height / FONT_HEIGHT, COLOR_LIGHTGRAY, OURBLE) != STATUS_SUCCESS) {
        PANIC("Failed to initialize Framebuffer Console!\n");
    }
    
    Kernel.Printf("[== BorealOS %z.%z.%z ==]\n", BOREALOS_MAJOR_VERSION, BOREALOS_MINOR_VERSION, BOREALOS_PATCH_VERSION);
    LOG(LOG_INFO, "Serial initialized successfully.\n");

    // Map the ACPI regions and initialize ACPI
    if (ACPIInit(InfoPtr) != STATUS_SUCCESS) {
        LOG(LOG_WARNING, "ACPI init failed!\n");
    }
    else {
        LOG(LOG_INFO, "ACPI initialized.\n");
    }

    // Now load the physical memory manager
    if (PhysicalMemoryManagerInit(InfoPtr) != STATUS_SUCCESS) {
        PANIC("Failed to initialize Physical Memory Manager!\n");
    }

    if (PhysicalMemoryManagerTest() != STATUS_SUCCESS) {
        PANIC("Physical Memory Manager test failed!\n");
    }

    LOGF(LOG_INFO, "Physical Memory Manager initialized successfully (%z pages totaling %z MiB).\n", KernelPhysicalMemoryManager.TotalPages, (KernelPhysicalMemoryManager.TotalPages * 4096) / (1024 * 1024));
    LOGF(LOG_INFO, "Physical Memory Manager has %z bytes for allocation & reservation maps.\n", KernelPhysicalMemoryManager.MapSize * 2);

    // Load the PIC
    if (PICInit(0x20, 0x28) != STATUS_SUCCESS) {
        PANIC("Failed to initialize PIC!\n");
    }
    LOG(LOG_INFO, "PIC initialized successfully.\n");

    // Load the IDT
    if (IDTInit() != STATUS_SUCCESS) { // This also enables interrupts
        PANIC("Failed to initialize IDT!\n");
    }

    LOG(LOG_INFO, "IDT initialized successfully.\n");

    if (PagingInit(&Kernel.Paging) != STATUS_SUCCESS) {
        PANIC("Failed to initialize Paging!\n");
    }

    PagingEnable(&Kernel.Paging);

    KernelFramebuffer.CanUse = false; // We can't use the framebuffer until we map it in

    if (PagingTest(&Kernel.Paging) != STATUS_SUCCESS) {
        PANIC("Paging test failed!\n");
    }

    LOG(LOG_INFO, "Paging initialized successfully.\n");

    // Map in the framebuffer to the kernel's virtual address space and enable write access for the framebuffer.
    FramebufferMapSelf(&Kernel.Paging);
    KernelFramebuffer.CanUse = true;

    LOG(LOG_INFO, "Framebuffer mapped into kernel virtual address space successfully.\n");

    // ONLY AFTER HERE IS IT SAFE TO LOG TO THE FRAMEBUFFER.

    // Initialize the kernel VMM
    if (VirtualMemoryManagerInit(&Kernel.VMM, &Kernel.Paging) != STATUS_SUCCESS) {
        PANIC("Failed to initialize Kernel Virtual Memory Manager!\n");
    }

    if (VirtualMemoryManagerTest(&Kernel.VMM) != STATUS_SUCCESS) {
        PANIC("Kernel Virtual Memory Manager test failed!\n");
    }

    LOG(LOG_INFO, "Kernel Virtual Memory Manager initialized successfully.\n");

    // Initialize the ATA/ATAPI subsystem
    if (ATAInit() != STATUS_SUCCESS) {
        PANIC("Failed to initialize ATA/ATAPI subsystem!\n");
    }
  
    // Initialize the PS/2 controller and any PS/2 keyboards & mice
    Status PS2InitStatus = PS2ControllerInit();

    if (PS2InitStatus == STATUS_UNSUPPORTED) {
        LOG(LOG_INFO, "No PS/2 controllers exist on this system.\n");
    }
    else if (PS2InitStatus != STATUS_SUCCESS) {
        LOG(LOG_INFO, "PS/2 controller init failed!\n");
    }

    LOGF(LOG_INFO, "Initialization finished successfully (took %ums).\n", bootDurationMS);
    return STATUS_SUCCESS;
}
