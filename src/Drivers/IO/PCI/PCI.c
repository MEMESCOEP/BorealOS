#include "PCI.h"
#include <Utility/SerialOperations.h>
#include <Core/Memory/HeapAllocator.h>
#include <Core/Memory/Paging.h>
#include <Core/Kernel.h>
#include <Drivers/CPU.h>

#define INITIAL_DEVICE_ARRAY_SIZE 8

PCIDevice* PCIDevices;
uint8_t* UniqueBuses;
uint32_t deviceArraySize = INITIAL_DEVICE_ARRAY_SIZE;
uint32_t busArraySize = INITIAL_DEVICE_ARRAY_SIZE;
uint32_t deviceCount = 0;
uint16_t busCount = 0;
uint16_t maxBuses = 256;
uint8_t maxSlots = 32;

// NOTE: This function uses PCI Configuration Space Access Mechanism #1
void PCIWriteConfigDWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    // Construct the 32-bit configuration address
    uint32_t address = (uint32_t)(
        (bus  << 16) |      // Bus number
        (slot << 11) |      // Device/slot number
        (func << 8)  |      // Function number
        (offset & 0xFC) |   // Register offset, 4-byte aligned
        0x80000000          // Enable bit
    );

    // Tell the PCI controller which configuration register we want to access
    outl(PCI_CONFIGSPACE_ADDRESS, address);

    // Write the 32-bit value
    outl(PCI_CONFIGSPACE_DATA, value);
}

// NOTE: This function uses PCI Configuration Space Access Mechanism #1
void PCIWriteConfigWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    // Construct the 32-bit configuration address
    uint32_t address = (uint32_t)(
        (bus  << 16) |      // Bus number
        (slot << 11) |      // Device/slot number
        (func << 8)  |      // Function number
        (offset & 0xFC) |   // Register offset, 4-byte aligned
        0x80000000          // Enable bit
    );

    // Tell the PCI controller which configuration register we want to access
    outl(PCI_CONFIGSPACE_ADDRESS, address);

    // Read the current 32-bit value first so we can preserve upper/lower half if needed
    uint32_t currentData = inl(PCI_CONFIGSPACE_DATA);

    // Determine which half of the dword to modify
    uint32_t shift = (offset & 2) * 8;

    // Clear the 16 bits we want to write, then OR in the new value
    currentData &= ~(0xFFFF << shift);
    currentData |= ((uint32_t)value) << shift;

    // Write the updated 32-bit value back
    outl(PCI_CONFIGSPACE_DATA, currentData);
}

void PCIWriteConfigByte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t value) {
    // Construct the 32-bit configuration address
    uint32_t address = (uint32_t)(
        (bus  << 16) |      // Bus number
        (slot << 11) |      // Device/slot number
        (func << 8)  |      // Function number
        (offset & 0xFC) |   // Register offset, 4-byte aligned
        0x80000000          // Enable bit
    );

    // Tell the PCI controller which configuration register we want to access
    outl(PCI_CONFIGSPACE_ADDRESS, address);

    // Read the current 32-bit value first so we can preserve other bytes
    uint32_t currentData = inl(PCI_CONFIGSPACE_DATA);

    // Determine which byte of the dword to modify
    uint32_t shift = (offset & 3) * 8;

    // Clear the 8 bits we want to write, then OR in the new value
    currentData &= ~(0xFF << shift);
    currentData |= ((uint32_t)value) << shift;

    // Write the updated 32-bit value back
    outl(PCI_CONFIGSPACE_DATA, currentData);
}

// NOTE: This function uses configspace access mechanism #1
uint32_t PCIReadConfigDWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    // Cast 8-bit values to 32-bit ones for safe shifting later
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
  
    // Construct the 32-bit configuration address
    // NOTE: This address has the following format:
    //  Bit  31:    Enable bit      (must be 1)
    //  Bits 30-24: Reserved        (must be 0)
    //  Bits 23-16: Bus number
    //  Bits 15-11: Device number   (often called the slot number)
    //  Bits 10-8:  Function number
    //  Bits 7-2:   Register offset (must be 4-byte aligned)
    //  Bits 1-0:   Must be zero; for 32-bit alignment purposes
    uint32_t address = (uint32_t)((
        lbus << 16) |          // Bus number
        (lslot << 11) |        // Device number
        (lfunc << 8) |         // Function number
        (offset & 0xFC) |      // Register offset; 4-byte boundary aligned
        ((uint32_t)0x80000000) // Enable bit
    );
  
    // Tell the PCI chipset which configuration register we want to access
    outl(PCI_CONFIGSPACE_ADDRESS, address);

    // Return the config data
    return inl(PCI_CONFIGSPACE_DATA);
}

