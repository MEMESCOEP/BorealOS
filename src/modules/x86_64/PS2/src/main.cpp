#include <Module.h>

#define HAS_SERVICE 0 // Set this to 1 if your module provides a service for other modules to use.
#include "../Service.h"

#include "../../../modules/Common/HIDService/Service.h"
#include "Core/ServiceManager.h"
#include "Core/Firmware/ACPI.h"
#include "PS2Definitions.h"
#include "KernelData.h"
#include "IO/Serial.h"

RELY_ON(EXTERNAL_MODULE(HID_MODULE_NAME, HID_MODULE_VERSION));
MODULE(PS2_MODULE_NAME, PS2_MODULE_DESCRIPTION, PS2_MODULE_VERSION, PS2_MODULE_IMPORTANCE);

Core::ServiceManager* serviceManager;
HID::HIDService* HIDService;
Kernel<KernelData>* kernel;
uint8_t lastScancode = 0x00;
uint8_t mouseID = 0x00;
bool canHandleLockKey = true;
bool twoChannels = false;
bool extended = false;
bool prevMouseLeft = false, prevMouseRight = false, prevMouseMiddle = false, prevMouseBtn4 = false, prevMouseBtn5 = false;
bool capsLock = false, scrollLock = false, numLock = false;
bool port1Works = false, port2Works = false;
bool keyboardInitialized = false;
bool mouseInitialized = false;

// Clear any left over data in the PS/2 controller's data port
// NOTE: We don't use ReadDataFromController because it waits, and we shouldn't wait for data to become available here!
void ClearDataBuffer() {
    while (IO::Serial::inb(STATUS_CMD) & 0x1) {
        IO::Serial::inb(DATA);
    }
}

void SendDataToController(uint8_t port, uint8_t value) {
    // Wait for the input buffer to be empty before sending the data
    while (IO::Serial::inb(STATUS_CMD) & 0x2);

    // Send the data
    IO::Serial::outb(port, value);
}

uint8_t ReadDataFromController(uint8_t port) {
    // Wait until output buffer is full before reading
    while (!(IO::Serial::inb(STATUS_CMD) & 0x1));

    // Return the data
    return IO::Serial::inb(port);
}

uint8_t SendKBCommandWithResult(uint8_t command, bool responseIsACK = false) {
    uint8_t result;
    uint8_t retries = 0;

    do {
        SendDataToController(DATA, command);
        result = ReadDataFromController(DATA);
        retries++;
    } while (result == DATA_RESEND && retries < 3);

    // Failed to send after all retries
    if (result == DATA_RESEND)
        return DATA_RESEND;

    // Consume the ACK, then read the real response
    if (responseIsACK == false && result == CONTROLLER_ACK) {
        result = ReadDataFromController(DATA);
    }

    return result;
}

uint8_t SendMouseCommandWithResult(uint8_t command, bool responseIsACK = false) {
    uint8_t result;
    uint8_t retries = 0;

    do {
        SendDataToController(STATUS_CMD, CONTROLLER_ADDRESS_MOUSE);
        SendDataToController(DATA, command);
        result = ReadDataFromController(DATA);
        retries++;
    } while (result == DATA_RESEND && retries < 3);

    // Failed to send after all retries
    if (result == DATA_RESEND)
        return DATA_RESEND;

    // Consume the ACK, then read the real response
    if (responseIsACK == false && result == CONTROLLER_ACK) {
        result = ReadDataFromController(DATA);
    }

    return result;
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
    // Mice send 3 bytes on reset (FA AA 00), keyboards send 2 (FA AA)
    // NOTE: QEMU does not send the 0x00 device ID byte, so we only read 2 bytes for both ports
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
            continue;
        }

        ClearDataBuffer();
        return 0x00;
    }

    LOG_WARNING("PS/2 port %u8 reset failed after 3 retries!", isPort2 ? 2 : 1);
    return 0xFF;
}

uint8_t BuildTypematicByte(uint8_t rate, uint8_t delay) {
    rate  &= 0x1F;  // Clamp to 5 bits
    delay &= 0x03;  // Clamp to 2 bits
    return (delay << 3) | rate;
}

