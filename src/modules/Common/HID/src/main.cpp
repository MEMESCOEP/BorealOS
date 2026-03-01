#include <Kernel.h>
#include <Module.h>
#include <Core/ServiceManager.h>

#include "../Service.h"
#include "Tracker.h"

MODULE(HID_MODULE_NAME, HID_MODULE_DESCRIPTION, HID_MODULE_VERSION, HID_MODULE_IMPORTANCE);

COMPATIBLE_FUNC() {
    // The hid service is compatible with all systems, since it is abstract.
    return true;
}

LOAD_FUNC() {
    auto tracker = new Tracker();
    Core::ServiceManager::GetInstance()->RegisterService(HID_SERVICE_NAME, tracker->GetService());

/*
    // This is left here as a usage example of both creating a service and using it.
    LOG_DEBUG("Testing HID service by registering a test device...");
    auto *service = static_cast<HID::HIDService *>(Core::ServiceManager::GetInstance()->GetService(HID_SERVICE_NAME));
    HID::InputDevice testDevice{
        .id = 1,
        .type = HID::DeviceType::Keyboard,
        .vendorId = 0x1234,
        .productId = 0x5678,
        .name = "Test Keyboard",
        .description = "This is a test keyboard device.",
        .manufacturer = "Test Manufacturer"
    };

    service->RegisterDevice(&testDevice);
    LOG_DEBUG("Registered test device with id %u64, name: %s", testDevice.id, testDevice.name);

    HID::SubscriptionInfo subscription{
        .deviceId = testDevice.id,
        .subscriptionId = 0, // This will be set by the SubscribeInputEvents function
        .callback = [](const HID::InputEvent* event) {
            LOG_DEBUG("Received input event from device %u64, event type: %u64 (local subscription)", event->deviceId, static_cast<int>(event->type));
        },
    };

    HID::SubscriptionInfo globalSubscription{
        .deviceId = 0, // Subscribe to all devices
        .subscriptionId = 0, // This will be set by the SubscribeInputEvents function
        .callback = [](const HID::InputEvent* event) {
            LOG_DEBUG("Received input event from device %u64, event type: %u64 (global subscription)", event->deviceId, static_cast<int>(event->type));
        },
    };

    service->SubscribeInputEvents(&subscription);
    service->SubscribeInputEvents(&globalSubscription);
    LOG_DEBUG("Subscribed to input events from device %u64 with subscription id %u64 (both device local, and global)", subscription.deviceId, subscription.subscriptionId);

    HID::InputEvent testEvent{
        .deviceId = testDevice.id,
        .type = HID::InputEventType::KeyPress,
        .keyEvent = {
            .keyCode = 42
        }
    };

    service->BroadcastInputEvent(&testEvent);
    LOG_DEBUG("Broadcasted test input event from device %u64, event type: %u64", testEvent.deviceId, static_cast<int>(testEvent.type));

    // Expected result in the logs:
    // [DEBUG] Received input event from device 1, event type: 0
    // [DEBUG] Received input event from device 1, event type: 0 (global subscription)

    // Clean up after ourselves by unsubscribing from the events and unregistering the test device.
    service->UnsubscribeInputEvents(subscription);
    service->UnsubscribeInputEvents(globalSubscription);
    service->UnregisterDevice(testDevice.id);
*/

    return STATUS::SUCCESS;
}