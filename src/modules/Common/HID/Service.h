#ifndef BOREALOS_SERVICE_H
#define BOREALOS_SERVICE_H

#define HID_MODULE_NAME "HID Service"
#define HID_MODULE_DESCRIPTION "This is the Human Interface Device (HID) service, which provides an abstract API for input devices like keyboards, mice, etc."
#define HID_MODULE_VERSION VERSION(0,0,1)
#define HID_MODULE_IMPORTANCE Formats::DriverModule::Importance::Required
#define HID_SERVICE_NAME "hid.service"

#include "Keycodes.h"

namespace HID {
    enum class DeviceType : uint16_t {
        Keyboard,
        Mouse,
        Gamepad
    };

    struct InputDevice {
        uint64_t id; // unique id for the device
        DeviceType type;
        uint16_t vendorId;
        uint16_t productId;

        char name[64]; // Name of device
        char description[256]; // Description of device, for example "Logitech G502 Hero Gaming Mouse"
        char manufacturer[64]; // Manufacturer of the device, for example "Logitech"
    };

    enum class InputEventType : uint16_t {
        KeyPress,
        KeyRelease,
        MouseMove,
        MouseButtonPress,
        MouseButtonRelease,
        GamepadButtonPress,
        GamepadButtonRelease,
        GamepadAxisMove
    };

    enum class MouseButton : uint16_t {
        None,
        Left,
        Right,
        Middle,
        Button4,
        Button5
    };

    enum class GamepadAxis : uint16_t {
        LeftStickX,
        LeftStickY,
        RightStickX,
        RightStickY,
        LeftTrigger,
        RightTrigger
    };

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    struct InputEvent {
        const uint64_t deviceId;
        InputEventType type;
        union {
            struct {
                KeyCode keyCode; // for keyboard events, using USB HID usage codes
            } keyEvent;
            struct {
                int16_t deltaX; // for mouse movement
                int16_t deltaY;
                int16_t deltaVerticalWheel;
                int16_t deltaHorizontalWheel;
                MouseButton button; // for mouse button events
            } mouseEvent;
            struct {
                uint16_t button; // for gamepad button events
                GamepadAxis axis; // for gamepad axis events
                int16_t value; // for gamepad axis events. [-32768, 32767], where 0 is the neutral position.
            } gamepadEvent;
        };
    };
#pragma GCC diagnostic pop // The above union with anonymous structs is technically non-standard.

    // Service API function types.

    /// Register a device with the HID service.
    typedef STATUS (*RegisterDeviceFunc)(const InputDevice* device);
    /// Unregister a device from the HID service, for example when it is disconnected.
    typedef STATUS (*UnregisterDeviceFunc)(uint64_t deviceId);

    /// When a device event occurs, you need to call this to announce the event to the system, and any module that has subscribed to events from this device will receive it.
    typedef void (*InputEventCallback)(const InputEvent* event);

    struct SubscriptionInfo {
        uint64_t deviceId;
        uint64_t subscriptionId; // Unique id for the subscription, set by SubscribeInputEvents, used to unsubscribe.
        InputEventCallback callback;
    };

    /// Check if a device with the given id is currently registered with the HID service.
    typedef bool (*HasDeviceFunc)(uint64_t deviceId);
    /// Get information about a registered device. Returns an error if the device is not registered.
    typedef STATUS (*GetDeviceInfoFunc)(uint64_t deviceId, InputDevice* outDevice);

    /// Subscribe to input events from a device. The callback will be called whenever an event from this device occurs. Returns an error if the device is not registered.
    /// On success, fills subscriptionId in the SubscriptionInfo struct, which can be used to unsubscribe from the events later.
    typedef STATUS (*SubscribeInputEventsFunc)(SubscriptionInfo *subscription);
    /// Unsubscribe from input events from a device. Returns an error if the device is not registered.
    typedef STATUS (*UnsubscribeInputEventsFunc)(const SubscriptionInfo &subscription);

    struct HIDService {
        RegisterDeviceFunc RegisterDevice; // Use this to register a device with the HID service, for example when it is connected.
        UnregisterDeviceFunc UnregisterDevice; // Use this to unregister a device from the HID service, for example when it is disconnected.
        HasDeviceFunc HasDevice; // Use this to check if a device with a given id is currently registered with the HID service.
        GetDeviceInfoFunc GetDeviceInfo; // Use this to get information about a registered device. Returns an error if the device is not registered.
        SubscribeInputEventsFunc SubscribeInputEvents; // Use this to subscribe to input events from a device.
        UnsubscribeInputEventsFunc UnsubscribeInputEvents; // Use this to unsubscribe from input events from a device.
        InputEventCallback BroadcastInputEvent; // Use this to broadcast an input event to all subscribers.
    };
}
#endif //BOREALOS_SERVICE_H