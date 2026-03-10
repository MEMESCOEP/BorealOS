#include <Module.h>

#define HAS_SERVICE 0 // Set this to 1 if your module provides a service for other modules to use.
#include "../Service.h"

#include "../../../modules/Common/HID/Service.h"
#include "Core/ServiceManager.h"
#include "Core/Firmware/ACPI.h"
#include "IO/Serial.h"
#include "KernelData.h"
#include "PS2Definitions.h"

RELY_ON(EXTERNAL_MODULE(HID_MODULE_NAME, HID_MODULE_VERSION));
MODULE(PS2_MODULE_NAME, PS2_MODULE_DESCRIPTION, PS2_MODULE_VERSION, PS2_MODULE_IMPORTANCE);

Core::ServiceManager* serviceManager;
HID::HIDService* HIDService;
Kernel<KernelData>* kernel;
uint8_t lastScancode = 0x00;
bool canHandleLockKey = true;
bool twoChannels = false;
bool extended = false;
bool capsLock = false, scrollLock = false, numLock = false;
bool keyboardInitialized = false;
bool mouseInitialized = false;

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

    // We can safely assume that a PS/2 controller exists for ACPI 1.0
    if (fadt->sdt.revision == 0x1) return true;

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

void UpdateKeyboardLEDs() {
    if (keyboardInitialized == false) return;

    SendDataToController(DATA, KEYBOARD_SET_LEDS);
    uint8_t LEDStates = 0;

    // Configure the bits based on the lock states
    scrollLock ? SET_BIT(LEDStates, 0) : CLEAR_BIT(LEDStates, 0);
    numLock    ? SET_BIT(LEDStates, 1) : CLEAR_BIT(LEDStates, 1);
    capsLock   ? SET_BIT(LEDStates, 2) : CLEAR_BIT(LEDStates, 2);

    SendDataToController(DATA, LEDStates);
}

uint8_t SendKBCommandWithResult(uint8_t command) {
    uint8_t result;
    uint8_t retries = 0;

    do {
        SendDataToController(DATA, command);
        LOG_DEBUG("Result: 0x%x8", result);
        result = ReadDataFromController(DATA);
        retries++;
    } while (result == DATA_RESEND && retries < 3);

    // Failed to send after all retries
    if (result == DATA_RESEND)
        return DATA_RESEND;

    // Consume the ACK, then read the real response
    if (result == CONTROLLER_ACK)
        result = ReadDataFromController(DATA);

    return result;
}

void KeyboardHandler() {
    if (keyboardInitialized == false) return;

    // Get the scancode
    uint8_t scancode = IO::Serial::inb(DATA);
    bool keyReleased = lastScancode == KEYBOARD_RELEASE_MODIFIER;

    // Toggle lock statuses and LEDs
    if (keyReleased == false) {
        if (canHandleLockKey == true) {
            if (scancode == SPECIAL_KEY_CAPSLOCK) {
                capsLock = !capsLock;
                canHandleLockKey = false;
                UpdateKeyboardLEDs();
            }
            else if (scancode == SPECIAL_KEY_SCROLLLOCK) {
                scrollLock = !scrollLock;
                canHandleLockKey = false;
                UpdateKeyboardLEDs();
            }
            else if (scancode == SPECIAL_KEY_NUMLOCK) {
                numLock = !numLock;
                canHandleLockKey = false;
                UpdateKeyboardLEDs();
            }
        }
    }
    else {
        canHandleLockKey = true;
    }

    if (scancode == KEYBOARD_EXTENDED_MODIFIER) {
        extended = true;
        lastScancode = scancode;
        return;
    }

    // We need to skip sending an event if the current scancode is 0xF0, as that is just a scancode that tells us wether a key was pressed or released. We check the last scancode to determine press/release
    if (scancode != KEYBOARD_RELEASE_MODIFIER) {
        // Generate and broadcast an input event
        HID::InputEvent* inputEvent = new HID::InputEvent {
            .deviceId = 0x00,
            .type = keyReleased ? HID::InputEventType::KeyRelease : HID::InputEventType::KeyPress,
            .keyEvent {
                .keyCode = extended ? Set2ExtendedKeyCodes[scancode] : Set2KeyCodes[scancode]
            }
        };

        HIDService->BroadcastInputEvent(inputEvent);
        delete inputEvent;

        // Only clear the extended flag if a key is released, this preserves cases like 0xE0/0xF0/xxx where lastScancode would get overwritten
        if (keyReleased) extended = false;
    }

    // Keep track of this scancode for multi-code sequences
    lastScancode = scancode;

    LOG_DEBUG("Scancode: 0x%x8", scancode);
}

