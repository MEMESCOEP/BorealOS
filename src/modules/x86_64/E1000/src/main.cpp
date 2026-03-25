#include <Module.h>

#include <Utility/List.h>
#include "../../../modules/Common/Networking/Service.h"
#include "../Service.h"
#include "E1000Definitions.h"
#include "KernelData.h"

MODULE(E1000_MODULE_NAME, E1000_MODULE_DESCRIPTION, E1000_MODULE_VERSION, E1000_MODULE_IMPORTANCE);

Kernel<KernelData>* kernel;
IO::PCI* pciInstance = nullptr;
IO::PCI::PCIDeviceHeader* _device = nullptr;
IO::PCI::PCI_BAR bar0;
volatile uint32_t* _mmioBase = nullptr;
uint8_t _mac[6] = {};

uint32_t ReadRegister(uint32_t offset) {
    return _mmioBase[offset / 4];
}

void WriteRegister(uint32_t offset, uint32_t value) {
    _mmioBase[offset / 4] = value;
}

uint16_t ReadEEPROM(uint8_t address) {
    WriteRegister(E1000_REGISTER_EEPROM, E1000_EEPROM_READ_START | ((uint32_t)address << 8));
    uint32_t result;
    while (!((result = ReadRegister(E1000_REGISTER_EEPROM)) & E1000_EEPROM_READ_DONE));
    return (result >> 16) & 0xFFFF;
}

void ReadMAC() {
    uint16_t word0 = ReadEEPROM(0);
    uint16_t word1 = ReadEEPROM(1);
    uint16_t word2 = ReadEEPROM(2);

    _mac[0] = word0 & 0xFF;
    _mac[1] = (word0 >> 8) & 0xFF;
    _mac[2] = word1 & 0xFF;
    _mac[3] = (word1 >> 8) & 0xFF;
    _mac[4] = word2 & 0xFF;
    _mac[5] = (word2 >> 8) & 0xFF;

    LOG_DEBUG("E1000 MAC address is %x16:%x16:%x16:%x16:%x16:%x16",
        _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
}

void Reset() {
    LOG_DEBUG("Resetting E1000 NIC(s)...");

    // Trigger a reset
    WriteRegister(E1000_REGISTER_CONTROL, ReadRegister(E1000_REGISTER_CONTROL) | E1000_CONTROL_RESET);

    // Wait for the reset to complete
    while(ReadRegister(E1000_REGISTER_CONTROL) & E1000_CONTROL_RESET);

    // Finally, set the link to UP and enable automatic speed detection
    WriteRegister(E1000_REGISTER_CONTROL, ReadRegister(E1000_REGISTER_CONTROL) | E1000_CONTROL_AUTO_SPEED_DETECT | E1000_CONTROL_SET_LINK_UP);
}

// RELY_ON(EXTERNAL_MODULE("Some other module", VERSION(0,0,1)));
COMPATIBLE_FUNC() {
    kernel = Kernel<KernelData>::GetInstance();
    if (!kernel) {
        LOG_ERROR("Failed to get kernel instance!");
        return false;
    }

    pciInstance = kernel->ArchitectureData->Pci;
    if (!pciInstance) {
        LOG_ERROR("Failed to get PCI instance!");
        return false;
    }

    // The E1000 is a PCI card, so PCI must be fully initialized first
    if (!pciInstance->initialized) {
        LOG_ERROR("Cannot initialize Intel E1000 NIC(s) because PCI is not initialized!");
        return false;
    }

    // There needs to be an Intel E1000 device present
    Utility::List<IO::PCI::PCIDeviceHeader*> results;
    pciInstance->FindDevicesByID(E1000_VENDOR_ID, E1000_DEVICE_ID, results);
    if (results.Size() == 0) {
        LOG_ERROR("There are no Intel E1000 NICs to initialize.");
        return false;
    }

    return true;
}

LOAD_FUNC() {
    // Find the E1000 device
    Utility::List<IO::PCI::PCIDeviceHeader*> results;
    pciInstance->FindDevicesByID(E1000_VENDOR_ID, E1000_DEVICE_ID, results);
    _device = results[0];

    // Read BAR0 for the MMIO base address
    IO::PCI::PCI_BAR bar0 = pciInstance->ReadBAR(*_device, 0);
    if (bar0.address == 0 || bar0.isIOPort) {
        LOG_ERROR("E1000 BAR0 is not a valid MMIO region!");
        return STATUS::FAILURE;
    }

    // Map the MMIO region before accessing it
    kernel->ArchitectureData->Paging.MapPage(
        bar0.address,
        bar0.address,
        Memory::PageFlags::ReadWrite | Memory::PageFlags::NoExecute | Memory::PageFlags::CacheDisable
    );

    LOG_DEBUG("E1000 MMIO base is at 0x%x64, with a size of 0x%x64", bar0.address, bar0.size);
    _mmioBase = reinterpret_cast<volatile uint32_t*>(bar0.address);

    // Enable bus mastering so the NIC can DMA
    uint32_t command = pciInstance->ReadConfig(*_device, IO::PCI::PCI_HEADER_FLAG_COMMAND);
    pciInstance->WriteConfig(*_device, IO::PCI::PCI_HEADER_FLAG_COMMAND, command | 0x4);

    // Read the status register to confirm MMIO is working
    uint32_t status = _mmioBase[E1000_REGISTER_STATUS / 4];
    LOG_DEBUG("E1000 device status: 0x%x32", status);

    // Reset the NIC and read the MAC address
    Reset();
    ReadMAC();

    // Register the E1000 with the networking service
    Networking::NetworkService *networking = static_cast<Networking::NetworkService*>(
        Core::ServiceManager::GetInstance()->GetService(NETWORKING_SERVICE_NAME));

    if (!networking) {
        LOG_ERROR("Networking service is not available!");
        return STATUS::FAILURE;
    }

    Networking::NetworkInterface iface {
        .ID            = (uint64_t)_device,
        .type          = Networking::InterfaceType::Ethernet,
        .state         = Networking::InterfaceState::Down,
        .vendorID      = E1000_VENDOR_ID,
        .productID     = E1000_DEVICE_ID,
        .macAddress    = { _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5] },
        .interfaceName = "eth0",
        .manufacturer  = "Intel",
        .description   = "Intel E1000 Gigabit Ethernet",
        .mtu           = 1500,
        .txFunc        = [](const Networking::Packet* packet) -> STATUS {
            // TODO: Write to the E1000 TX descriptor ring here.
            // For now, just log that we would send the packet.
            LOG_DEBUG("E1000 TX: %u64 bytes on interface %u64", packet->length, packet->interfaceID);
            return STATUS::SUCCESS;
        }
    };

    STATUS result = networking->RegisterInterface(&iface);
    if (result != STATUS::SUCCESS) {
        LOG_ERROR("Failed to register Intel E1000 as a network interface!");
        return result;
    }

    // Bring the link up
    networking->SetInterfaceState(iface.ID, Networking::InterfaceState::Up);
    LOG_DEBUG("Intel E1000 NIC initialized and registered as '%s'.", iface.interfaceName);

    LOG_DEBUG("Intel E1000 NIC(s) initialized.");
    return STATUS::SUCCESS;
}