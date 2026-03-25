#ifndef BOREALOS_NETWORKING_TRACKER_H
#define BOREALOS_NETWORKING_TRACKER_H

#include <Definitions.h>
#include <Utility/List.h>
#include "../Service.h"

class Tracker {
public:
    Tracker();
    [[nodiscard]] Networking::NetworkService *GetService() { return &_service; }

private:
    Networking::NetworkService _service{};
    Utility::List<Networking::NetworkInterface>  _interfaces;
    Utility::List<Networking::SubscriptionInfo>  _eventHandlers;

    static STATUS RegisterInterface   (const Networking::NetworkInterface *interface);
    static STATUS UnregisterInterface (uint64_t interfaceID);
    static STATUS GetInterfaceInfo    (uint64_t interfaceID, Networking::NetworkInterface* out);
    static STATUS SetInterfaceState   (uint64_t interfaceID, Networking::InterfaceState state);
    static STATUS EnumerateInterfaces (Utility::List<Networking::NetworkInterface>* interfaceList);
    static STATUS SendPacket          (const Networking::Packet* packet);
    static STATUS SubscribeEvents     (Networking::SubscriptionInfo* subscription);
    static STATUS UnsubscribeEvents   (const Networking::SubscriptionInfo &subscription);
    static bool   HasInterface        (uint64_t interfaceID);
    static void   BroadcastEvent      (const Networking::NetworkEvent* event);

    static uint64_t GenerateSubscriptionID();
};

#endif // BOREALOS_NETWORKING_TRACKER_H