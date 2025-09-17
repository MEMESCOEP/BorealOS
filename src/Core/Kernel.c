#include "Kernel.h"

#include <stdarg.h>

#include "Utility/StringFormatter.h"

static NORETURN void serial_panic(const KernelState* state, const char* message) {
    SerialWriteString(&state->Serial, "\nPanic!\n");
    SerialWriteString(&state->Serial, message);
    ASM("cli");
    while (true) {
        ASM("hlt");
    }
}

static void serial_log(const KernelState* state, const char* message) {
    SerialWriteString(&state->Serial, message);
}

static void serial_printf(const KernelState* state, const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    size_t written = VStringFormat(buffer, sizeof(buffer), format, args);
    va_end(args);
    SerialWriteString(&state->Serial, buffer);

    if (written >= sizeof(buffer)) {
        SerialWriteString(&state->Serial, "...(truncated)\n");
    }
}

Status KernelLoad(uint32_t InfoPtr, KernelState *out) {
    out->Panic = serial_panic; // For now, just use the serial functions.
    out->Log = serial_log;
    out->Printf = serial_printf;

    if (SerialLoad(SERIAL_COM1, &out->Serial) != STATUS_SUCCESS) {
        // We can't use serial, this probably means this is running on a real machine without a serial port.
        // Just ignore it for now, all logging will just be no-ops.
        // It's safe to keep the log & panic functions as they are, they will print nothing, but will still halt on panic.
        // TODO: Decide on something better for this case.
    }

    LOG(out, "Serial initialized successfully.\n");
    out->Printf(out, "Loading kernel version: %z.%z.%z.\n", BOREALOS_MAJOR_VERSION, BOREALOS_MINOR_VERSION, BOREALOS_PATCH_VERSION);

    // Now load the physical memory manager
    if (PhysicalMemoryManagerLoad(InfoPtr, &out->PhysicalMemoryManager) != STATUS_SUCCESS) {
        PANIC(out, "Failed to initialize Physical Memory Manager!\n");
    }

    if (PhysicalMemoryManagerTest(&out->PhysicalMemoryManager, out) != STATUS_SUCCESS) {
        PANIC(out, "Physical Memory Manager test failed!\n");
    }

    out->Printf(out, "Physical Memory Manager initialized successfully. With %z pages of memory (%z MiB).\n", out->PhysicalMemoryManager.TotalPages, (out->PhysicalMemoryManager.TotalPages * 4096) / (1024 * 1024));
    out->Printf(out, "Physical Memory Manager has %z bytes for allocation & reservation maps.\n", out->PhysicalMemoryManager.MapSize * 2);

    // Load the PIC
    if (PICLoad(0x20, 0x28, &out->PIC) != STATUS_SUCCESS) {
        PANIC(out, "Failed to initialize PIC!\n");
    }
    LOG(out, "PIC initialized successfully.\n");

    // Load the IDT
    if (IDTLoad(out, &out->IDT) != STATUS_SUCCESS) {
        PANIC(out, "Failed to initialize IDT!\n");
    }

    LOG(out, "IDT initialized successfully.\n");

    return STATUS_SUCCESS;
}
