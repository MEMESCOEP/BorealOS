#include <Definitions.h>
#include <Core/Kernel.h>
#include <Core/Interrupts/PIT.h>
#include <Drivers/IO/PCI/PCI.h>
#include <lai/host.h>
#include <Utility/SerialOperations.h>

#include "ACPI.h"

void laihost_log(int level, const char *msg) {
    KernelLogType logType;
    switch (level) {
        case LAI_DEBUG_LOG:
            logType = LOG_DEBUG;
            break;
        case LAI_WARN_LOG:
            logType = LOG_WARNING;
            break;
        default:
            logType = LOG_INFO;
            break;
    }

    LOGF(logType, "LAI: %s\n", msg);
}

NORETURN
void laihost_panic(const char *msg) {
    LOGF(LOG_CRITICAL, "LAI Panic: %s\n", msg);
    PANIC("LAI Panic occurred. See log for details.\n");
}

void *laihost_malloc(size_t size) {
    void* addr = HeapAlloc(&Kernel.Heap, size);
    return addr;
}

void *laihost_realloc(void *ptr, size_t newsize, size_t) {
    void* newAddr = HeapRealloc(&Kernel.Heap, ptr, newsize);
    return newAddr;
}

void laihost_free(void *ptr, size_t) {
    HeapFree(&Kernel.Heap, ptr);
}

// Address is the physical address to map, count is the size in bytes
void *laihost_map(size_t address, size_t count) {
    uintptr_t alignedAddr = ALIGN_DOWN(address, PMM_PAGE_SIZE);
    size_t alignedSize = ALIGN_UP(address + count, PMM_PAGE_SIZE) - alignedAddr;
    for (uintptr_t addr = alignedAddr; addr < alignedAddr + alignedSize; addr += PMM_PAGE_SIZE) {
        if (PagingMapPage(&Kernel.Paging, (void *)(addr), (void *)addr, true, false, 0) != STATUS_SUCCESS) {
            LOGF(LOG_ERROR, "laihost_map: Failed to map page at %p!\n", (void *)addr);
            return nullptr;
        }
    }

    return (void *)(address );
}

void laihost_unmap(void *addrress, size_t count) {
    uintptr_t address = (uintptr_t)addrress;
    uintptr_t alignedAddr = ALIGN_DOWN(address, PMM_PAGE_SIZE);
    size_t alignedSize = ALIGN_UP(address + count, PMM_PAGE_SIZE) - alignedAddr;
    for (uintptr_t addr = alignedAddr; addr < alignedAddr + alignedSize; addr += PMM_PAGE_SIZE) {
        if (PagingUnmapPage(&Kernel.Paging, (void *)addr) != STATUS_SUCCESS) {
            LOGF(LOG_ERROR, "laihost_unmap: Failed to unmap page at %p!\n", (void *)addr);
        }
    }
}

void *laihost_scan(const char *sig, size_t idx) {
    void* result = nullptr;
    Status status = ACPIGetTableBySignature(sig, idx, &result);
    if (status != STATUS_SUCCESS) {
        return nullptr;
    }
    return result;
}

void laihost_outb(uint16_t port, uint8_t val){
    outb(port, val);
}

void laihost_outw(uint16_t port, uint16_t val){
    outw(port, val);
}

void laihost_outd(uint16_t port, uint32_t val){
    outl(port, val);
}

uint8_t laihost_inb(uint16_t port) {
    return inb(port);
}

uint16_t laihost_inw(uint16_t port) {
    return inw(port);
}

uint32_t laihost_ind(uint16_t port) {
    return inl(port);
}

uint8_t laihost_pci_readb(UNUSED uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset){
    return PCIReadConfigByte(bus, slot, fun, offset);
}

uint16_t laihost_pci_readw(UNUSED uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset){
    return PCIReadConfigWord(bus, slot, fun, offset);
}

uint32_t laihost_pci_readd(UNUSED uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset){
    return PCIReadConfigDWord(bus, slot, fun, offset);
}

void laihost_pci_writeb(UNUSED uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint8_t val){
    PCIWriteConfigByte(bus, slot, fun, offset, val);
}

void laihost_pci_writew(UNUSED uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint16_t val){
    PCIWriteConfigWord(bus, slot, fun, offset, val);
}

void laihost_pci_writed(UNUSED uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint32_t val){
    PCIWriteConfigDWord(bus, slot, fun, offset, val);
}


void laihost_sleep(uint64_t ms) {
    PITBusyWaitMicroseconds(PIT_MILLISECONDS_TO_MICROSECONDS(ms));
}

uint64_t laihost_timer(void) {
    return KernelPIT.Milliseconds * 10000; // Convert ms to 100ns units (smh)
}