void UpdateKeyboardLEDs() {
    if (keyboardInitialized == false) return;

    uint8_t LEDStates = 0;
    scrollLock ? SET_BIT(LEDStates, 0) : CLEAR_BIT(LEDStates, 0);
    numLock    ? SET_BIT(LEDStates, 1) : CLEAR_BIT(LEDStates, 1);
    capsLock   ? SET_BIT(LEDStates, 2) : CLEAR_BIT(LEDStates, 2);

    uint8_t cmdResult = SendKBCommandWithResult(KEYBOARD_SET_LEDS, true);
    if (cmdResult != CONTROLLER_ACK) {
        LOG_WARNING("Failed to send SET_LEDS command to PS/2 keyboard(response was 0x%x8 instead of 0x%x8)!", cmdResult, CONTROLLER_ACK);
        return;
    }

    uint8_t LEDResult = SendKBCommandWithResult(LEDStates, true);
    if (LEDResult != CONTROLLER_ACK) {
        LOG_WARNING("Failed to update PS/2 keyboard LEDs (response was 0x%x8 instead of 0x%x8)!", LEDResult, CONTROLLER_ACK);
    }
}

void KeyboardHandler() {
    if (keyboardInitialized == false) return;

    // Get the scancode
    uint8_t scancode = ReadDataFromController(DATA);
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
            .deviceId = 1,
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
}

void MouseHandler() {
    if (mouseInitialized == false) return;

    static uint8_t packetBuffer[4] = {0};
    static uint8_t packetIndex = 0;

    packetBuffer[packetIndex++] = ReadDataFromController(DATA);

    // If we're reading the first byte but bit 3 isn't set, it's not a valid packet start
    if (packetIndex == 1 && !(packetBuffer[0] & (1 << 3))) {
        packetIndex = 0;
        return;
    }

    // Wait until we have all the bytes we need
    uint8_t expectedPackets = mouseID != 0x00 ? 4 : 3;
    if (packetIndex < expectedPackets) return;
    packetIndex = 0;

    uint8_t packet1 = packetBuffer[0];
    uint8_t packet2 = packetBuffer[1];
    uint8_t packet3 = packetBuffer[2];
    uint8_t packet4 = mouseID != 0x00 ? packetBuffer[3] : 0x00;

    // PS/2 mice send "packets", which are 3 bytes and an optional 4th byte (Z-mvmt, or Z-movement values are added to reconstruct the original value):
    //  Packet 1: Y overflow | X overflow | Y sign | X sign | Always 1 | Middle btn down | Right btn down | Left btn down
    //  Packet 2: X movement
    //  Packet 3: Y movement
    //  Packet 4
    //      Wheel only:      Z "scroll" movement
    //      Wheel + buttons: Mostly 0 | Mostly 0 | 5th btn down | 4th btn down | Z-mvmt 3 | Z-mvmt 2 | Z-mvmt 1 | z-mvmt 0
    bool leftBtnDown   = packet1 & (1 << 0);
    bool rightBtnDown  = packet1 & (1 << 1);
    bool middleBtnDown = packet1 & (1 << 2);
    bool btn4Down      = (mouseID == 0x04) && (packet4 & (1 << 4));
    bool btn5Down      = (mouseID == 0x04) && (packet4 & (1 << 5));
    bool xNegative = packet1 & (1 << 4);
    bool yNegative = packet1 & (1 << 5);

    // Generate and broadcast input events
    // Movement
    HID::InputEvent* moveEvent = new HID::InputEvent {
        .deviceId = 2,
        .type = HID::InputEventType::MouseMove,
        .mouseMoveEvent = {
            .deltaX = xNegative ? (int16_t)(packet2 - 256) : (int16_t)packet2,
            .deltaY = yNegative ? (int16_t)(packet3 - 256) : (int16_t)packet3,
        }
    };

    // Some mice broadcast "resting" state events when no movement occurs
    if (moveEvent->mouseMoveEvent.deltaX != 0 || moveEvent->mouseMoveEvent.deltaY != 0) HIDService->BroadcastInputEvent(moveEvent);
    delete moveEvent;

    // Scrolling
    HID::InputEvent* scrollEvent = new HID::InputEvent {
        .deviceId = 2,
        .type = HID::InputEventType::MouseMove,
        .mouseScrollEvent = {
            .deltaVerticalWheel = (int16_t)((mouseID >= 0x03) ?
                ((packet4 & 0x08) ? (int16_t)((packet4 & 0x0F) - 16) : (int16_t)(packet4 & 0x0F)) : 0),
            .deltaHorizontalWheel = 0
        }
    };

    // Some mice broadcast "resting" state events when no scrolling stops
    if (scrollEvent->mouseScrollEvent.deltaVerticalWheel != 0 || scrollEvent->mouseScrollEvent.deltaHorizontalWheel != 0) HIDService->BroadcastInputEvent(scrollEvent);
    delete scrollEvent;

    // Buttons
    auto sendButtonEvent = [&](bool prevState, bool currrentState, HID::MouseButton button) {
        if (prevState == currrentState) return;
        HID::InputEvent* buttonEvent = new HID::InputEvent {
            .deviceId = 2,
            .type = currrentState ? HID::InputEventType::MouseButtonPress : HID::InputEventType::MouseButtonRelease,
            .mouseButtonEvent = {
                .button = button,
            }
        };

        HIDService->BroadcastInputEvent(buttonEvent);
        delete buttonEvent;
    };

    sendButtonEvent(prevMouseLeft,   leftBtnDown,   HID::MouseButton::Left);
    sendButtonEvent(prevMouseRight,  rightBtnDown,  HID::MouseButton::Right);
    sendButtonEvent(prevMouseMiddle, middleBtnDown, HID::MouseButton::Middle);
    sendButtonEvent(prevMouseBtn4,   btn4Down,      HID::MouseButton::Button4);
    sendButtonEvent(prevMouseBtn5,   btn5Down,      HID::MouseButton::Button5);

    prevMouseLeft   = leftBtnDown;
    prevMouseRight  = rightBtnDown;
    prevMouseMiddle = middleBtnDown;
    prevMouseBtn4   = btn4Down;
    prevMouseBtn5   = btn5Down;
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

    port1Works = port1TestResult == 0x00;
    port2Works = twoChannels && port2TestResult == 0x00;

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

    LOG_INFO("PS/2 controller initialized in %s-channel mode.", (twoChannels && port1Works && port2Works) ? "dual" : "single");
    return STATUS::SUCCESS;
}