void MouseHandler() {
    // TODO: Get mouse IRQs working properly
}

STATUS InitPS2Controller() {
    // Disable the mouse and keyboard during initialization
    LOG_DEBUG("Disabling PS/2 keyboard and mouse...");
    SendDataToController(STATUS_CMD, PORT_1_DISABLE);
    SendDataToController(STATUS_CMD, PORT_2_DISABLE); // This is ignored if the PS/2 controller only has one port

    // Clear any left over data in the PS/2 controller's data port. This will place the controller in a known and stable state
    // NOTE: We don't use ReadDataFromController because it waits, and we shouldn't wait for data to become available here!
    ClearDataBuffer();

    // Set the controller's config byte
    LOG_DEBUG("Modifying PS/2 controller's configuration byte...");
    SendDataToController(STATUS_CMD, READ_CONFIG_BYTE_CMD);
    uint8_t configByte = ReadDataFromController(DATA);
    CLEAR_BIT(configByte, 0); // Disable IRQs for port 1
    CLEAR_BIT(configByte, 6); // Disable translation for port 1
    CLEAR_BIT(configByte, 4); // Enable the clock signal for port 1
    SendDataToController(STATUS_CMD, WRITE_CONFIG_BYTE_CMD);
    SendDataToController(DATA, configByte);

    // Test the controller by sending 0xAA and checking if the response is 0x55
    LOG_DEBUG("Testing PS/2 controller...");
    SendDataToController(STATUS_CMD, CONTROLLER_SELF_TEST);
    uint8_t testResponse = ReadDataFromController(DATA);
    
    // Handle ACKs
    if (testResponse == CONTROLLER_ACK) testResponse = ReadDataFromController(DATA);
    if (testResponse != CONTROLLER_SELF_TEST_PASSED) {
        LOG_ERROR("PS/2 controller self test failed (0x%x8 != 0x%x8)!", testResponse, CONTROLLER_SELF_TEST_PASSED);
        return STATUS::FAILURE;
    }    

    // Check if there are two channels
    LOG_DEBUG("Getting PS/2 channel count...");
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
    LOG_DEBUG("Performing interface tests on %u8 channel(s)...", twoChannels ? 2 : 1);
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
    LOG_DEBUG("Enabling PS/2 keyboard and mouse and their interrupts...");
    if (port1Works) SendDataToController(STATUS_CMD, PORT_1_ENABLE);    
    if (port2Works) SendDataToController(STATUS_CMD, PORT_2_ENABLE);

    SendDataToController(STATUS_CMD, READ_CONFIG_BYTE_CMD);
    configByte = ReadDataFromController(DATA);
    port1Works ? SET_BIT(configByte, 0) : CLEAR_BIT(configByte, 0);
    port2Works ? SET_BIT(configByte, 1) : CLEAR_BIT(configByte, 1);
    SendDataToController(STATUS_CMD, WRITE_CONFIG_BYTE_CMD);
    SendDataToController(DATA, configByte);

    // Reset the peripherals
    LOG_DEBUG("Resetting PS/2 keyboard and mouse...");
    if (port1Works) port1Works = ResetPort(false) == 0x00;
    if (port2Works) port2Works = ResetPort(true) == 0x00;

    // Finally, set up IRQ handlers and unmask IRQs
    LOG_DEBUG("Setting up PS/2 keyboard and mouse IRQ handlers...");
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

uint8_t BuildTypematicByte(uint8_t rate, uint8_t delay) {
    rate  &= 0x1F;  // clamp to 5 bits
    delay &= 0x03;  // clamp to 2 bits
    return (delay << 3) | rate;
}

STATUS InitKeyboard() {
    // We use scancode set 2
    SendDataToController(DATA, KEYBOARD_GET_SELECT_SCANCODE_SET); // Set
    SendDataToController(DATA, 0x2);

    SendDataToController(DATA, KEYBOARD_GET_SELECT_SCANCODE_SET); // Get
    uint8_t currentSet = SendKBCommandWithResult(0x0);

    if (currentSet != 0x02) {
        LOG_ERROR("PS/2 keyboard initialization failed, scancode set 0x%x8 is not set 2 (0x02)!", currentSet);
        return STATUS::FAILURE;
    }

    /*
    // Enable scanning so the keyboard actually sends scancodes
    uint8_t scanningResult = SendKBCommandWithResult(KEYBOARD_ENABLE_SCANNING);
    if (scanningResult != CONTROLLER_ACK) {
        LOG_ERROR("PS/2 keyboard initialization failed, could not enable scanning (response was 0x%x8 instead of 0x%x8)!", scanningResult, CONTROLLER_ACK);
        return STATUS::FAILURE;
    }

    // Finally, set the typematic rate and delay
    SendDataToController(DATA, KEYBOARD_SET_TYPEMATIC_RATE_DELAY);
    SendDataToController(DATA, BuildTypematicByte(0x05, 0x03));
    */

    keyboardInitialized = true;
    return STATUS::SUCCESS;
}

LOAD_FUNC() {
    // Get the service manager instance
    LOG_DEBUG("Getting service manager and HID service instances...");
    serviceManager = Core::ServiceManager::GetInstance();

    if (!serviceManager) {
        LOG_ERROR("Failed to get service manager instance!");
        return STATUS::FAILURE;
    }

    // Get the HID service instance
    HIDService = static_cast<HID::HIDService*>(serviceManager->GetService(HID_SERVICE_NAME));

    if (!HIDService)  {
        LOG_ERROR("Failed to get HID service instance!");
        return STATUS::FAILURE;
    }

    // Initialize the PS/2 controller
    LOG_DEBUG("Initializing PS/2 controller...");
    if (InitPS2Controller() != STATUS::SUCCESS) {
        LOG_ERROR("PS/2 controller initialization failed!");
        return STATUS::FAILURE;
    }
    
    // Initialize the keyboard and mouse
    if (InitKeyboard() != STATUS::SUCCESS) {
        LOG_ERROR("PS/2 keyboard initialization failed!");
        return STATUS::FAILURE;
    }

    // Register the keyboard and mouse as HID devices
    LOG_DEBUG("Registering keyboard and mouse...");
    HID::InputDevice* keyboard = new HID::InputDevice {
        .id = 1,
        .type = HID::DeviceType::Keyboard,
        .vendorId = 0x00,
        .productId = 0x00,
        .name = "Generic PS/2 Keyboard",
        .description = "Generic PS/2 Keyboard",
        .manufacturer = "Unknown"
    };

    HID::InputDevice* mouse = new HID::InputDevice {
        .id = 2,
        .type = HID::DeviceType::Mouse,
        .vendorId = 0x00,
        .productId = 0x00,
        .name = "Generic PS/2 Mouse",
        .description = "Generic PS/2 Mouse",
        .manufacturer = "Unknown"
    };

    HIDService->RegisterDevice(keyboard);
    HIDService->RegisterDevice(mouse);
    while(true);
    return STATUS::SUCCESS;
}