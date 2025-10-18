#include <Core/Kernel.h>
#include <Core/Interrupts/PIT.h>
#include <Core/Memory/Memory.h>
#include <uacpi/kernel_api.h>

#include "ACPI.h"

// This is the implementation of the uACPI library's platform-specific functions for BorealOS.
// Since I had a tearful amount of problems with uACPI, and related things this is pretty undocumented, but i just dont know how he works i have no clue i geniunely
// God help me

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
    if (KernelACPI.Initialized) {
        *out_rsdp_address = (uintptr_t)KernelACPI.RSDP;
        return UACPI_STATUS_OK;
    }
    return UACPI_STATUS_NOT_FOUND;
}

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size) {
    return (void*) addr;
}

void uacpi_kernel_unmap(void*, uacpi_size) {

}

void *uacpi_kernel_alloc(uacpi_size size) {
    void* addr = (void*) VirtualMemoryManagerAllocate(&Kernel.VMM, size, true, false);
    return addr;
}

void *uacpi_kernel_alloc_zeroed(uacpi_size size) {
    void *ptr = uacpi_kernel_alloc(size);
    if (ptr) {
        memset32_8086(ptr, 0, size);
    }
    return ptr;
}

void uacpi_kernel_free(void *mem, uacpi_size size_hint){
    VirtualMemoryManagerFree(&Kernel.VMM, mem, size_hint);
}

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    return KernelPIT.Milliseconds * 1000000ULL;
}

void uacpi_kernel_stall(uacpi_u8 usec) {
    PITBusyWaitMicroseconds(usec);
}

void uacpi_kernel_sleep(uacpi_u64 msec) {
    PITBusyWaitMicroseconds(PIT_MILLISECONDS_TO_MICROSECONDS(msec));
}

void uacpi_kernel_log(uacpi_log_level level, const uacpi_char* string) {
    KernelLogType log_type;
    switch (level) {
        case UACPI_LOG_INFO:
            log_type = LOG_INFO;
            break;
        case UACPI_LOG_WARN:
            log_type = LOG_WARNING;
            break;
        case UACPI_LOG_ERROR:
            log_type = LOG_ERROR;
            break;
        case UACPI_LOG_DEBUG:
            log_type = LOG_DEBUG;
            break;
        case UACPI_LOG_TRACE:
            log_type = LOG_DEBUG;
            break;
        default:
            log_type = LOG_INFO;
            break;
    }

    LOGF(log_type, "[uACPI] %s", string);
}

uacpi_handle uacpi_kernel_create_mutex(void) {
    return (uacpi_handle)1; // We do not have mutexes yet TODO: Implement
}
void uacpi_kernel_free_mutex(uacpi_handle h) { (void)h; }

uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle h, uacpi_u16 timeout_ms) {
    (void)h;
    (void)timeout_ms;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_release_mutex(uacpi_handle h) {
    (void)h;
}

uacpi_handle uacpi_kernel_create_spinlock(void) {
    // return a dummy pointer
    return (uacpi_handle)1;
}
void uacpi_kernel_free_spinlock(uacpi_handle h) { (void)h; }

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle h) {
    (void)h;
    // pretend interrupts were enabled and now disabled. return previous flags = 0
    return (uacpi_cpu_flags)0;
}

void uacpi_kernel_unlock_spinlock(uacpi_handle h, uacpi_cpu_flags flags) {
    (void)h; (void)flags; // no-op
}


uacpi_status uacpi_kernel_schedule_work(uacpi_work_type type, uacpi_work_handler handler, uacpi_handle ctx) {
    if (!handler) return UACPI_STATUS_INVALID_ARGUMENT;
    // We are single-threaded for now, so just
    // execute immediately; for GPE handlers spec suggests CPU0, but single-threaded -> immediate.
    (void)type;
    handler(ctx);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
    // single-threaded implementation: no outstanding work maintained here.
    return UACPI_STATUS_OK;
}

struct _irq_reg {
    uacpi_u32 irq;
    uacpi_interrupt_handler handler;
    uacpi_handle ctx;
    uacpi_handle id;
};
static struct _irq_reg irq_registry[256];
static uacpi_u32 irq_count = 0;

static void irqHandler(uint8_t irq, RegisterState* state) {
    // Call irq_registry[irq].handler with irq_registry[irq].ctx
    (void) state;
    for (uacpi_u32 i = 0; i < irq_count; i++) {
        if (irq_registry[i].irq == irq) {
            if (irq_registry[i].handler) {
                irq_registry[i].handler(irq_registry[i].ctx);
            }
            break;
        }
    }
}

uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx, uacpi_handle *out_irq_handle) {
    // At the moment we only support IRQs 0-15
    if (irq > 15 || !handler || irq_count >= 256 || !out_irq_handle) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    // Check if already registered
    for (uacpi_u32 i = 0; i < irq_count; i++) {
        if (irq_registry[i].irq == irq) {
            return UACPI_STATUS_ALREADY_EXISTS;
        }
    }

    // Register the IRQ
    irq_registry[irq_count].irq = irq;
    irq_registry[irq_count].handler = handler;
    irq_registry[irq_count].ctx = ctx;
    irq_registry[irq_count].id = (uacpi_handle)(uintptr_t)(irq_count + 1); // id cannot be NULL
    *out_irq_handle = irq_registry[irq_count].id;
    irq_count++;
    IDTSetIRQHandler(irq, irqHandler);
    return UACPI_STATUS_OK;
}



uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler handler, uacpi_handle irq_handle) {
for (uacpi_u32 i = 0; i < irq_count; i++) {
        if (irq_registry[i].id == irq_handle && irq_registry[i].handler == handler) {
            // Remove the handler by shifting the rest down
            for (uacpi_u32 j = i; j < irq_count - 1; j++) {
                irq_registry[j] = irq_registry[j + 1];
            }
            irq_count--;
            return UACPI_STATUS_OK;
        }
    }
    return UACPI_STATUS_NOT_FOUND;
}

struct _uacpi_event {
    uacpi_u32 counter;
};


uacpi_handle uacpi_kernel_create_event(void) {
    struct _uacpi_event *e = uacpi_kernel_alloc_zeroed(sizeof(*e));
    return (uacpi_handle)e;
}
void uacpi_kernel_free_event(uacpi_handle h) {
    if (!h) return;
    uacpi_kernel_free((void*)h, sizeof(struct _uacpi_event));
}


uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle h, uacpi_u16 timeout_ms) {
    if (!h) return UACPI_FALSE;
    struct _uacpi_event *e = (struct _uacpi_event*)h;


    if (e->counter > 0) {
        e->counter--;
        return UACPI_TRUE;
    }


    if (timeout_ms == 0) return UACPI_FALSE;


    uacpi_u64 start_ns = uacpi_kernel_get_nanoseconds_since_boot();
    uacpi_u64 timeout_ns = (timeout_ms == 0xFFFF) ? (uacpi_u64)-1 : (uacpi_u64)timeout_ms * 1000000ULL;


    while (1) {
        if (e->counter > 0) {
            e->counter--;
            return UACPI_TRUE;
        }
        if (timeout_ns != (uacpi_u64)-1) {
            uacpi_u64 now = uacpi_kernel_get_nanoseconds_since_boot();
            if (now - start_ns >= timeout_ns) return UACPI_FALSE;
        }
        // Yield a little
        uacpi_kernel_stall(1);
    }
}


void uacpi_kernel_signal_event(uacpi_handle h) {
    if (!h) return;
    struct _uacpi_event *e = (struct _uacpi_event*)h;
    e->counter++;
}


void uacpi_kernel_reset_event(uacpi_handle h) {
    if (!h) return;
    struct _uacpi_event *e = (struct _uacpi_event*)h;
    e->counter = 0;
}

uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    // Single-threaded. ID must not be UACPI_THREAD_ID_NONE.
    return (uacpi_thread_id)1;
}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *req) {
    if (!req) return UACPI_STATUS_INVALID_ARGUMENT;

    LOGF(LOG_INFO, "uACPI firmware request: type=%u\n", (unsigned)req->type);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address address, uacpi_handle *out_handle) {
    (void)address, (void)out_handle;
    PANIC("uACPI PCI support not implemented");
}

void uacpi_kernel_pci_device_close(uacpi_handle handle) {
    (void)handle;
    PANIC("uACPI PCI support not implemented");
}

uacpi_status uacpi_kernel_pci_read8(uacpi_handle device, uacpi_size offset, uacpi_u8 *value) {
    (void)device, (void)offset, (void)value;
    PANIC("uACPI PCI support not implemented");
}

uacpi_status uacpi_kernel_pci_read16(uacpi_handle device, uacpi_size offset, uacpi_u16 *value) {
    (void)device, (void)offset, (void)value;
    PANIC("uACPI PCI support not implemented");
}

uacpi_status uacpi_kernel_pci_read32(uacpi_handle device, uacpi_size offset, uacpi_u32 *value) {
    (void)device, (void)offset, (void)value;
    PANIC("uACPI PCI support not implemented");
}