STATUS InitKeyboard() {
    if (!port1Works) {
        LOG_ERROR("Port 1 does not work, the keyboard IRQ cannot be handled!");
        return STATUS::FAILURE;
    }

    // Clear the data buffer to get rid of stuck data
    ClearDataBuffer();

    // We use scancode set 2
    LOG_DEBUG("Setting scancode set to 2...");
    SendKBCommandWithResult(KEYBOARD_GET_SELECT_SCANCODE_SET, true); // Set the scancode set
    SendKBCommandWithResult(0x2, true);

    LOG_DEBUG("Verifying scancode set...");
    SendKBCommandWithResult(KEYBOARD_GET_SELECT_SCANCODE_SET, true); // Get the scancode set
    uint8_t currentSet = SendKBCommandWithResult(0x0);

    if (currentSet == CONTROLLER_ACK) {
        LOG_WARNING("PS/2 keyboard didn't send the scancode set, we will have to assume that it's set 2!");;
    }
    else if (currentSet != 0x02) {
        LOG_ERROR("PS/2 keyboard initialization failed, scancode set 0x%x8 is not set 2 (0x02)!", currentSet);
        return STATUS::FAILURE;
    }

    // Enable scanning so the keyboard actually sends scancodes
    LOG_DEBUG("Enabling scanning");
    uint8_t scanningResult = SendKBCommandWithResult(KEYBOARD_ENABLE_SCANNING, true);
    if (scanningResult != CONTROLLER_ACK) {
        LOG_ERROR("PS/2 keyboard initialization failed, could not enable scanning (response was 0x%x8 instead of 0x%x8)!", scanningResult, CONTROLLER_ACK);
        return STATUS::FAILURE;
    }

    // Finally, set the typematic rate and delay
    // NOTE: This is non-fatal, the keyboard will just use default typematic settings
    SendKBCommandWithResult(KEYBOARD_SET_TYPEMATIC_RATE_DELAY, true);
    uint8_t typematicResult = SendKBCommandWithResult(BuildTypematicByte(0x05, 0x03), true);
    if (typematicResult != CONTROLLER_ACK) {
        LOG_WARNING("PS/2 keyboard typematic configuration failed (response was 0x%x8 instead of 0x%x8)!", typematicResult, CONTROLLER_ACK);
    }

    // Finally, set up the IRQ handler and unmask IRQ 1
    LOG_DEBUG("Setting up PS/2 keyboard IRQ handler...");
    kernel->ArchitectureData->Idt.RegisterIRQHandler(KEYBOARD_IRQ, KeyboardHandler);
    kernel->ArchitectureData->Idt.UnmaskIRQ(KEYBOARD_IRQ);

    keyboardInitialized = true;
    return STATUS::SUCCESS;
}