// Function adapted from https://wiki.osdev.org/PCI#Configuration_Space_Access_Mechanism_#1
// NOTE: This function uses configspace access mechanism #1
uint16_t PCIReadConfigWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    // Cast 8-bit values to 32-bit ones for safe shifting later
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
  
    // Construct the 32-bit configuration address
    // NOTE: This address has the following format:
    //  Bit  31:    Enable bit      (must be 1)
    //  Bits 30-24: Reserved        (must be 0)
    //  Bits 23-16: Bus number
    //  Bits 15-11: Device number   (often called the slot number)
    //  Bits 10-8:  Function number
    //  Bits 7-2:   Register offset (must be 4-byte aligned)
    //  Bits 1-0:   Must be zero; for 32-bit alignment purposes
    uint32_t address = (uint32_t)((
        lbus << 16) |          // Bus number
        (lslot << 11) |        // Device number
        (lfunc << 8) |         // Function number
        (offset & 0xFC) |      // Register offset; 4-byte boundary aligned
        ((uint32_t)0x80000000) // Enable bit
    );
  
    // Tell the PCI chipset which configuration register we want to access
    outl(PCI_CONFIGSPACE_ADDRESS, address);

    // Read a dword (32-bit) value in from the config data port
    uint32_t data = inl(PCI_CONFIGSPACE_DATA);

    // Determine whether to take the lower or upper 16 bits of the 32-bit value:
    //   - If (offset & 2) == 0, low  word (bits 15:0)
    //   - If (offset & 2) == 2, high word (bits 31:16)
    return (uint16_t)(((data >> (((offset & 2) * 8))) & 0xFFFF));
}

// Adapted version of PCIReadConfigWord; reads a byte (8 bits) instead of a word (16 bits)
// NOTE: This function uses configspace access mechanism #1
uint8_t PCIReadConfigByte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    // Construct the 32-bit configuration address
    // NOTE: This address has the following format:
    //  Bit  31:    Enable bit      (must be 1)
    //  Bits 30-24: Reserved        (must be 0)
    //  Bits 23-16: Bus number
    //  Bits 15-11: Device number   (often called the slot number)
    //  Bits 10-8:  Function number
    //  Bits 7-2:   Register offset (must be 4-byte aligned)
    //  Bits 1-0:   Must be zero; for 32-bit alignment purposes
    uint32_t address = (uint32_t)(
        (bus << 16) |     // Bus number
        (slot << 11) |    // Device number
        (func << 8) |     // Function number
        (offset & 0xFC) | // Register offset; 4-byte boundary aligned
        0x80000000        // Enable bit
    );

    // Tell the PCI chipset which configuration register we want to access
    outl(PCI_CONFIGSPACE_ADDRESS, address);
    
    // Read a dword (32-bit) value in from the config data port
    uint32_t data = inl(PCI_CONFIGSPACE_DATA);
    
    // Calculate which byte within the 32-bit value we want
    // (offset & 3) extracts the byte index [0..3]
    // Multiply by 8 to convert the byte index into a bit shift count
    uint8_t shift = (offset & 3) * 8;

    // Shift the dword to align the target byte to bits [7:0], then mask and return it
    return (data >> shift) & 0xFF;
}

// Add information about a specific PCI device to the list and resize the list as necessary
Status PCIStoreDevice(PCIDevice DeviceInfoStruct) {
    // Make sure the device's vendor ID is valid before adding it to the list
    if (DeviceInfoStruct.VendorID == 0x0000 || DeviceInfoStruct.VendorID == 0xFFFF) {
        return STATUS_INVALID_PARAMETER;
    }

    // Make sure the device's header type is valid (low 7 bits should be 0x0, 0x1, or 0x2)
    if ((DeviceInfoStruct.HeaderType & 0x7F) > 0x02) {
        return STATUS_INVALID_PARAMETER;
    }

    // Resize the PCI device buffer when it gets full
    // NOTE: The array doubles in size because it's more efficient; memory allocations are very expensive, so just allocating more in one go means we can avoid doing this work later when timing is critical
    if (deviceCount >= deviceArraySize) {
        deviceArraySize *= 2;
        PCIDevices = HeapRealloc(&Kernel.Heap, PCIDevices, sizeof(PCIDevice) * deviceArraySize);

        if (!PCIDevices) {
            LOG(LOG_WARNING, "PCI devices array allocation failed!\n");
            return STATUS_FAILURE;
        }
    }

    // Finally, store the device info struct in the list
    PCIDevices[deviceCount] = DeviceInfoStruct;
    deviceCount++;
    return STATUS_SUCCESS;
}

