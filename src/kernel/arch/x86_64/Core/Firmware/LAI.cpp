// Lai function implementations
#include <Definitions.h>
#include <Kernel.h>
#include "../../KernelData.h"
#include "../../Memory/Paging.h"
#include "lai_include.h"

extern "C" {
    void laihost_log(int level, const char* message) {
        switch (level) {
            case LAI_DEBUG_LOG:
                LOG_DEBUG("[LAI] %s", message);
                break;
            case LAI_WARN_LOG:
                LOG_WARNING("[LAI] %s", message);
                break;
            default:
                LOG_INFO("[LAI] %s", message);
        }
    }

    void laihost_panic(const char* message) {
        LOG_ERROR("[LAI] %s", message);
        PANIC("Lai interpreter panic");
    }

    void *laihost_malloc(size_t size) {
        return new uint8_t[size];
    }

    void *laihost_realloc(void *ptr, size_t new_size, size_t old_size) {
        void* newPtr = new uint8_t[new_size];
        if (ptr) {
            memcpy(newPtr, ptr, old_size < new_size ? old_size : new_size);
            delete[] reinterpret_cast<uint8_t*>(ptr);
        }
        return newPtr;
    }

    void laihost_free(void *ptr, size_t) {
        delete[] reinterpret_cast<uint8_t*>(ptr);
    }

    // Map a page, count is in bytes.
    void *laihost_map(size_t address, size_t count) {
        Memory::Paging* paging = &Kernel<KernelData>::GetInstance()->ArchitectureData->Paging;
        uintptr_t alignedLowerHalf = ALIGN_DOWN(address, (uintptr_t)Architecture::KernelPageSize);
        uintptr_t alignedUpperHalf = ALIGN_UP(address + count, (uintptr_t)Architecture::KernelPageSize);
        uintptr_t higherHalf = hhdm_request.response->offset + alignedLowerHalf;

        for (uintptr_t offset = 0; offset < alignedUpperHalf - alignedLowerHalf; offset += Architecture::KernelPageSize) {
            if (!paging->IsMapped(higherHalf + offset))
                paging->MapPage(higherHalf + offset, alignedLowerHalf + offset, Memory::PageFlags::Present | Memory::PageFlags::ReadWrite);
        }

        return reinterpret_cast<void*>(higherHalf + (address - alignedLowerHalf));
    }

    void laihost_unmap(void *address, size_t count) {
        Memory::Paging* paging = &Kernel<KernelData>::GetInstance()->ArchitectureData->Paging;
        uintptr_t higherHalf = ALIGN_DOWN(reinterpret_cast<size_t>(address), (uintptr_t)Architecture::KernelPageSize);
        uintptr_t alignedUpperHalf = ALIGN_UP(reinterpret_cast<size_t>(address) + count, (uintptr_t)Architecture::KernelPageSize);

        for (uintptr_t offset = 0; offset < alignedUpperHalf - higherHalf; offset += Architecture::KernelPageSize) {
            if (paging->IsMapped(higherHalf + offset))
                paging->UnmapPage(higherHalf + offset);
        }
    }

    void *laihost_scan(const char* sig, size_t idx) {
        Core::Firmware::ACPI *acpi = &Kernel<KernelData>::GetInstance()->ArchitectureData->Acpi;
        return acpi->GetTable(sig, idx);
    }

    void laihost_outb(uint16_t port, uint8_t value) {
        IO::Serial::outb(port, value);
    }

    void laihost_outw(uint16_t port, uint16_t value) {
        IO::Serial::outw(port, value);
    }

    void laihost_outd(uint16_t port, uint32_t value) {
        IO::Serial::outl(port, value);
    }

    uint8_t laihost_inb(uint16_t port) {
        return IO::Serial::inb(port);
    }

    uint16_t laihost_inw(uint16_t port) {
        return IO::Serial::inw(port);
    }

    uint32_t laihost_ind(uint16_t port) {
        return IO::Serial::inl(port);
    }

    uint8_t laihost_pci_readb(UNUSED uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset) {
        IO::PCI* pci = Kernel<KernelData>::GetInstance()->ArchitectureData->Pci;
        uint32_t val = pci->ReadConfig(pci->GetDeviceHeader(bus, slot, fun), offset);
        return (val >> ((offset & 3) * 8)) & 0xFF;
    }

    uint16_t laihost_pci_readw(UNUSED uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset) {
        IO::PCI* pci = Kernel<KernelData>::GetInstance()->ArchitectureData->Pci;
        uint32_t val = pci->ReadConfig(pci->GetDeviceHeader(bus, slot, fun), offset);
        return (val >> ((offset & 2) * 8)) & 0xFFFF;
    }

    uint32_t laihost_pci_readd(UNUSED uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset) {
        IO::PCI* pci = Kernel<KernelData>::GetInstance()->ArchitectureData->Pci;
        return pci->ReadConfig(pci->GetDeviceHeader(bus, slot, fun), offset);
    }

    void laihost_pci_writeb(UNUSED uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint8_t val) {
        IO::PCI* pci = Kernel<KernelData>::GetInstance()->ArchitectureData->Pci;
        uint32_t current = pci->ReadConfig(pci->GetDeviceHeader(bus, slot, fun), offset);
        uint32_t shift = (offset & 3) * 8;
        uint32_t newVal = (current & ~(0xFF << shift)) | ((uint32_t)val << shift);
        pci->WriteConfig(pci->GetDeviceHeader(bus, slot, fun), offset, newVal);
    }

    void laihost_pci_writew(UNUSED uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint16_t val) {
        IO::PCI* pci = Kernel<KernelData>::GetInstance()->ArchitectureData->Pci;
        uint32_t current = pci->ReadConfig(pci->GetDeviceHeader(bus, slot, fun), offset);
        uint32_t shift = (offset & 2) * 8;
        uint32_t newVal = (current & ~(0xFFFF << shift)) | ((uint32_t)val << shift);
        pci->WriteConfig(pci->GetDeviceHeader(bus, slot, fun), offset, newVal);
    }

    void laihost_pci_writed(UNUSED uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint32_t val) {
        IO::PCI* pci = Kernel<KernelData>::GetInstance()->ArchitectureData->Pci;
        pci->WriteConfig(pci->GetDeviceHeader(bus, slot, fun), offset, val);
    }

    void laihost_sleep(uint64_t ms) {
        auto hpet = &Kernel<KernelData>::GetInstance()->ArchitectureData->Hpet;
        hpet->BusyWait(ms * 1000 * 1000); // Convert ms to ns for HPET
    }

    uint64_t laihost_timer(void) {
        auto hpet = &Kernel<KernelData>::GetInstance()->ArchitectureData->Hpet;
        return hpet->GetNanoseconds() / 100; // Return time in 100ns units for LAI
    }
}