// NOTE: Apparently QEMU's PS/2 mouse emulation is kinda ass. The mouse refuses to send IRQs if it's moved during the
//  init and is not yet fully initialized. Issuing the movement counter reset command seems to fix this *mostly*, maybe the values are corrupted?
STATUS InitMouse() {
    if (!port2Works) {
        LOG_ERROR("Port 2 does not work, the mouse IRQ cannot be handled!");
        return STATUS::FAILURE;
    }

    // Flush any movement packets that may have accumulated during initialization
    ClearDataBuffer();

    // Set the resolution to 4 counts per millimeter
    LOG_DEBUG("Setting PS/2 mouse resolution...");
    uint8_t resolutionResult = SendMouseCommandWithResult(MOUSE_SET_RESOLUTION, true);
    if (resolutionResult != CONTROLLER_ACK) LOG_WARNING("Failed to issue PS/2 mouse resolution command (response was 0x%x8 instead of 0x%x8)!", resolutionResult, CONTROLLER_ACK);
    else {
        uint8_t resolutionValueResult = SendMouseCommandWithResult(0x02, true);
        if (resolutionValueResult != CONTROLLER_ACK) LOG_WARNING("Failed to set PS/2 mouse resolution value (response was 0x%x8 instead of 0x%x8)!", resolutionValueResult, CONTROLLER_ACK);
    }

    // Reset the mouse's movement counters
    // NOTE: This seems to have fixed QEMU's dogshit PS/2 emulation. I guess the movement counters get corrupted if the mouse is moved during its init?
    // NOTE: This mist be done before IntelliMouse configuration, as it might reset the mouse to a 3-packet state
    LOG_DEBUG("Resetting PS/2 mouse movement counters...");
    ClearDataBuffer();
    uint8_t defaultsResult = SendMouseCommandWithResult(MOUSE_RESET_MOVEMENT_COUNTERS, true);
    if (defaultsResult != CONTROLLER_ACK) LOG_WARNING("PS/2 mouse set defaults command failed (response was 0x%x8 instead of 0x%x8)!", defaultsResult, CONTROLLER_ACK);

    // Attempt to enable scroll wheel support via the IntelliMouse magic sequence (set sample rate 200, 100, 80)
    // If the mouse supports it, it will switch to a 4-byte packet format with a scroll wheel byte
    // NOTE: The mouse may not support this, that's fine
    LOG_DEBUG("Attempting to enable PS/2 mouse scroll wheel...");
    SendMouseCommandWithResult(MOUSE_SET_SAMPLE_RATE, true); SendMouseCommandWithResult(200, true);
    SendMouseCommandWithResult(MOUSE_SET_SAMPLE_RATE, true); SendMouseCommandWithResult(100, true);
    SendMouseCommandWithResult(MOUSE_SET_SAMPLE_RATE, true); SendMouseCommandWithResult(80,  true);

    // Check if the mouse accepted the IntelliMouse sequence by reading its device ID (should be 0x03 if it did)
    mouseID = SendMouseCommandWithResult(MOUSE_GET_ID);
    if (mouseID == 0x03) {
        LOG_INFO("PS/2 mouse supports scroll wheel.");

        // Attempt to enable extra buttons via a second magic sequence
        LOG_DEBUG("Attempting to enable extra buttons...");
        SendMouseCommandWithResult(MOUSE_SET_SAMPLE_RATE, true); SendMouseCommandWithResult(200, true);
        SendMouseCommandWithResult(MOUSE_SET_SAMPLE_RATE, true); SendMouseCommandWithResult(200, true);
        SendMouseCommandWithResult(MOUSE_SET_SAMPLE_RATE, true); SendMouseCommandWithResult(80,  true);

        // Check if the mouse accepted the explorer sequence by reading its device ID (should be 0x04 if it did)
        mouseID = SendMouseCommandWithResult(MOUSE_GET_ID);
        if (mouseID == 0x04) {
            LOG_INFO("PS/2 mouse supports extra buttons.");
        }
    }
    else LOG_WARNING("PS/2 mouse does not have a scroll wheel.");

    // Set the sample rate to 100 samples per second
    LOG_DEBUG("Setting PS/2 mouse sample rate...");
    uint8_t sampleRateResult = SendMouseCommandWithResult(MOUSE_SET_SAMPLE_RATE, true);
    if (sampleRateResult != CONTROLLER_ACK) LOG_WARNING("PS/2 mouse sample rate command failed (response was 0x%x8 instead of 0x%x8)!", sampleRateResult, CONTROLLER_ACK);
    else {
        uint8_t sampleRateValueResult = SendMouseCommandWithResult(0x64, true);
        if (sampleRateValueResult != CONTROLLER_ACK) LOG_WARNING("PS/2 mouse sample rate value failed (response was 0x%x8 instead of 0x%x8)!", sampleRateValueResult, CONTROLLER_ACK);
    }

    // Enable packet transmission
    LOG_DEBUG("Enabling packet transmission for PS/2 mouse...");
    uint8_t packetTXResult = SendMouseCommandWithResult(MOUSE_ENABLE_PACKET_TRANSMISSION, true);
    if (packetTXResult != CONTROLLER_ACK) {
        LOG_ERROR("PS/2 mouse initialization failed, could not enable packet transmission (response was 0x%x8 instead of 0x%x8)!", packetTXResult, CONTROLLER_ACK);
        return STATUS::FAILURE;
    }

    // Finally, set up the IRQ handler and unmask IRQ 12
    LOG_DEBUG("Setting up PS/2 mouse IRQ handler...");
    kernel->ArchitectureData->Idt.RegisterIRQHandler(MOUSE_IRQ, MouseHandler);
    kernel->ArchitectureData->Idt.UnmaskIRQ(MOUSE_IRQ);

    mouseInitialized = true;
    return STATUS::SUCCESS;
}

