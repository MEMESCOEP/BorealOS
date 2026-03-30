#ifndef BOREALOS_TRACKER_H
#define BOREALOS_TRACKER_H

#include <Definitions.h>
#include <Utility/List.h>

#include "../Service.h"

class Tracker {
public:
    Tracker();

    [[nodiscard]] HID::HIDService* GetService() { return &_service; }
private:
    HID::HIDService _service{};

    Utility::List<HID::InputDevice> _devices;
    Utility::List<HID::SubscriptionInfo> _eventHandlers;

    static STATUS RegisterDevice(const HID::InputDevice* device);
    static STATUS UnregisterDevice(uint64_t deviceId);
    static bool HasDevice(uint64_t deviceId);
    static STATUS GetDeviceInfo(uint64_t deviceId, HID::InputDevice* outDevice);
    static STATUS SubscribeInputEvents(HID::SubscriptionInfo *subscription);
    static STATUS UnsubscribeInputEvents(const HID::SubscriptionInfo &subscription);
    static void BroadcastInputEvent(const HID::InputEvent* event);
    static uint64_t GenerateSubscriptionId();
};


#endif //BOREALOS_TRACKER_H