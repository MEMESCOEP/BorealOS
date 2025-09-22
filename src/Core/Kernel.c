#include "Kernel.h"

#include <stdarg.h>
#include <Drivers/Graphics/DefaultFont.h>
#include <Drivers/Graphics/Framebuffer.h>
#include <Drivers/IO/FramebufferConsole.h>
#include <Utility/Color.h>

#include "Utility/StringFormatter.h"

KernelState Kernel = {};

static NORETURN void serial_panic(const char* message) {
    SerialWriteString(&Kernel.Serial, "\nPanic!\n");
    SerialWriteString(&Kernel.Serial, message);
    FramebufferConsoleWriteString("\nPanic!\n");
    FramebufferConsoleWriteString(message);
    ASM("cli");
    while (true) {
        ASM("hlt");
    }
}

static void serial_log(const char* message) {
    SerialWriteString(&Kernel.Serial, message);
    FramebufferConsoleWriteString(message);
}

static void serial_printf(const char* format, ...) {
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

Status KernelInit(uint32_t InfoPtr) {
    ASM ("cli"); // Disable interrupts while we set up

    Kernel.Panic = serial_panic; // For now, just use the serial functions.
    Kernel.Log = serial_log;
    Kernel.Printf = serial_printf;

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

    if (FramebufferConsoleInit(KernelFramebuffer.Width / FONT_WIDTH, KernelFramebuffer.Height / FONT_HEIGHT, COLOR_WHITE, OURBLE) != STATUS_SUCCESS) {
        PANIC("Failed to initialize Framebuffer Console!\n");
    }

    LOG("Serial initialized successfully.\n");
    Kernel.Printf("Loading kernel version: %z.%z.%z.\n", BOREALOS_MAJOR_VERSION, BOREALOS_MINOR_VERSION, BOREALOS_PATCH_VERSION);

    // Now load the physical memory manager
    if (PhysicalMemoryManagerInit(InfoPtr) != STATUS_SUCCESS) {
        PANIC("Failed to initialize Physical Memory Manager!\n");
    }

    if (PhysicalMemoryManagerTest() != STATUS_SUCCESS) {
        PANIC("Physical Memory Manager test failed!\n");
    }

    Kernel.Printf("Physical Memory Manager initialized successfully. With %z pages of memory (%z MiB).\n", KernelPhysicalMemoryManager.TotalPages, (KernelPhysicalMemoryManager.TotalPages * 4096) / (1024 * 1024));
    Kernel.Printf("Physical Memory Manager has %z bytes for allocation & reservation maps.\n", KernelPhysicalMemoryManager.MapSize * 2);

    // Load the PIC
    if (PICInit(0x20, 0x28) != STATUS_SUCCESS) {
        PANIC("Failed to initialize PIC!\n");
    }
    LOG("PIC initialized successfully.\n");

    // Load the IDT
    if (IDTInit() != STATUS_SUCCESS) { // This also enables interrupts
        PANIC("Failed to initialize IDT!\n");
    }

    LOG("IDT initialized successfully.\n");

    if (PagingInit(&Kernel.Paging) != STATUS_SUCCESS) {
        PANIC("Failed to initialize Paging!\n");
    }

    PagingEnable(&Kernel.Paging);
    KernelFramebuffer.CanUse = false; // We can't use the framebuffer until we map it in

    if (PagingTest(&Kernel.Paging) != STATUS_SUCCESS) {
        PANIC("Paging test failed!\n");
    }

    LOG("Paging initialized successfully.\n");

    // Map in the framebuffer to the kernel's virtual address space.
    FramebufferMapSelf(&Kernel.Paging);
    KernelFramebuffer.CanUse = true; // Now we can use the framebuffer for drawing

    // ONLY AFTER HERE IS IT SAFE TO LOG TO THE FRAMEBUFFER.

    // Initialize the kernel VMM
    if (VirtualMemoryManagerInit(&Kernel.VMM, &Kernel.Paging) != STATUS_SUCCESS) {
        PANIC("Failed to initialize Kernel Virtual Memory Manager!\n");
    }

    Kernel.Printf("Kernel Virtual Memory Manager initialized successfully.\n");

    return STATUS_SUCCESS;
}
