#ifndef BOREALOS_NETWORKING_TRACKER_H
#define BOREALOS_NETWORKING_TRACKER_H

#include <Definitions.h>
#include <Utility/List.h>
#include "../Service.h"
#include "ARPTable.h"

class Tracker {
public:
    Tracker();
    [[nodiscard]] Networking::NetworkService *GetService() { return &_service; }

private:
    Utility::List<Networking::NetworkInterface> _interfaces;
    Utility::List<Networking::SubscriptionInfo> _eventHandlers;
    Utility::List<Networking::SubscriptionInfo> _receiveHandlers;
    Utility::List<Networking::ReceiveFilter>    _receiveFilters;
    Networking::NetworkService _service{};
    Networking::ARPTable _arpTable;

    // NIC management
    static STATUS RegisterInterface   (const Networking::NetworkInterface* interface);
    static STATUS UnregisterInterface (uint64_t interfaceID);
    static bool   HasInterface        (uint64_t interfaceID);
    static STATUS GetInterfaceInfo    (uint64_t interfaceID, Networking::NetworkInterface* out);
    static STATUS SetInterfaceState   (uint64_t interfaceID, Networking::InterfaceState state);
    static STATUS EnumerateInterfaces (Utility::List<Networking::NetworkInterface>* interfaceList);
    static STATUS SubscribeEvents     (Networking::SubscriptionInfo* subscription);
    static STATUS UnsubscribeEvents   (const Networking::SubscriptionInfo &subscription);
    static void   BroadcastEvent      (const Networking::NetworkEvent* event);

    // High level TX / RX
    static STATUS SendTo (const Networking::TransmitTarget* target, const uint8_t *data, uint16_t length);
    static STATUS SubscribeReceive (const Networking::ReceiveFilter* filter, Networking::NetworkEventCallback callback, uint64_t* outSubscriptionID);
    static STATUS UnsubscribeReceive (uint64_t subscriptionID);

    // Internal helpers
    static STATUS SendARPRequest (const Networking::IPv4Address &ip);
    static bool MatchesFilter (const Networking::ReceiveFilter &filter, const Networking::NetworkEvent* event);
    static void ProcessARPReply(const Networking::Packet* packet);
    static bool IsARPPacket(const Networking::Packet* packet);

    // Misc
    static uint64_t GenerateSubscriptionID();
};

#endif // BOREALOS_NETWORKING_TRACKER_H