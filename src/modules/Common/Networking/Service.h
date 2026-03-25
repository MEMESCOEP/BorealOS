#ifndef BOREALOS_NETWORKING_SERVICE_H
#define BOREALOS_NETWORKING_SERVICE_H

#define NETWORKING_MODULE_NAME "Networking Service"
#define NETWORKING_MODULE_DESCRIPTION "This is the networking service, which provides an abstract API for WiFi and Ethernet."
#define NETWORKING_MODULE_VERSION VERSION(0,0,1)
#define NETWORKING_MODULE_IMPORTANCE Formats::DriverModule::Importance::Optional
#define NETWORKING_SERVICE_NAME "networking.service"

namespace Networking {
    // TX / RX Data
    struct Packet {
        uint64_t interfaceID;
        uint16_t length;
        uint8_t* data;
    };

    // Interface
    enum class InterfaceType : uint16_t {
        WiFi,
        Ethernet,
        Loopback,
        Virtual
    };

    enum class InterfaceState : uint16_t {
        Down,
        Up,
        NoCarrier
    };

    typedef STATUS (*TransmitFunc)(const Packet *packet);

    struct NetworkInterface {
        uint64_t       ID;
        InterfaceType  type;
        InterfaceState state;
        uint16_t       vendorID;
        uint16_t       productID;
        uint8_t        macAddress[6];
        char           interfaceName[32];
        char           manufacturer[256];
        char           description[256];
        uint32_t       mtu;
        TransmitFunc   txFunc;
    };

    // Events
    enum class NetworkEventType : uint16_t {
        PacketReceived,
        PacketSent,
        InterfaceUp,
        InterfaceDown,
        LinkSpeedChanged
    };

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
        struct NetworkEvent {
            uint64_t         interfaceID;
            NetworkEventType type;
            union {
                struct { Packet   packet;    } packetEvent;
                struct { uint32_t speedMbps; } linkSpeedEvent;
            };
        };
    #pragma GCC diagnostic pop

    // Subscriptions
    typedef void (*NetworkEventCallback)(const NetworkEvent* event);

    struct SubscriptionInfo {
        uint64_t              interfaceID;   // 0 = subscribe to all interfaces
        uint64_t              subscriptionID;
        NetworkEventCallback  netEventCallback;
    };

    // API
    // Register an interface when a device is connected or discovered
    typedef STATUS (*RegisterInterfaceFunc) (const NetworkInterface *interface);

    // Unregister an interface when a device is disconnected
    typedef STATUS (*UnregisterInterfaceFunc) (uint64_t interfaceID);

    // Returns true if an interface with the given ID is currently registered
    typedef bool (*HasInterfaceFunc) (uint64_t interfaceID);

    // Get a copy of the interface's descriptor if it's registered
    typedef STATUS (*GetInterfaceInfoFunc) (uint64_t interfaceID, NetworkInterface* out);

    // Bring an interface up or down. Automatically broadcasts interface up and down events
    typedef STATUS (*SetInterfaceStateFunc) (uint64_t interfaceID, InterfaceState state);

    // Get all registered network interfaces
    typedef STATUS (*EnumerateInterfacesFunc) (Utility::List<NetworkInterface>* interfaceList);

    // Validate and transmit a packet via the interface's registered txFunc
    typedef STATUS (*SendPacketFunc) (const Packet* packet);

    // Subscribe to events from a specific interface (all interfaces if interfaceID is 0)
    typedef STATUS (*SubscribeEventsFunc) (SubscriptionInfo* subscription);

    // Unsubscribe using the subscriptionID filled in by SubscribeEvents
    typedef STATUS (*UnsubscribeEventsFunc) (const SubscriptionInfo &subscription);

    struct NetworkService {
        RegisterInterfaceFunc   RegisterInterface;
        UnregisterInterfaceFunc UnregisterInterface;
        HasInterfaceFunc        HasInterface;
        GetInterfaceInfoFunc    GetInterfaceInfo;
        SetInterfaceStateFunc   SetInterfaceState;
        EnumerateInterfacesFunc EnumerateInterfaces;
        SendPacketFunc          SendPacket;
        SubscribeEventsFunc     SubscribeEvents;
        UnsubscribeEventsFunc   UnsubscribeEvents;
        NetworkEventCallback    BroadcastEvent;
    };
}

#endif //BOREALOS_NETWORKING_SERVICE_H