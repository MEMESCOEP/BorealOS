#include "Tracker.h"

Tracker *trackerInstance = nullptr;

STATUS Tracker::RegisterDevice(const HID::InputDevice *device) {
    if (trackerInstance == nullptr) {
        return STATUS::FAILURE;
    }

    if (HasDevice(device->id)) {
        return STATUS::FAILURE; // Device with this id is already registered
    }

    trackerInstance->_devices.Add(*device);
    return STATUS::SUCCESS;
}

STATUS Tracker::UnregisterDevice(uint64_t deviceId) {
    if (trackerInstance == nullptr) {
        return STATUS::FAILURE;
    }

    for (size_t i = 0; i < trackerInstance->_devices.Size(); i++) {
        if (trackerInstance->_devices[i].id == deviceId) {
            trackerInstance->_devices.Remove(i);
            return STATUS::SUCCESS;
        }
    }

    return STATUS::FAILURE; // Device not found
}

bool Tracker::HasDevice(uint64_t deviceId) {
    if (trackerInstance == nullptr) {
        return false;
    }

    for (size_t i = 0; i < trackerInstance->_devices.Size(); i++) {
        if (trackerInstance->_devices[i].id == deviceId) {
            return true;
        }
    }

    return false;
}

STATUS Tracker::GetDeviceInfo(uint64_t deviceId, HID::InputDevice *outDevice) {
    if (trackerInstance == nullptr) {
        return STATUS::FAILURE;
    }

    for (size_t i = 0; i < trackerInstance->_devices.Size(); i++) {
        if (trackerInstance->_devices[i].id == deviceId) {
            *outDevice = trackerInstance->_devices[i];
            return STATUS::SUCCESS;
        }
    }

    return STATUS::FAILURE; // Device not found
}

STATUS Tracker::SubscribeInputEvents(HID::SubscriptionInfo *subscription) {
    if (trackerInstance == nullptr || subscription->callback == nullptr) {
        return STATUS::FAILURE;
    }

    if (subscription->deviceId != 0 && !HasDevice(subscription->deviceId)) {
        return STATUS::FAILURE; // Can't subscribe to events from a device that isn't registered
    }

    subscription->subscriptionId = GenerateSubscriptionId();
    trackerInstance->_eventHandlers.Add(*subscription);

    return STATUS::SUCCESS;
}

STATUS Tracker::UnsubscribeInputEvents(const HID::SubscriptionInfo &subscription) {
    if (trackerInstance == nullptr) {
        return STATUS::FAILURE;
    }

    for (size_t i = 0; i < trackerInstance->_eventHandlers.Size(); i++) {
        if (trackerInstance->_eventHandlers[i].subscriptionId == subscription.subscriptionId) {
            trackerInstance->_eventHandlers.Remove(i);
            return STATUS::SUCCESS;
        }
    }

    return STATUS::FAILURE; // Event handler not found
}

void Tracker::BroadcastInputEvent(const HID::InputEvent *event) {
    if (trackerInstance == nullptr) {
        return;
    }

    for (size_t i = 0; i < trackerInstance->_eventHandlers.Size(); i++) {
        if (trackerInstance->_eventHandlers[i].deviceId == event->deviceId || trackerInstance->_eventHandlers[i].deviceId == 0) { // deviceId of 0 means subscribe to all events regardless of device
            trackerInstance->_eventHandlers[i].callback(event);
        }
    }
}

uint64_t Tracker::GenerateSubscriptionId() {
    static uint64_t nextId = 1; // if there are more than (2 ^ 64) - 1 subscriptions, we have bigger problems than id collisions.
    // I'm not even sure if there are that many devices in the world, let alone that many subscriptions to input events.
    return nextId++;
}

Tracker::Tracker() : _devices(16), _eventHandlers(16) {
    if (trackerInstance != nullptr) {
        PANIC("Tracker instance already exists!");
    }

    _service = HID::HIDService{
        .RegisterDevice = RegisterDevice,
        .UnregisterDevice = UnregisterDevice,
        .HasDevice = HasDevice,
        .GetDeviceInfo = GetDeviceInfo,
        .SubscribeInputEvents = SubscribeInputEvents,
        .UnsubscribeInputEvents = UnsubscribeInputEvents,
        .BroadcastInputEvent = BroadcastInputEvent
    };

    trackerInstance = this;
}

/*
// Usage example of the HID service API from another module (something like PS/2 or USB driver):
void LoadService(){
    auto *hidService = (HID::HIDService*) Core::ServiceManager::GetInstance()->GetService(HID_SERVICE_NAME);

    // Enumerate through whatever devices this service manages
    auto *devices = GetDevicesSomehow();
    for (size_t i = 0; i < deviceCount; i++) {
        hidService->RegisterDevice(&devices[i]);
    }

    // Now when an input event occurs from one of the registered devices, we can broadcast it to the system:
    HID::InputEvent event{};
    event.deviceId = devices[0].id;
    event.type = HID::InputEventType::KeyPress;
    event.keyEvent.keyCode = 42; // Some key code for the pressed key
    hidService->BroadcastInputEvent(&event);
}

// Usage as a consumer of a device event:
void SomeOtherModuleFunction() {
    auto *hidService = (HID::HIDService*) Core::ServiceManager::GetInstance()->GetService(HID_SERVICE_NAME);

    HID::SubscriptionInfo subscription{};
    subscription.deviceId = 1; // The id of the device we want to subscribe to
    subscription.callback = [](const HID::InputEvent* event) {
        // Handle the input event here
    };

    hidService->SubscribeInputEvents(&subscription);
}
*/