uacpi_status uacpi_kernel_pci_write8(uacpi_handle device, uacpi_size offset, uacpi_u8 value) {
    (void)device, (void)offset, (void)value;
    PANIC("uACPI PCI support not implemented");
}

uacpi_status uacpi_kernel_pci_write16(uacpi_handle device, uacpi_size offset, uacpi_u16 value) {
    (void)device, (void)offset, (void)value;
    PANIC("uACPI PCI support not implemented");
}

uacpi_status uacpi_kernel_pci_write32(uacpi_handle device, uacpi_size offset, uacpi_u32 value) {
    (void)device, (void)offset, (void)value;
    PANIC("uACPI PCI support not implemented");
}

struct _uacpi_io_map {
    uacpi_io_addr base;
    uacpi_size len;
};

uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle) {
    if (!out_handle) return UACPI_STATUS_INVALID_ARGUMENT;
    struct _uacpi_io_map *m = uacpi_kernel_alloc_zeroed(sizeof(*m));
    if (!m) return UACPI_STATUS_OUT_OF_MEMORY;
    m->base = base;
    m->len = len;
    *out_handle = (uacpi_handle)m;
    return UACPI_STATUS_OK;
}


void uacpi_kernel_io_unmap(uacpi_handle handle) {
    if (!handle) return;
    uacpi_kernel_free((void*)handle, sizeof(struct _uacpi_io_map));
}


static int validate_io_handle_and_offset(uacpi_handle h, uacpi_size offset, uacpi_size width) {
    if (!h) return 0;
    struct _uacpi_io_map *m = (struct _uacpi_io_map*)h;
    if (offset + width > m->len) return 0;
    return 1;
}

#define UACPI_STATUS_INVALID_PARAMETER UACPI_STATUS_INVALID_ARGUMENT
#include <Utility/SerialOperations.h>

uacpi_status uacpi_kernel_io_read8(uacpi_handle h, uacpi_size offset, uacpi_u8 *out_value) {
if (!validate_io_handle_and_offset(h, offset, 1) || !out_value) return UACPI_STATUS_INVALID_PARAMETER;
struct _uacpi_io_map *m = (struct _uacpi_io_map*)h;
uint16_t port = (uint16_t)(m->base + offset);
*out_value = inb(port);
return UACPI_STATUS_OK;
}


uacpi_status uacpi_kernel_io_read16(uacpi_handle h, uacpi_size offset, uacpi_u16 *out_value) {
if (!validate_io_handle_and_offset(h, offset, 2) || !out_value) return UACPI_STATUS_INVALID_PARAMETER;
struct _uacpi_io_map *m = (struct _uacpi_io_map*)h;
uint16_t port = (uint16_t)(m->base + offset);
*out_value = inw(port);
return UACPI_STATUS_OK;
}


uacpi_status uacpi_kernel_io_read32(uacpi_handle h, uacpi_size offset, uacpi_u32 *out_value) {
if (!validate_io_handle_and_offset(h, offset, 4) || !out_value) return UACPI_STATUS_INVALID_PARAMETER;
struct _uacpi_io_map *m = (struct _uacpi_io_map*)h;
uint16_t port = (uint16_t)(m->base + offset);
*out_value = inl(port);
return UACPI_STATUS_OK;
}


uacpi_status uacpi_kernel_io_write8(uacpi_handle h, uacpi_size offset, uacpi_u8 in_value) {
if (!validate_io_handle_and_offset(h, offset, 1)) return UACPI_STATUS_INVALID_PARAMETER;
struct _uacpi_io_map *m = (struct _uacpi_io_map*)h;
uint16_t port = (uint16_t)(m->base + offset);
outb(port, in_value);
return UACPI_STATUS_OK;
}


uacpi_status uacpi_kernel_io_write16(uacpi_handle h, uacpi_size offset, uacpi_u16 in_value) {
if (!validate_io_handle_and_offset(h, offset, 2)) return UACPI_STATUS_INVALID_PARAMETER;
struct _uacpi_io_map *m = (struct _uacpi_io_map*)h;
uint16_t port = (uint16_t)(m->base + offset);
outw(port, in_value);
return UACPI_STATUS_OK;
}


uacpi_status uacpi_kernel_io_write32(uacpi_handle h, uacpi_size offset, uacpi_u32 in_value) {
if (!validate_io_handle_and_offset(h, offset, 4)) return UACPI_STATUS_INVALID_PARAMETER;
struct _uacpi_io_map *m = (struct _uacpi_io_map*)h;
uint16_t port = (uint16_t)(m->base + offset);
outl(port, in_value);
return UACPI_STATUS_OK;
}