COMPATIBLE_FUNC() {
    kernel = Kernel<KernelData>::GetInstance();
    if (!kernel) {
        LOG_ERROR("Failed to get kernel instance!");
        return false;
    }

    // ACPI isn't supported on this system, assume PS/2 exists
    if (kernel->ArchitectureData->Acpi.ACPISupported() == false) return true;

    // Assume PS/2 exists if the FADT table wasn't found
    void* fadtPtr = kernel->ArchitectureData->Acpi.GetTable("FADT");
    if (!fadtPtr) return true;
    Core::Firmware::ACPI::FADT* fadt = (Core::Firmware::ACPI::FADT*)fadtPtr;

    // ACPI 1.0 is old enough that we *have* to assume PS/2 exists, as BootArchitectureFlags is only defined in ACPI 2.0+
    if (fadt->sdt.revision == 0x1) return true;

    // At this point, we have to rely on the firmware to tell us if the PS/2 controller exists, as any attempt to access it can cause crashes and hangs on some systems (e.g. x86 Macs)
    // NOTE: A lot of firmware implementations are pretty dogshit and/or inconsistent, they might not set bit 1 correctly (I'm looking at you, Dell and HP!)
    if (!(fadt->BootArchitectureFlags & (1 << 1))) {
        LOG_WARNING("There is no PS/2 controller to initialize on this system. Some firmware implementations don't populate BIT 1 of the boot arch flags if USB legacy emulation is enabled.");
        return false;
    }

    return true;
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
    if (InitKeyboard() == STATUS::SUCCESS) {
        LOG_DEBUG("Registering PS/2 keyboard as HID device...");
        HID::InputDevice* keyboard = new HID::InputDevice {
            .id = 1,
            .type = HID::DeviceType::Keyboard,
            .vendorId = 0x00,
            .productId = 0x00,
            .name = "Generic PS/2 Keyboard",
            .description = "Generic PS/2 Keyboard",
            .manufacturer = "Unknown"
        };

        HIDService->RegisterDevice(keyboard);
    }
    else {
        LOG_WARNING("PS/2 keyboard initialization failed!");
    }

    if (InitMouse() == STATUS::SUCCESS) {
        LOG_DEBUG("Registering PS/2 mouse as HID device...");
        HID::InputDevice* mouse = new HID::InputDevice {
            .id = 2,
            .type = HID::DeviceType::Mouse,
            .vendorId = 0x00,
            .productId = 0x00,
            .name = "Generic PS/2 Mouse",
            .description = "Generic PS/2 Mouse",
            .manufacturer = "Unknown"
        };

        HIDService->RegisterDevice(mouse);
    }
    else {
        LOG_WARNING("PS/2 Mouse initialization failed!");
    }

    return STATUS::SUCCESS;
}