Status PCIStoreUniqueBus(uint8_t bus) {
    // Resize the unique buses buffer when it gets full
    // NOTE: The array doubles in size because it's more efficient; memory allocations are very expensive, so just allocating more in one go means we can avoid doing this work later when timing is critical
    if (busCount >= busArraySize) {
        busArraySize *= 2;
        UniqueBuses = HeapRealloc(&Kernel.Heap, UniqueBuses, sizeof(uint8_t) * busArraySize);

        if (!UniqueBuses) {
            LOG(LOG_WARNING, "PCI unique buses array allocation failed!\n");
            return STATUS_FAILURE;
        }
    }

    // Finally, store the bus in the array
    UniqueBuses[busCount] = bus;
    return STATUS_SUCCESS;
}

// Populate a PCI device info struct based on the configspace values it provides
Status PCIPopulateDevice(uint8_t bus, uint8_t slot, uint8_t function, PCIDevice* dev) {
    // Make sure the device's vendor ID is valid before adding it to the list
    uint16_t vendorID = PCIReadConfigWord(bus, slot, function, PCI_HEADER_VENDOR_ID_OFFSET);
    if (vendorID == 0x0000 || vendorID == 0xFFFF) return STATUS_INVALID_PARAMETER;

    // Make sure the device's header type is valid (low 7 bits should be 0x0, 0x1, or 0x2)
    uint8_t headerType = PCIReadConfigByte(bus, slot, function, PCI_HEADER_HDRTYPE_OFFSET);
    if ((headerType & 0x7F) > 0x02) return STATUS_INVALID_PARAMETER;

    dev->VendorID       = vendorID;
    dev->DeviceID       = PCIReadConfigWord(bus, slot, function, PCI_HEADER_DEVICE_ID_OFFSET);
    dev->Command        = PCIReadConfigWord(bus, slot, function, PCI_HEADER_COMMAND_OFFSET);
    dev->DeviceStatus   = PCIReadConfigWord(bus, slot, function, PCI_HEADER_STATUS_OFFSET);
    dev->RevisionID     = PCIReadConfigByte(bus, slot, function, PCI_HEADER_REVISION_OFFSET);
    dev->ProgIF         = PCIReadConfigByte(bus, slot, function, PCI_HEADER_PROGIF_OFFSET);
    dev->SubClass       = PCIReadConfigByte(bus, slot, function, PCI_HEADER_SUBCLASS_OFFSET);
    dev->Class          = PCIReadConfigByte(bus, slot, function, PCI_HEADER_CLASS_OFFSET);
    dev->CacheLineSize  = PCIReadConfigByte(bus, slot, function, PCI_HEADER_CACHELINE_OFFSET);
    dev->LatencyTimer   = PCIReadConfigByte(bus, slot, function, PCI_HEADER_LATENCY_TIMER_OFFSET);
    dev->HeaderType     = headerType;
    dev->BIST           = PCIReadConfigByte(bus, slot, function, PCI_HEADER_BIST_OFFSET);
    dev->FunctionNumber = function;
    dev->SlotNumber     = slot;
    dev->BusNumber      = bus;

    return STATUS_SUCCESS;
}

void PCIPrintDeviceInfo(PCIDevice* dev) {
    PRINTF("<===[ \033[0;38;2;255;0;255;49mPCI DEVICE %u:%u:%u\033[0m ]===>\n", (uint64_t)dev->BusNumber, (uint64_t)dev->SlotNumber, (uint64_t)dev->FunctionNumber);
    PRINTF("| Cache line size: %p\n", (uint64_t)dev->CacheLineSize);
    PRINTF("| Latency timer: %p\n", (uint64_t)dev->LatencyTimer);
    PRINTF("| Header type: %p\n", (uint64_t)dev->HeaderType);
    PRINTF("| Base class: %p\n", (uint64_t)dev->Class);
    PRINTF("| Subclass: %p\n", (uint64_t)dev->SubClass);
    PRINTF("| Revision: %p\n", (uint64_t)dev->RevisionID);
    PRINTF("| Command: %p\n", (uint64_t)dev->Command);
    PRINTF("| Device: %p\n", (uint64_t)dev->DeviceID);
    PRINTF("| ProgIF: %p\n", (uint64_t)dev->ProgIF);
    PRINTF("| Vendor: %p\n", (uint64_t)dev->VendorID);
    PRINTF("| Status: %p\n", (uint64_t)dev->DeviceStatus);
    PRINTF("| BIST: %p\n\n", (uint64_t)dev->BIST);
}

