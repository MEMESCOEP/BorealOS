#include <IO/PCI.h>

namespace IO {
    PCI::PCIDeviceHeader PCI::GetDeviceHeader(uint8_t bus, uint8_t slot, uint8_t function) {
        PCIDeviceHeader header;

        header.bus      = bus;
        header.slot     = slot;
        header.function = function;
        header.vendorID = GetVendorID(bus, slot, function);
        header.deviceID = GetDeviceID(bus, slot, function);
        header.command  = ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_COMMAND);
        header.status   = ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_STATUS);

        // Revision ID and Prog IF share a word at 0x08
        uint16_t revisionWord = ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_REVISION);
        header.revisionID     = revisionWord & 0xFF;
        header.progIF         = (revisionWord >> 8) & 0xFF;

        // Class and subclass share a word at 0x0A
        uint16_t classWord = GetClassCode(bus, slot, function);
        header.subclass    = classWord & 0xFF;
        header.classCode   = (classWord >> 8) & 0xFF;

        // Cache line size and latency timer share a word at 0x0C
        uint16_t cacheWord   = ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_CACHE_WORD);
        header.cacheLineSize = cacheWord & 0xFF;
        header.latencyTimer  = (cacheWord >> 8) & 0xFF;

        // Header type and BIST share a word at 0x0E
        uint16_t headerWord = GetHeaderType(bus, slot, function);
        header.headerType   = headerWord & 0xFF;
        header.bist         = (headerWord >> 8) & 0xFF;

        return header;
    }

    PCI::PCI_BAR PCI::ReadBAR(const PCI::PCIDeviceHeader& device, uint8_t barIndex) {
        PCI_BAR bar = {};
        uint8_t offset = 0x10 + (barIndex * 4);

        // Disable memory and I/O decode during BAR probing to prevent devices from misinterpreting the all-1s write as an actual access
        uint16_t command = ReadConfigWord(device.bus, device.slot, device.function, PCI_HEADER_FLAG_COMMAND);
        WriteConfigDWord(device.bus, device.slot, device.function, PCI_HEADER_FLAG_COMMAND, command & ~0x3);

        // Make sure this bar is actualy implemented
        uint32_t barValue = ReadConfigDWord(device.bus, device.slot, device.function, offset);
        if (barValue == 0) {
            // Restore the command register
            WriteConfigDWord(device.bus, device.slot, device.function, PCI_HEADER_FLAG_COMMAND, command);
            return bar;
        }

        bar.isIOPort = barValue & 0x1;

        if (bar.isIOPort) {
            // This is an I/O port BAR, we need to mask the lower 2 bits
            bar.address = barValue & 0xFFFFFFFC;

            // Write all 1s to get the size
            WriteConfigDWord(device.bus, device.slot, device.function, offset, 0xFFFFFFFF);
            uint32_t readback = ReadConfigDWord(device.bus, device.slot, device.function, offset);
            WriteConfigDWord(device.bus, device.slot, device.function, offset, barValue); // Restore
            bar.size = ~(readback & 0xFFFFFFFC) + 1;
        }
        else {
            bar.isPrefetchable = (barValue >> 3) & 0x1;
            bar.is64Bit        = ((barValue >> 1) & 0x3) == 0x2;

            if (bar.is64Bit) {
                // Upper 32 bits are in the next BAR
                uint32_t barHigh = ReadConfigDWord(device.bus, device.slot, device.function, offset + 4);
                bar.address = ((uint64_t)barHigh << 32) | (barValue & 0xFFFFFFF0);

                // Write all 1s to both halves to get the size
                WriteConfigDWord(device.bus, device.slot, device.function, offset,     0xFFFFFFFF);
                WriteConfigDWord(device.bus, device.slot, device.function, offset + 4, 0xFFFFFFFF);
                uint32_t sizeLow  = ReadConfigDWord(device.bus, device.slot, device.function, offset);
                uint32_t sizeHigh = ReadConfigDWord(device.bus, device.slot, device.function, offset + 4);
                WriteConfigDWord(device.bus, device.slot, device.function, offset,     barValue); // Restore
                WriteConfigDWord(device.bus, device.slot, device.function, offset + 4, barHigh);  // Restore
                uint64_t size64 = ((uint64_t)sizeHigh << 32) | sizeLow;
                bar.size = ~(size64 & ~0xFULL) + 1;
            }
            else {
                // 32-bit MMIO — mask lower 4 bits
                bar.address = barValue & 0xFFFFFFF0;

                // Write all 1s to get the size
                WriteConfigDWord(device.bus, device.slot, device.function, offset, 0xFFFFFFFF);
                uint32_t readback = ReadConfigDWord(device.bus, device.slot, device.function, offset);
                WriteConfigDWord(device.bus, device.slot, device.function, offset, barValue); // Restore
                bar.size = ~(readback & 0xFFFFFFF0) + 1;
            }
        }

        // Restore the command register
        WriteConfigDWord(device.bus, device.slot, device.function, PCI_HEADER_FLAG_COMMAND, command);
        return bar;
    }

    uint32_t PCI::ReadConfig(const PCI::PCIDeviceHeader& device, uint8_t offset) {
        return ReadConfigDWord(device.bus, device.slot, device.function, offset);
    }

    uint8_t PCI::FindCapability(const PCI::PCIDeviceHeader& device, uint8_t capabilityID) {
        // Check if the device supports the capabilities list (status register bit 4)
        if (!(device.status & 0x10)) return 0;

        // Start at the capabilities pointer (offset 0x34)
        uint8_t offset = ReadConfigDWord(device.bus, device.slot, device.function, PCI_CAPABILITIES_POINTER) & 0xFF;

        // Walk the capabilities list
        while (offset != 0) {
            uint32_t capabilityDWord = ReadConfigDWord(device.bus, device.slot, device.function, offset);
            uint8_t  next = (capabilityDWord >> 8) & 0xFF;
            uint8_t  id = capabilityDWord & 0xFF;

            // Return the offset if it was found, otherwise keep walking
            if (id == capabilityID) return offset;
            offset = next;
        }

        // No offset was found
        return 0;
    }

    void PCI::FindDevicesByClass(uint8_t classCode, uint8_t subclass, Utility::List<PCIDeviceHeader*>& results) {
        if (!_PCIDevices) return;

        for (size_t i = 0; i < _PCIDevices->Size(); i++) {
            if ((*_PCIDevices)[i].classCode == classCode &&
                (*_PCIDevices)[i].subclass == subclass)
                results.Add(&(*_PCIDevices)[i]);
        }
    }

    void PCI::FindDevicesByID(uint16_t vendorID, uint16_t deviceID, Utility::List<PCIDeviceHeader*>& results) {
        if (!_PCIDevices) return;

        for (size_t i = 0; i < _PCIDevices->Size(); i++) {
            if ((*_PCIDevices)[i].vendorID == vendorID &&
                (*_PCIDevices)[i].deviceID == deviceID)
                results.Add(&(*_PCIDevices)[i]);
        }
    }

    uint32_t PCI::ReadConfigDWord(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
        uint32_t func32 = (uint32_t)function;
        uint32_t slot32 = (uint32_t)slot;
        uint32_t bus32  = (uint32_t)bus;

        uint32_t configAddress = (bus32 << 16) | (slot32 << 11) |
            (func32 << 8) | (offset & 0xFC) | 0x80000000;

        Serial::outl(PCI_CONFIG_ADDRESS, configAddress);
        return Serial::inl(PCI_CONFIG_DATA);
    }

    // Returns the selected property of the 32-bit configuration word from device <bus>:<slot>:<func>
    uint16_t PCI::ReadConfigWord(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
        uint32_t configAddress;

        // Convert to 32-bit unsigned integers
        uint32_t func32 = (uint32_t)function;
        uint32_t slot32 = (uint32_t)slot;
        uint32_t bus32 = (uint32_t)bus;
    
        // Create a configuration address based on Configuration Space Access Mechanism #1
        configAddress = (uint32_t)((bus32 << 16) | (slot32 << 11) |
                (func32 << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    
        // Write the config address to the PCI bridge
        Serial::outl(PCI::PCI_CONFIG_ADDRESS, configAddress);

        // Read in the data
        // NOTE: "(offset & 2) * 8) = 0" chooses the first word of the 32-bit register
        return (uint16_t)((Serial::inl(PCI::PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
    }

    uint16_t PCI::GetHeaderType(uint8_t bus, uint8_t slot, uint8_t function) {
        return ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_HEADER_TYPE);
    }
    
    uint16_t PCI::GetClassCode(uint8_t bus, uint8_t slot, uint8_t function) {
        return ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_CLASS_CODE);
    }

    uint16_t PCI::GetVendorID(uint8_t bus, uint8_t slot, uint8_t function) {
        return ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_VENDOR_ID);
    }

    uint16_t PCI::GetDeviceID(uint8_t bus, uint8_t slot, uint8_t function) {
        return ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_DEVICE_ID);
    }

    bool PCI::EnableMSI(const PCI::PCIDeviceHeader& device, uint64_t messageAddr, uint32_t messageData) {
        // Get the capability offset
        uint8_t capabilityOffset = FindCapability(device, PCI::PCI_CAPABILITY_MSI);
        if (capabilityOffset == 0) return false;

        // Read the message control dword
        uint16_t control = (ReadConfigDWord(device.bus, device.slot, device.function, capabilityOffset) >> 16) & 0xFFFF;
        bool is64Bit = (control >> 7) & 0x01;

        // Write the message address
        WriteConfigDWord(device.bus, device.slot, device.function, capabilityOffset + 0x04, (uint32_t)(messageAddr & 0xFFFFFFFF));

        if (is64Bit) {
            WriteConfigDWord(device.bus, device.slot, device.function, capabilityOffset + 0x8, (uint32_t)(messageAddr >> 32));
            WriteConfigDWord(device.bus, device.slot, device.function, capabilityOffset + 0xC, messageData);
        }
        else {
            WriteConfigDWord(device.bus, device.slot, device.function, capabilityOffset + 0x8, messageData);
        }

        // Enable MSI by setting bit 0 in the message control
        control |= 0x01;
        uint32_t dword0 = ReadConfigDWord(device.bus, device.slot, device.function, capabilityOffset);
        WriteConfigDWord(device.bus, device.slot, device.function, capabilityOffset, (dword0 & 0x0000FFFF) | ((uint32_t)control << 16));
        return true;
    }

    bool PCI::EnableMSIX(const PCI::PCIDeviceHeader& device, uint64_t messageAddr, uint32_t messageData, uint16_t entryIndex) {
        uint8_t capabilityOffset = FindCapability(device, PCI_CAPABILITY_MSI_X);
        if (capabilityOffset == 0) return false;

        // Read the message control word (upper 16 bits of the first dword)
        uint32_t dword0   = ReadConfigDWord(device.bus, device.slot, device.function, capabilityOffset);
        uint16_t control  = (dword0 >> 16) & 0xFFFF;
        uint16_t tableSize = (control & 0x7FF) + 1; // Bits 10:0 are N-1 encoded

        if (entryIndex >= tableSize) return false;

        // Read the table BIR and offset (second dword)
        uint32_t tableDWord = ReadConfigDWord(device.bus, device.slot, device.function, capabilityOffset + 0x4);
        uint8_t  barIndex   = tableDWord & 0x7;        // Bits 2:0 is "which BAR"
        uint32_t tableOffset = tableDWord & 0xFFFFFFF8; // Bits 31:3 is the offset into the BAR

        // Read the BAR to get the physical base address
        PCI_BAR bar = ReadBAR(device, barIndex);
        if (bar.address == 0) return false;

        // Map the MMIO region
        _paging->MapPage(
            bar.address,
            bar.address,
            Memory::PageFlags::ReadWrite | Memory::PageFlags::NoExecute | Memory::PageFlags::CacheDisable
        );

        volatile MSIXTableEntry* table = reinterpret_cast<volatile MSIXTableEntry*>(bar.address + tableOffset);

        // Write the message address and data into the entry
        table[entryIndex].messageAddrLow  = (uint32_t)(messageAddr & 0xFFFFFFFF);
        table[entryIndex].messageAddrHigh = (uint32_t)(messageAddr >> 32);
        table[entryIndex].messageData     = messageData;
        table[entryIndex].vectorControl   = 0; // Unmask the entry

        // Enable MSI-X by setting bit 15 of the message control and clearing function mask (bit 14)
        control |=  (1 << 15);
        control &= ~(1 << 14);
        WriteConfigDWord(device.bus, device.slot, device.function, capabilityOffset, (dword0 & 0x0000FFFF) | ((uint32_t)control << 16));

        return true;
    }

    void PCI::WriteConfig(const PCI::PCIDeviceHeader& device, uint8_t offset, uint32_t value) {
        WriteConfigDWord(device.bus, device.slot, device.function, offset, value);
    }

    void PCI::WriteConfigDWord(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint32_t value) {
        uint32_t func32 = (uint32_t)function;
        uint32_t slot32 = (uint32_t)slot;
        uint32_t bus32  = (uint32_t)bus;

        uint32_t configAddress = (bus32 << 16) | (slot32 << 11) |
            (func32 << 8) | (offset & 0xFC) | 0x80000000;

        Serial::outl(PCI_CONFIG_ADDRESS, configAddress);
        Serial::outl(PCI_CONFIG_DATA, value);
    }

    void PCI::CheckFunction(uint8_t bus, uint8_t slot, uint8_t function) {
        PCI::PCIDeviceHeader header = GetDeviceHeader(bus, slot, function);

        // Store the device
        _PCIDevices->Add(header);

        LOG_DEBUG("PCI %u8:%u8.%u8:\n\r  * Vendor ID: 0x%x8\n\r  * Device ID: 0x%x8\n\r  * Class: 0x%x8\n\r  * Subclass: 0x%x8\n\r",
            header.bus, header.slot, header.function,
            header.vendorID, header.deviceID, header.classCode, header.subclass);

        // If this is a PCI-to-PCI bridge, scan the secondary bus it exposes
        if (header.classCode == PCI_CLASS_BRIDGE && header.subclass == PCI_SUBCLASS_PCI_PCI_BRIDGE) {
            // The secondary bus is at offset 0x18, upper byte of the word
            uint8_t secondaryBus = (ReadConfigWord(bus, slot, function, PCI_OFFSET_SECONDARY_BUS) >> 8) & 0xFF;
            CheckBus(secondaryBus);
        }
    }

    void PCI::CheckSlot(uint8_t bus, uint8_t slot) {
        if (GetVendorID(bus, slot, 0) == PCI_VENDOR_ID_UNPOPULATED)
            return;

        CheckFunction(bus, slot, 0);

        uint8_t headerType = GetHeaderType(bus, slot, 0) & 0xFF;
        if (headerType & PCI_HEADER_FLAG_MULTIFUNCTION) {
            // This is a multifunction slot, functions 1-7 may contain additional devices
            // NOTE: We already checked function 0 above
            for (uint8_t function = 1; function < 8; function++) {
                if (GetVendorID(bus, slot, function) != PCI_VENDOR_ID_UNPOPULATED)
                    CheckFunction(bus, slot, function);
            }
        }
    }

    void PCI::CheckBus(uint8_t bus) {
        // A PCI bus can have up to 32 slots
        for (uint8_t slot = 0; slot < 32; slot++)
            CheckSlot(bus, slot);
    }

    // NOTE: MSI, MSI-X, and BAR reading are done per-driver, so we don't do that here
    void PCI::Initialize() {
        // Initialize the PCI devices array, we store up to 256 devices
        _PCIDevices = new Utility::List<PCIDeviceHeader>(256);

        // We start by enumerating the PCI bus via the recursive bus enumeration method
        // NOTE: This is more efficient than blindly scanning every single bus/slot combo, as we only check populated buses
        uint8_t headerType = GetHeaderType(0, 0, 0) & 0xFF;

        if (!(headerType & PCI_HEADER_FLAG_MULTIFUNCTION)) {
            // This is a single PCI host controller, only bus 0 exists at the root
            CheckBus(0);
        }
        else {
            // This is a multifunction host bridge, each function is a separate bus
            // NOTE: There can be up to 8 functions
            for (uint8_t function = 0; function < 8; function++) {
                // Functions are contiguous, so we have to break here instead of continuing
                if (GetVendorID(0, 0, function) == PCI_VENDOR_ID_UNPOPULATED)
                    break;

                CheckBus(function);
            }
        }

        LOG_INFO("Discovered and stored %u8 PCI device(s).", _PCIDevices->Size());
    }

    PCI::PCI(Memory::Paging* paging) {
        _paging = paging;
    }
}