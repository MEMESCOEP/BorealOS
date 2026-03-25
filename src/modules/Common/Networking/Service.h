#ifndef BOREALOS_NETWORKING_SERVICE_H
#define BOREALOS_NETWORKING_SERVICE_H

#define NETWORKING_MODULE_NAME "Networking Service"
#define NETWORKING_MODULE_DESCRIPTION "This is the networking service, which provides an abstract API for WiFi and Ethernet."
#define NETWORKING_MODULE_VERSION VERSION(0,0,1)
#define NETWORKING_MODULE_IMPORTANCE Formats::DriverModule::Importance::Optional
#define NETWORKING_SERVICE_NAME "networking.service"

namespace Networking {
    // Addressing, targetting, and data
    enum class TransmitTargetType : uint8_t {
        Unicast,   // One specific IP or MAC
        Multicast, // A multicast group address that multiple devices listen on
        Broadcast  // All devices on the network
    };

    enum class ReceiveFilterType : uint8_t {
        All,       // Everything
        Unicast,   // Only packets addressed to this machine
        Multicast, // Only packets for a specific multicast group
        Broadcast, // Only broadcast packets
        FromIP,    // Only packets from a specific IP
        FromMAC,   // Only packets from a specific MAC
    };

    struct MACAddress {
        uint8_t octets[6];
    };

    struct IPv4Address {
        uint8_t octets[4];
    };

    struct IPv6Address {
        uint8_t octets[16];
    };

    union IPAddress {
        IPv4Address v4;
        IPv6Address v6;
    };

    enum class IPVersion : uint8_t {
        IPv4,
        IPv6
    };

    struct NetworkAddress {
        IPVersion ipVersion;
        IPAddress IP;
        MACAddress mac;
    };

    struct TransmitTarget {
        TransmitTargetType type;
        NetworkAddress address; // This is ignored for Broadcast
    };

    struct ReceiveFilter {
        ReceiveFilterType type;
        NetworkAddress address; // This is only used for FromIP / FromMAC / Multicast
    };

    struct Packet {
        uint64_t interfaceID;
        uint16_t length;
        uint8_t* data;
        MACAddress destMAC;
        MACAddress srcMAC;
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
        MACAddress     mac;
        uint16_t       vendorID;
        uint16_t       productID;
        char           interfaceName[32];
        char           manufacturer[256];
        char           description[256];
        uint32_t       mtu;
        IPVersion      ipVersion;
        IPAddress      ip;
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
            uint64_t interfaceID;
            NetworkEventType type;
            union {
                struct { Packet packet; } packetEvent;
                struct { uint32_t speedMbps; } linkSpeedEvent;
            };
        };
    #pragma GCC diagnostic pop

    // Subscriptions
    typedef void (*NetworkEventCallback)(const NetworkEvent* event);

    struct SubscriptionInfo {
        uint64_t interfaceID; // 0 = subscribe to all interfaces
        uint64_t subscriptionID;
        NetworkEventCallback  netEventCallback;
    };

    // NIC management API
    // Register an interface when a device is connected or discovered
    typedef STATUS (*RegisterInterfaceFunc)(const NetworkInterface* interface);

    // Unregister an interface when a device is disconnected
    typedef STATUS (*UnregisterInterfaceFunc)(uint64_t interfaceID);

    // Returns true if an interface with the given ID is currently registered
    typedef bool (*HasInterfaceFunc)(uint64_t interfaceID);

    // Get a copy of the interface's descriptor if it's registered
    typedef STATUS (*GetInterfaceInfoFunc)(uint64_t interfaceID, NetworkInterface* out);

    // Bring an interface up or down. Automatically broadcasts interface up and down events
    typedef STATUS (*SetInterfaceStateFunc)(uint64_t interfaceID, InterfaceState state);

    // Get all registered network interfaces
    typedef STATUS (*EnumerateInterfacesFunc)(Utility::List<NetworkInterface>* interfaceList);

    // Subscribe to events from a specific interface (all interfaces if interfaceID is 0)
    typedef STATUS (*SubscribeEventsFunc)(SubscriptionInfo* subscription);

    // Unsubscribe using the subscriptionID filled in by SubscribeEvents
    typedef STATUS (*UnsubscribeEventsFunc)(const SubscriptionInfo &subscription);

    // High level TX / RX API
    // Send data to a specific target. The service resolves MACs via ARP internally if only an IP is provided
    typedef STATUS (*SendToFunc)(
        const TransmitTarget *target,
        const uint8_t        *data,
        uint16_t              length
    );

    // Subscribe to incoming packets matching a filter. The callback fires for every matching packet received on any up interface
    typedef STATUS (*SubscribeReceiveFunc)(
        const ReceiveFilter* filter,
        NetworkEventCallback netEventCallback,
        uint64_t* outSubscriptionID
    );

    // Unsubscribe a receive handler by the ID returned from SubscribeReceive
    typedef STATUS (*UnsubscribeReceiveFunc)(uint64_t subscriptionID);

    struct NetworkService {
        // NIC management
        RegisterInterfaceFunc   RegisterInterface;
        UnregisterInterfaceFunc UnregisterInterface;
        HasInterfaceFunc        HasInterface;
        GetInterfaceInfoFunc    GetInterfaceInfo;
        SetInterfaceStateFunc   SetInterfaceState;
        EnumerateInterfacesFunc EnumerateInterfaces;
        SubscribeEventsFunc     SubscribeEvents;
        UnsubscribeEventsFunc   UnsubscribeEvents;
        NetworkEventCallback    BroadcastEvent;

        // High level TX / RX
        SendToFunc             SendTo;
        SubscribeReceiveFunc   SubscribeReceive;
        UnsubscribeReceiveFunc UnsubscribeReceive;
    };
}

#endif //BOREALOS_NETWORKING_SERVICE_H