void PCIDetectDeviceSpecialFunction(PCIDevice* device) {
    // We're only interested in devices with a class of 0x06
    if (device->Class != 0x06) return;

    // Get and validate the secondary bus
    uint8_t secondaryBus = PCIReadConfigByte(device->BusNumber, device->SlotNumber, device->FunctionNumber, PCI_HEADER_SECONDARY_BUS_OFFSET);

    if (secondaryBus == device->BusNumber) {
        PRINTF("\t* Secondary bus %u and current bus %u are the same, skipping...\n", (uint64_t)secondaryBus, (uint64_t)device->BusNumber);
        return;
    }

    if (secondaryBus == 0 || secondaryBus > maxBuses) {
        PRINTF("\t* Secondary bus %u is invalid or larger than %u, skipping this device...\n", (uint64_t)secondaryBus, (uint64_t)maxBuses);
        return;
    }

    // Check if the device's subclass is something we need to be interested in
    switch (device->SubClass) {
        case 0x00:
            PRINT("\t\t* PCI host controller detected.\n\n");
            break;

        case 0x04:
            PRINT("\t\t* PCI-PCI bridge detected.\n\n");
            break;

        default:
            return;
    }

    // It's now okay to scan the secondary bus
    busCount++;
    PCIScanBus(secondaryBus);
}

// Scan all slots (32 max) in a PCI bus
// NOTE: This function uses the recursive scan method, which finds all valid buses in the system
// NOTE: Function 0 must be handled first
void PCIScanBus(uint8_t bus) {
    for (uint8_t slot = 0; slot < maxSlots; slot++) {
        PCIDevice newDevice = {0};
        Status populateDevStatus = PCIPopulateDevice(bus, slot, 0, &newDevice);

        if (populateDevStatus != STATUS_SUCCESS) {
            if (populateDevStatus != STATUS_INVALID_PARAMETER) {
                PRINTF("\t\t* Failed to populate device info struct at bus #%u, slot #%u, function #%u (device may not exist or data may be invalid)\n", (uint64_t)bus, (uint64_t)slot, 0);
            }
            
            continue;
        }

        // Make sure the device information is correctly stored
        if (PCIStoreDevice(newDevice) != STATUS_SUCCESS) {
            PRINTF("\t\t* Failed to store device at bus #%u, slot #%u, function #%u (device data may be invalid)\n", (uint64_t)bus, (uint64_t)slot, 0);
            continue;
        }

        PRINTF("\t* Device found at %u:%u:%u; vendor=%p, class=%p, subclass=%p, dev_id=%p\n",
            (uint64_t)bus,
            (uint64_t)slot,
            0,
            (uint64_t)newDevice.VendorID,
            (uint64_t)newDevice.Class,
            (uint64_t)newDevice.SubClass,
            (uint64_t)newDevice.DeviceID
        );

        PCIDetectDeviceSpecialFunction(&newDevice);

        // Check if this is a multifunction device
        if ((newDevice.HeaderType & PCI_HEADER_MULTIFUNCTION_MASK) != 0) {
            for (uint8_t function = 1; function < 8; function++) {
                PCIDevice newFuncDevice = {0};
                populateDevStatus = PCIPopulateDevice(bus, slot, function, &newFuncDevice);

                if (populateDevStatus != STATUS_SUCCESS) {
                    if (populateDevStatus != STATUS_INVALID_PARAMETER) {
                        PRINTF("\t* Failed to populate device info struct at bus #%u, slot #%u, function #%u (device may not exist or data may be invalid)\n", (uint64_t)bus, (uint64_t)slot, (uint64_t)function);
                    }
                    
                    continue;
                }

                // Make sure the device information is correctly stored
                if (PCIStoreDevice(newFuncDevice) != STATUS_SUCCESS) {
                    PRINTF("\t* Failed to store device at bus #%u, slot #%u, function #%u (device data may be invalid)\n", (uint64_t)bus, (uint64_t)slot, (uint64_t)function);
                    continue;
                }

                PRINTF("\t* Device found at %u:%u:%u; vendor=%p, class=%p, subclass=%p, dev_id=%p\n",
                    (uint64_t)bus,
                    (uint64_t)slot,
                    (uint64_t)function,
                    (uint64_t)newFuncDevice.VendorID,
                    (uint64_t)newFuncDevice.Class,
                    (uint64_t)newFuncDevice.SubClass,
                    (uint64_t)newFuncDevice.DeviceID
                );

                PCIDetectDeviceSpecialFunction(&newFuncDevice);
            }
        }
    }
}

// Is the device a USB controller?
bool PCIIsUSBController(PCIDevice* dev) {
    return dev->Class == PCI_CLASS_SERIAL_BUS_CONTROLLER &&
           dev->SubClass == PCI_SUBCLASS_USB;
}

// Is the device a NIC?
/*bool PCIIsNIC(PCIDevice* dev) {
    return (dev->Class == || dev->Class == )  &&
           (dev->SubClass == || );
}*/

