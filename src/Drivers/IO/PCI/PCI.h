#ifndef BOREALOS_PCI_H
#define BOREALOS_PCI_H

#include <Definitions.h>

#define PCI_BAR_IS_MEM_ADDR(bar)   (((bar) & 0x1) == 0)
#define PCI_BAR_IS_IO_ADDR(bar)    (((bar) & 0x1) == 1)
#define PCI_BAR_IS_64BIT(bar)      ((((bar) >> 1) & 0x3) == 0x2)
#define PCI_BAR_MEM_ADDR(bar)      ((uint64_t)((bar) & 0xFFFFFFF0u))
#define PCI_BAR_IO_ADDR(bar)       ((uint64_t)((bar) & 0xFFFFFFFCu))
#define PCI_BAR_COMBINE_ADDR(high, low) \
    ((((uint64_t)(high)) << 32) | PCI_BAR_MEM_ADDR(low))

#define PCI_SUBCLASS_WIRELESS_ETHERNET_8021a     0x20
#define PCI_SUBCLASS_WIRELESS_ETHERNET_8021b     0x21
#define PCI_SUBCLASS_WIRELESS_CONSUMER_IR        0x01
#define PCI_SUBCLASS_WIRELESS_BLUETOOTH          0x11
#define PCI_SUBCLASS_WIRELESS_BROADBAND          0x12
#define PCI_SUBCLASS_WIRELESS_iRDA               0x00
#define PCI_SUBCLASS_WIRELESS_RF                 0x10
#define PCI_SUBCLASS_USB                         0x03

#define PCI_CONFIGSPACE_ADDRESS                  0xCF8
#define PCI_CONFIGSPACE_DATA                     0xCFC

#define PCI_HEADER_LATENCY_TIMER_OFFSET          0x0D
#define PCI_HEADER_SECONDARY_BUS_OFFSET          0x19
#define PCI_HEADER_MULTIFUNCTION_MASK            0x80
#define PCI_HEADER_CACHELINE_OFFSET              0x0C
#define PCI_HEADER_DEVICE_ID_OFFSET              0x02
#define PCI_HEADER_VENDOR_ID_OFFSET              0x00
#define PCI_HEADER_REVISION_OFFSET               0x08
#define PCI_HEADER_SUBCLASS_OFFSET               0x0A
#define PCI_HEADER_COMMAND_OFFSET                0x04
#define PCI_HEADER_HDRTYPE_OFFSET                0x0E
#define PCI_HEADER_PROGIF_OFFSET                 0x09
#define PCI_HEADER_STATUS_OFFSET                 0x06
#define PCI_HEADER_CLASS_OFFSET                  0x0B
#define PCI_HEADER_BIST_OFFSET                   0x0F

#define PCI_PROGIF_USB_UHCI                      0x00
#define PCI_PROGIF_USB_OHCI                      0x10
#define PCI_PROGIF_USB_EHCI                      0x20
#define PCI_PROGIF_USB_XHCI                      0x30

#define PCI_CLASS_SERIAL_BUS_CONTROLLER          0x0C
#define PCI_CLASS_WIRELESS_CONTROLLER            0x0D
#define PCI_CLASS_NETWORK_CONTROLLER             0x02

#define PCI_BAR_ADDR_START 0x10
#define PCI_BAR_ADDR_END 0x24
#define PCI_MAX_BARS 6

typedef struct {
    uint64_t Address;  // BAR address
    uint32_t RawLow;   // Raw low DWORD
    uint32_t RawHigh;  // Raw high DWORD (64-bit only)
    uint64_t Size;     // Size of the BAR
    bool Is64Bit;      // Only relevant for memory BARs
    bool Valid;        // True if BAR is implemented
    bool IsIO;         // If this is false, assume the BAR contains a memory address
} PCIBar;

typedef struct {
    PCIBar BARs[PCI_MAX_BARS];   // A list of this PCI device's Base Address Registers (BARs)
    uint16_t DeviceStatus;       // A 16-bit register used to store PCI bus events
    uint16_t DeviceID;           // Identifies this PCI device
    uint16_t VendorID;           // Identifies the manufacturer of this PCI device (see https://pcisig.com/membership/member-companies for valid values)
    uint16_t Command;            // Used to control a PCI device's ability to generate & respond to PCI cycles
    uint8_t CacheLineSize;       // Specifies the system cache line size in 32-bit units
    uint8_t LatencyTimer;        // Specifies the latency timer in units of PCI bus clocks
    uint8_t HeaderType;          // Identifies the layout of the rest of the header beginning at byte 0x10 (bit 7 set means the device has multipel functions; see https://wiki.osdev.org/PCI#PCI_Device_Structure)
    uint8_t RevisionID;          // Specifies the revision of the PCI device
    uint8_t SubClass;            // Specifies the specific PCI device function (READ ONLY)
    uint8_t Class;               // Specifies the general PCI device function (READ ONLY)
    uint8_t ProgIF;              // Programming Interface Byte; specifies a register-level programming interface (READ ONLY; NOT PRESENT IN ALL DEVICES)
    uint8_t BIST;                // Represents and allows control of a PCI device's Built-In Self Test
    uint8_t FunctionNumber;      // Represents the PCI function of this device
    uint8_t SlotNumber;          // Identifies the slot that this device is installed in
    uint8_t BusNumber;           // Identifies the bus that this device is connected to
} PCIDevice;

void PCIWriteConfigDWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
void PCIWriteConfigWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);
void PCIWriteConfigByte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t value);
uint32_t PCIReadConfigDWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t PCIReadConfigWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint8_t PCIReadConfigByte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

/// Scan for and initialize any devices that are connected to the PCI bus
Status PCIInit();
Status PCIMapDeviceBARs(PCIDevice* device);

// Scan all slots (32 max) in a PCI bus
// NOTE: This function uses the recursive scan method, which finds all valid buses in the system
void PCIScanBus(uint8_t bus);

#endif //BOREALOS_PCI_H