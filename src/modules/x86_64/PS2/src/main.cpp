#include <Module.h>

#define HAS_SERVICE 0 // Set this to 1 if your module provides a service for other modules to use.
#include "../Service.h"

#include "Core/Firmware/ACPI.h"
#include "IO/Serial.h"
#include "KernelData.h"
#include "PS2Definitions.h"

MODULE(PS2_MODULE_NAME, PS2_MODULE_DESCRIPTION, PS2_MODULE_VERSION, PS2_MODULE_IMPORTANCE);

Kernel<KernelData>* kernel;
bool twoChannels = false;

// It is safe to call any kernel function wherever!

COMPATIBLE_FUNC() {
    kernel = Kernel<KernelData>::GetInstance();

    // Make sure we got a kernel instance
    if (!kernel) {
        LOG_ERROR("Failed to get kernel instance!");
        return false;
    }

    // Check if this system supports ACPI and has a PS/2 controller
    if (kernel->ArchitectureData->Acpi.ACPISupported() == false) {
        LOG_ERROR("This system doesn't support ACPI, no checks for PS/2 controllers can be performed!");
        return false;
    }

    void* fadtPtr = kernel->ArchitectureData->Acpi.GetTable("FADT");
    if (!fadtPtr) {
        LOG_ERROR("Failed to find FADT for PS/2 controller checks!");
        return false;
    }

    Core::Firmware::ACPI::FADT* fadt = (Core::Firmware::ACPI::FADT*)fadtPtr;
    if (fadt->BootArchitectureFlags != 0 && !(fadt->BootArchitectureFlags & (1 << 1))) {
        LOG_WARNING("There is no PS/2 controller to initialize.");
        return false;
    }

    return true;
}

// Clear any left over data in the PS/2 controller's data port
// NOTE: We don't use ReadDataFromController because it waits, and we shouldn't wait for data to become available here!
void ClearDataBuffer() {
    while (IO::Serial::inb(STATUS_CMD) & 0x1) {
        IO::Serial::inb(DATA);
    }
}

uint8_t ReadDataFromController(uint8_t port) {
    // Wait until output buffer is full before reading
    while (!(IO::Serial::inb(STATUS_CMD) & 0x1));

    // Return the data
    return IO::Serial::inb(port);
}

void SendDataToController(uint8_t port, uint8_t value) {
    // Wait for the input buffer to be empty before sending the data
    while (IO::Serial::inb(STATUS_CMD) & 0x2);

    // Send the data
    IO::Serial::outb(port, value);
}

uint8_t TestPort(uint8_t portTestCmd) {
    uint8_t testResponse = 0xFF;

    for (uint8_t retries = 3; retries > 0; retries--) {
        SendDataToController(STATUS_CMD, portTestCmd);
        testResponse = ReadDataFromController(DATA);
        if (testResponse != DATA_RESEND) return testResponse;
    }

    return testResponse;
}

uint8_t ResetPort(bool isPort2) {
    for (uint8_t retries = 3; retries > 0; retries--) {
        if (isPort2) SendDataToController(STATUS_CMD, SELECT_PORT_2);
        SendDataToController(DATA, PERIPHERAL_RESET);

        uint8_t response1 = ReadDataFromController(DATA);
        if (response1 == DATA_RESEND) continue;

        uint8_t response2 = ReadDataFromController(DATA);
        if (response2 == DATA_RESEND) continue;

        bool gotACK  = (response1 == 0xFA || response2 == 0xFA);
        bool gotPass = (response1 == 0xAA || response2 == 0xAA);

        if (response1 == 0xFC || response2 == 0xFC) {
            LOG_WARNING("PS/2 port %u8 self test failed!", isPort2 ? 2 : 1);
            ClearDataBuffer();
            return 0xFC;
        }

        if (!gotACK || !gotPass) {
            LOG_WARNING("PS/2 port %u8 reset failed (got 0x%x8, 0x%x8)!", isPort2 ? 2 : 1, response1, response2);
            ClearDataBuffer();
            return 0xFF;
        }
        
        ClearDataBuffer();
        return 0x00;
    }

    LOG_WARNING("PS/2 port %u8 reset failed after 3 retries!", isPort2 ? 2 : 1);
    return 0xFF;
}

void KeyboardHandler() {
    uint8_t scancode = 0;
    asm volatile("inb %1, %0" : "=a"(scancode) : "Nd"(DATA));
    LOG_DEBUG("Scancode received: 0x%x8", scancode);
}

void MouseHandler() {
    LOG_DEBUG("Mouse used!");
}