Status PCIInit() {
    // Create an initial array to store PCI device info structs
    LOG(LOG_INFO, "Initializing PCI device buffer...\n");
    PCIDevices = HeapAlloc(&Kernel.Heap, sizeof(PCIDevice) * 8);
    deviceCount = 0;

    if (!PCIDevices) {
        LOG(LOG_WARNING, "PCI devices array allocation failed!\n");
        return STATUS_FAILURE;
    }



    // Scan the PCI bus(es)
    LOG(LOG_INFO, "Scanning PCI bus(es)...\n");
    uint8_t headerType = PCIReadConfigByte(0, 0, 0, PCI_HEADER_HDRTYPE_OFFSET);

    // We need to check the function bits if the device identifies itself as multifunction; some devices have multiple PCI host controllers
    // NOTE: This only scans bus 0, finding more host controllers at other buses might be needed later
    if ((headerType & 0x80) == 0) {
        busCount++;
        PCIStoreUniqueBus(0);
        PCIScanBus(0);
    }
    else {
        for (uint8_t function = 0; function < 8; function++) {
            // Validate the vendor ID
            if (PCIReadConfigWord(0, 0, function, PCI_HEADER_VENDOR_ID_OFFSET) == 0xFFFF) continue;

            // Check if the system has multiple PCI host controllers
            uint8_t classCode = PCIReadConfigByte(0, 0, function, PCI_HEADER_CLASS_OFFSET);
            uint8_t subclass = PCIReadConfigByte(0, 0, function, PCI_HEADER_SUBCLASS_OFFSET);

            if (classCode == 0x06 && subclass == 0x00) {
                uint8_t secondaryBus = PCIReadConfigByte(0, 0, function, PCI_HEADER_SECONDARY_BUS_OFFSET);

                PRINTF("\t\t* Scanning PCI bus from extra PCI host controller (%u)...\n", (uint64_t)secondaryBus);
                busCount++;
                PCIStoreUniqueBus(secondaryBus);
                PCIScanBus(secondaryBus);
                
                PRINT("\t\t\t* PCI controller bus scan complete.\n\n");
            }
        }
    }
    
    PRINTF("\t* PCI scanning complete; detected %u device(s) on %u bus(es).\n\n", (uint64_t)deviceCount, (uint64_t)busCount);

    // Print each device's info (for debugging)
    /*for (uint32_t PCIDevIndex = 0; PCIDevIndex < deviceCount; PCIDevIndex++) {
        PCIPrintDeviceInfo(&PCIDevices[PCIDevIndex]);
    }*/



    // Map BARs
    LOGF(LOG_INFO, "Mapping BARs for %u device(s)...\n", (uint64_t)deviceCount);
    for (uint32_t PCIDevIndex = 0; PCIDevIndex < deviceCount; PCIDevIndex++) {
        PCIDevice* currentDevice = &PCIDevices[PCIDevIndex];
        PCIBar newBar;
        uint64_t barSize = 0;
        uint8_t storedBARIndex = 0;

        // Figure out how many BARs to map
        uint32_t maxBARs = currentDevice->HeaderType == 0x01 ? 2 : PCI_MAX_BARS;
        PRINTF("\t* Mapping %u BAR(s) for device at %u:%u:%u...\n", (uint64_t)maxBARs, (uint64_t)currentDevice->BusNumber, (uint64_t)currentDevice->SlotNumber, (uint64_t)currentDevice->FunctionNumber);

        // Read the BAR
        for (uint8_t barIndex = 0; barIndex < maxBARs; barIndex++) {
            uint8_t BAROffset = 0x10 + (barIndex * 4);
            newBar.RawLow = PCIReadConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset);
            newBar.RawHigh = 0x0;
            newBar.Is64Bit = PCI_BAR_IS_64BIT(newBar.RawLow);
            newBar.IsIO = PCI_BAR_IS_IO_ADDR(newBar.RawLow);
            newBar.Valid = true;
            barSize = 0;
            newBar.Size = 0;

            // Is the BAR implemented?
            if (newBar.RawLow == 0x0) {
                newBar.Valid = false;
                currentDevice->BARs[barIndex] = newBar;
                continue;
            }           

            // 64-bit BARs should always be memory BARs, they should never be marked as I/O
            if (newBar.Is64Bit == true && newBar.IsIO == true) {
                newBar.Valid = false;
                currentDevice->BARs[barIndex] = newBar;
                continue;
            }

            // Check if the bar is 64 bit
            if (newBar.Is64Bit == true) {
                // It's 64 bit, which means we read the next bar
                // NOTE: We need to skip the next BAR since it's part of this one
                newBar.RawHigh = PCIReadConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset + 4);
                barIndex++;
            }

            // Now that we have the bar, we need to check its type (memory mapped or direct I/O port)
            // NOTE: If bit 0 of the BAR is 0, it's a memory bar. If it's 1, it's an I/O bar
            if (newBar.IsIO == true) {
                PRINTF("\t\t* BAR #%u is an I/O port\n", (uint64_t)(newBar.Is64Bit == true ? barIndex - 1 : barIndex));
                newBar.Address = PCI_BAR_IO_ADDR(newBar.RawLow);
                newBar.Size = 0;

                PRINTF("\t\t\t* I/O BAR address is %p\n", newBar.Address);
            }
            else {
                PRINTF("\t\t* BAR #%u is memory mapped\n", (uint64_t)(newBar.Is64Bit == true ? barIndex - 1 : barIndex));
                newBar.Address = newBar.Is64Bit
                    ? PCI_BAR_COMBINE_ADDR(newBar.RawHigh, newBar.RawLow)
                    : PCI_BAR_MEM_ADDR(newBar.RawLow);

                PRINTF("\t\t\t* MEM BAR address is %p\n", newBar.Address);

                if (newBar.Is64Bit == true) {
                    uint32_t lowOrig = PCIReadConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset);
                    uint32_t highOrig = PCIReadConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset + 4);

                    PCIWriteConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset, 0xFFFFFFFF);
                    PCIWriteConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset + 4, 0xFFFFFFFF);

                    uint32_t lowMask = PCIReadConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset);
                    uint32_t highMask = PCIReadConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset + 4);

                    barSize = ~(PCI_BAR_COMBINE_ADDR(highMask, lowMask) & ~0xFULL) + 1;

                    // Restore original values
                    PCIWriteConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset, lowOrig);
                    PCIWriteConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset + 4, highOrig);
                } else {
                    uint32_t originalBAR = PCIReadConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset);
                    PCIWriteConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset, 0xFFFFFFFF);

                    uint32_t barMask = PCIReadConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset);
                    uint32_t size32 = ~(barMask & ~0xF) + 1; // we do this in 32 bits so it doesnt create a massive size (0xFFFFFFFXXXXXXXX) where X is the 32 bit size

                    barSize = (uint64_t)size32;
                    PCIWriteConfigDWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, BAROffset, originalBAR);
                }
                
                PRINTF("\t\t\t* MEM BAR size is %u bytes (%u KB)\n", barSize, (int)barSize / 1024);
                newBar.Size = barSize;

                // Reverse the pages from the PMM so we can't override them
                PhysicalMemoryManagerReserveRegion((void*)(uintptr_t)ALIGN_DOWN(newBar.Address, PMM_PAGE_SIZE), ALIGN_UP(BYTES_TO_PAGES(newBar.Size), 1));
            }

            // Store the BAR in the device's info struct's BAR array
            PRINT("\t\t\t* Storing BAR...\n");
            currentDevice->BARs[storedBARIndex] = newBar;
            storedBARIndex++;

            // Map the device's BARs
            if (PCIMapDeviceBARs(currentDevice) != STATUS_SUCCESS) {
                PRINTF("\t* Failed to map BARs for device at %u:%u:%u\n", (uint64_t)currentDevice->BusNumber, (uint64_t)currentDevice->SlotNumber, (uint64_t)currentDevice->FunctionNumber);
            }

            PRINT("\n");
        }
    }

    // Enable bus mastering for all valid controller types now
    LOGF(LOG_INFO, "Enabling bus mastering for up to %u devices...\n", (uint64_t)deviceCount);

    for (uint32_t PCIDevIndex = 0; PCIDevIndex < deviceCount; PCIDevIndex++) {
        PCIDevice* currentDevice = &PCIDevices[PCIDevIndex];
        bool skipDevice = true;

        // Figure out what device type we're working with
        if (PCIIsUSBController(currentDevice) == true) {
            // The device is a USB controller, let's determine the type
            PRINTF("\t* PCI USB controller detected at %u:%u:%u\n", (uint64_t)currentDevice->BusNumber, (uint64_t)currentDevice->SlotNumber, (uint64_t)currentDevice->FunctionNumber);

            switch (currentDevice->ProgIF) {
                case PCI_PROGIF_USB_UHCI:
                    PRINT("\t\t* USB controller is UHCI\n");
                    skipDevice = false;
                    break;

                case PCI_PROGIF_USB_OHCI:
                    PRINT("\t\t* USB controller is OHCI\n");
                    skipDevice = false;
                    break;

                case PCI_PROGIF_USB_EHCI:
                    PRINT("\t\t* USB controller is EHCI\n");
                    skipDevice = false;
                    break;

                case PCI_PROGIF_USB_XHCI:
                    PRINT("\t\t* USB controller is XHCI\n");
                    skipDevice = false;
                    break;

                default:
                    PRINTF("\t\t* USB controller type is unknown, or this is a usb device: %u\n\n", (uint64_t)currentDevice->ProgIF);
                    break;
            }
        }

        if (skipDevice == true) continue;

        // Enable bus mastering now
        uint16_t cmdRegisterData = PCIReadConfigWord(currentDevice->BusNumber,
            currentDevice->SlotNumber,
            currentDevice->FunctionNumber,
            PCI_HEADER_COMMAND_OFFSET
        );

        // Check if bus mastering is already enabled
        if (!(cmdRegisterData & 0x04)) {
            // It's not enabled, we can proceed. Now we set bit 2
            cmdRegisterData |= 0x04;

            // Finally, write the cmd data back to the device
            PCIWriteConfigWord(
                currentDevice->BusNumber,
                currentDevice->SlotNumber,
                currentDevice->FunctionNumber,
                PCI_HEADER_COMMAND_OFFSET,
                cmdRegisterData
            );
            
            PRINT("\t\t* Bus mastering is now enabled for this device\n\n");
        }
        else {
            PRINT("\t\t* Bus mastering is already enabled for this device, no need to re-enable it\n\n");
        }
    }

    // Enable Message Signaled Interrupts (MSI)
    // NOTE: This is NOT MSI-X! PCIe devices require MSI-X, but this is just regular PCI and thus only MSI is supported
    LOGF(LOG_INFO, "Enabling Message Signaled Interrupts (MSI) for up to %u devices...\n", (uint64_t)deviceCount);
    for (uint32_t PCIDevIndex = 0; PCIDevIndex < deviceCount; PCIDevIndex++) {
        PCIDevice* currentDevice = &PCIDevices[PCIDevIndex];
        bool devHasCapabilities = true;
        
        // Check if the device has a pointer to the capabilities list (status bit 4 set to 1)
        uint16_t devStatus = PCIReadConfigWord(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, PCI_HEADER_STATUS_OFFSET);

        if (devStatus & (1 << 4)) {
            PRINTF("\t* Capabilities pointer exists for device %u:%u:%u\n",
                (uint64_t)currentDevice->BusNumber,
                (uint64_t)currentDevice->SlotNumber,
                (uint64_t)currentDevice->FunctionNumber
            );

            // Get the device's capability register based on the header type
            // NOTE: CardBus bridge devices use 0x14 as the offset instead of 0x34
            uint8_t capabilitiesPointer = PCIReadConfigByte(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, (currentDevice->HeaderType == 0x02) ? 0x14 : 0x34);

            // Verify the capabilities pointer
            if (capabilitiesPointer == 0x0) {
                PRINTF("\t\t* Capabilities pointer device %u:%u:%u is invalid!\n\n",
                    (uint64_t)currentDevice->BusNumber,
                    (uint64_t)currentDevice->SlotNumber,
                    (uint64_t)currentDevice->FunctionNumber
                );

                devHasCapabilities = false;
                continue;
            }

            // Now walk the capabilities list
            uint8_t capabilityOffset = capabilitiesPointer;

            // NOTE: See https://pcisig.com/sites/default/files/files/PCI_Code-ID_r_1_11__v24_Jan_2019.pdf for capability IDs
            while (capabilityOffset != 0) {
                uint8_t capabilityID  = PCIReadConfigByte(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, capabilityOffset);
                uint8_t nextPtr = PCIReadConfigByte(currentDevice->BusNumber, currentDevice->SlotNumber, currentDevice->FunctionNumber, capabilityOffset + 1);

                switch (capabilityID) {
                    // --- STANDARD CAPABILITIES ---
                    case 0x00: // Null capability
                        PRINT("\t\t* This device has a NULL capability, its function is not specified\n");
                        break;

                    case 0x01: // Power Management
                        PRINT("\t\t* This device has power management capabilities (unsupported)\n");
                        break;

                    case 0x02: // AGP (Accelerated Graphics Port) features
                        PRINT("\t\t* This device can use AGP features (unsupported)\n");
                        break;

                    case 0x03: // VPD (Vital Product Data)
                        PRINT("\t\t* This device supports Vital Product Data (VPD) (unsupported)\n");
                        break;

                    case 0x04: // External expansion bridge
                        PRINT("\t\t* This device provides external expansion (unsupported)\n");
                        break;

                    case 0x05: // MSI
                        PRINT("\t\t* This device is MSI capable\n");
                        break;

                    case 0x06: // CompactPCI hot swap
                        PRINT("\t\t* This device is CompactPCI hot swap capable (unsupported)\n");
                        break;

                    case 0x07: // PCI-X
                        PRINT("\t\t* This is a PCI-X device (unsupported)\n");
                        break;

                    case 0x08: // HyperTransport
                        PRINT("\t\t* This device is HyperTransport capable (unsupported)\n");
                        break;
                    
                    case 0x09: // Vendor specific
                        PRINTF("\t\t* Vendor specific capability detected at offset %p\n", capabilityOffset);
                        break;

                    case 0x0A: // Debug port
                        PRINT("\t\t* This device is a debug port (unsupported)\n");
                        break;

                    case 0x0B: // CompactPCI central resource control
                        PRINT("\t\t* This device is a CompactPCI central resource control (unsupported)\n");
                        break;

                    case 0x0C: // PCI hot plug
                        PRINT("\t\t* This device is PCIe hot plug capable (unsupported)\n");
                        break;

                    case 0x0D: // PCI bridge subsystem vendor ID
                        PRINT("\t\t* This device is a PCI bridge with a subsystem vendor ID (unsupported)\n");
                        break;

                    case 0x0E: // AGP 8x
                        PRINT("\t\t* This is an AGP 8x device (unsupported)\n");
                        break;

                    case 0x0F: // Secure device
                        PRINT("\t\t* This device is a secure device (details unknown)\n");
                        break;

                    case 0x10: // PCIe
                        PRINT("\t\t* This is a PCIe device (unsupported)\n");
                        break;

                    case 0x11: // MSI-X
                        PRINT("\t\t* This device is MSI-X capable (unsupported)\n");
                        break;

                    case 0x12: // SATA data/index configuration
                        PRINT("\t\t* This device supports SATA data/index config (unsupported)\n");
                        break;

                    case 0x13: // PCIe advanced features
                        PRINT("\t\t* This device is PCIe Advanced Features capable (unsupported)\n");
                        break;

                    case 0x14: // Enhanced allocation
                        PRINT("\t\t* This device supports enhanced allocation (unsupported)\n");
                        break;

                    case 0x15: // Flattening portal bridge
                        PRINT("\t\t* This device is a flattening portal bridge (unsupported)\n");
                        break;

                    // --- EXTENDED CAPABILITIES ---
                    // To be implemented later

                    default: // Unknown / unimplemented / reserved
                        PRINTF("\t\t* Unknown / unimplemented / reserved capability ID %p at offset %p\n", capabilityID, capabilityOffset);
                        break;
                }

                capabilityOffset = nextPtr;
            }

            if (devHasCapabilities == true) PRINT("\n");
        }
        else {
            devHasCapabilities = false;
        }
    }

    return STATUS_SUCCESS;
}