LOAD_FUNC() {
    // Disable the mouse and keyboard during initialization
    SendDataToController(STATUS_CMD, PORT_1_DISABLE);
    SendDataToController(STATUS_CMD, PORT_2_DISABLE); // This is ignored if the PS/2 controller only has one port

    // Clear any left over data in the PS/2 controller's data port. This will place the controller in a known and stable state
    // NOTE: We don't use ReadDataFromController because it waits, and we shouldn't wait for data to become available here!
    while (IO::Serial::inb(STATUS_CMD) & 0x1) {
        IO::Serial::inb(DATA);
    }

    // Set the controller's config byte
    SendDataToController(STATUS_CMD, READ_CONFIG_BYTE_CMD);
    uint8_t configByte = ReadDataFromController(DATA);
    CLEAR_BIT(configByte, 0); // Disable IRQs for port 1
    CLEAR_BIT(configByte, 6); // Disable translation for port 1
    CLEAR_BIT(configByte, 4); // Enable the clock signal for port 1
    SendDataToController(STATUS_CMD, WRITE_CONFIG_BYTE_CMD);
    SendDataToController(DATA, configByte);

    // Test the controller by sending 0xAA and checking if the response is 0x55
    SendDataToController(STATUS_CMD, CONTROLLER_SELF_TEST);
    uint8_t testResponse = ReadDataFromController(DATA);
    
    // Handle ACKs
    if (testResponse == CONTROLLER_ACK) testResponse = ReadDataFromController(DATA);
    if (testResponse != CONTROLLER_SELF_TEST_PASSED) {
        LOG_ERROR("PS/2 controller self test failed (0x%x8 != 0x%x8)!", testResponse, CONTROLLER_SELF_TEST_PASSED);
        return STATUS::FAILURE;
    }    

    // Check if there are two channels
    SendDataToController(STATUS_CMD, PORT_2_ENABLE);
    SendDataToController(STATUS_CMD, READ_CONFIG_BYTE_CMD);
    configByte = ReadDataFromController(DATA);
    if (!(configByte & (1 << 5))) twoChannels = true;

    // Disable port 2 again and reconfigure the config byte
    SendDataToController(STATUS_CMD, PORT_2_DISABLE);
    SendDataToController(STATUS_CMD, READ_CONFIG_BYTE_CMD);
    configByte = ReadDataFromController(DATA);
    CLEAR_BIT(configByte, 1); // Disable IRQs for port 2
    CLEAR_BIT(configByte, 5); // Enable the clock signal for port 2
    SendDataToController(STATUS_CMD, WRITE_CONFIG_BYTE_CMD);
    SendDataToController(DATA, configByte);

    // Perform an interface test on port 1 and port 2 (if port 2 exists)
    uint8_t port1TestResult = TestPort(PORT_1_TEST);
    uint8_t port2TestResult = 0xFF;

    if (port1TestResult != 0x00) LOG_WARNING("PS/2 port 1 test failed (got 0x%x8 instead of 0x00)!", port1TestResult);

    if (twoChannels) {
        port2TestResult = TestPort(PORT_2_TEST);
        if (port2TestResult != 0x00) LOG_WARNING("PS/2 port 2 test failed (got 0x%x8 instead of 0x00)!", port2TestResult);
    }

    bool port1Works = port1TestResult == 0x00;
    bool port2Works = twoChannels && port2TestResult == 0x00;

    if (!port1Works && !port2Works) {
        LOG_DEBUG("There are no working PS/2 ports available!");
        return STATUS::FAILURE;
    }

    // Enable the peripherals again and enable interrupts in the config byte
    if (port1Works) SendDataToController(STATUS_CMD, PORT_1_ENABLE);    
    if (port2Works) SendDataToController(STATUS_CMD, PORT_2_ENABLE);

    SendDataToController(STATUS_CMD, READ_CONFIG_BYTE_CMD);
    configByte = ReadDataFromController(DATA);
    port1Works ? SET_BIT(configByte, 0) : CLEAR_BIT(configByte, 0);
    port2Works ? SET_BIT(configByte, 1) : CLEAR_BIT(configByte, 1);
    SendDataToController(STATUS_CMD, WRITE_CONFIG_BYTE_CMD);
    SendDataToController(DATA, configByte);

    // Reset the peripherals
    if (port1Works) port1Works = ResetPort(false) == 0x00;
    if (port2Works) port2Works = ResetPort(true) == 0x00;

    // Finally, set up IRQ handlers and unmask IRQs
    if (port1Works) {
        kernel->ArchitectureData->Idt.RegisterIRQHandler(KEYBOARD_IRQ, KeyboardHandler);
        kernel->ArchitectureData->Idt.UnmaskIRQ(KEYBOARD_IRQ);
    }

    if (port2Works) {
        kernel->ArchitectureData->Idt.RegisterIRQHandler(MOUSE_IRQ, MouseHandler);
        kernel->ArchitectureData->Idt.UnmaskIRQ(MOUSE_IRQ);
    }

    LOG_INFO("PS/2 controller initialized in %s-channel mode.", (twoChannels && port1Works && port2Works) ? "dual" : "single");
    return STATUS::SUCCESS;
}