Status PCIMapDeviceBARs(PCIDevice *device) {
    PCIBar* current = &device->BARs[0];
    uint8_t barIndex = 0;
    while (current->Valid) {
        if (current->IsIO) {
            PRINTF("\t\t\t* Skipping I/O BAR at address %p\n", current->Address);
            current++;
            barIndex++;
            continue;
        }

        if (current->Size == 0 || current->Size > UINT32_MAX) {
            PRINTF("\t\t\t* BAR at address %p has invalid size %u, cannot map it\n", (void*)(uintptr_t)current->Address, (uint64_t)current->Size);
            return STATUS_FAILURE;
        }

        // Map the BAR into the kernel's virtual address space
        uintptr_t aligned_bar_addr = ALIGN_DOWN(current->Address, PMM_PAGE_SIZE);
        uintptr_t aligned_bar_end = ALIGN_UP(current->Address + current->Size, PMM_PAGE_SIZE);

        if (aligned_bar_addr > UINT32_MAX || aligned_bar_end > UINT32_MAX) {
            PRINTF("\t\t\t* BAR at address %p is above 4GB, cannot map it in 32-bit address space\n", (void*)(uintptr_t)current->Address);
            return STATUS_FAILURE;
        }

        PRINTF("\t\t\t* Mapping %u:%u:%u BAR %u at physical address %p to virtual address %p (size: %u bytes)\n",
            (uint64_t)device->BusNumber,
            (uint64_t)device->SlotNumber,
            (uint64_t)device->FunctionNumber,
            (uint64_t)barIndex,
            (void*)(uintptr_t)current->Address,
            (void*)aligned_bar_addr,
            (uint64_t)(aligned_bar_end - aligned_bar_addr)
        );

        for (uintptr_t addr = aligned_bar_addr; addr < aligned_bar_end; addr += PMM_PAGE_SIZE) {
            Status mapState = PagingMapPage(&Kernel.Paging, (void*)addr, (void*)addr, true, false, PAGE_CACHE_DISABLE);
            if (mapState != STATUS_SUCCESS) {
                PRINTF("\t\t\t\t* Failed to map page at address %p for BAR mapping\n", (void*)addr);
                return STATUS_FAILURE;
            }
        }

        current++;
        barIndex++;
    }

    return STATUS_SUCCESS;
}
