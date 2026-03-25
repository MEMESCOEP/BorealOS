#include "Tracker.h"
#include <Kernel.h>

static Tracker *s_instance = nullptr;

Tracker::Tracker() : _service{
    .RegisterInterface   = RegisterInterface,
    .UnregisterInterface = UnregisterInterface,
    .HasInterface        = HasInterface,
    .GetInterfaceInfo    = GetInterfaceInfo,
    .SetInterfaceState   = SetInterfaceState,
    .EnumerateInterfaces = EnumerateInterfaces,
    .SubscribeEvents     = SubscribeEvents,
    .UnsubscribeEvents   = UnsubscribeEvents,
    .BroadcastEvent      = BroadcastEvent,
    .SendTo              = SendTo,
    .SubscribeReceive    = SubscribeReceive,
    .UnsubscribeReceive  = UnsubscribeReceive,
} {
    s_instance = this;
}

// Internal helpers
bool Tracker::IsARPPacket(const Networking::Packet* packet) {
    // An Ethernet frame has at least 14 bytes: dest MAC (6), src MAC (6), EtherType (2)
    if (packet->length < 14) return false;
    
    // Check EtherType: 0x0806 = ARP
    return (packet->data[12] == 0x08 && packet->data[13] == 0x06);
}

void Tracker::ProcessARPReply(const Networking::Packet* packet) {
    // ARP packet is 28 bytes for IPv4 over Ethernet
    if (packet->length < 28) {
        LOG_DEBUG("ARP packet too short: %u32 bytes", packet->length);
        return;
    }
    
    const uint8_t* arp = packet->data;
    
    // Check operation: 0x0001 = request, 0x0002 = reply
    uint16_t operation = (arp[6] << 8) | arp[7];
    if (operation != 0x0002) {
        // Not a reply, ignore
        return;
    }
    
    // Extract sender's MAC address (bytes 8-13)
    Networking::MACAddress senderMAC;
    for (int i = 0; i < 6; i++) {
        senderMAC.octets[i] = arp[8 + i];
    }
    
    // Extract sender's IP address (bytes 14-17)
    Networking::IPv4Address senderIP;
    senderIP.octets[0] = arp[14];
    senderIP.octets[1] = arp[15];
    senderIP.octets[2] = arp[16];
    senderIP.octets[3] = arp[17];
    
    // Insert into ARP table - now we can access _arpTable directly
    s_instance->_arpTable.Insert(senderIP, senderMAC, Networking::ARPEntryState::Resolved);
    
    LOG_DEBUG("ARP reply: %u32.%u32.%u32.%u32 -> %x8:%x8:%x8:%x8:%x8:%x8",
              senderIP.octets[0], senderIP.octets[1], senderIP.octets[2], senderIP.octets[3],
              senderMAC.octets[0], senderMAC.octets[1], senderMAC.octets[2],
              senderMAC.octets[3], senderMAC.octets[4], senderMAC.octets[5]);
}

// High level TX / RX
STATUS Tracker::SendTo(const Networking::TransmitTarget* target, const uint8_t* data, uint16_t length) {
    if (!target || !data || length == 0) return STATUS::INVALID_ARGUMENT;

    // Find the first up interface to send on
    Networking::NetworkInterface* iface = nullptr;
    for (size_t i = 0; i < s_instance->_interfaces.Size(); i++) {
        if (s_instance->_interfaces[i].state == Networking::InterfaceState::Up &&
            s_instance->_interfaces[i].txFunc != nullptr) {
            iface = &s_instance->_interfaces[i];
            break;
        }
    }

    if (!iface) return STATUS::NOT_FOUND;

    switch (target->type) {
        case Networking::TransmitTargetType::Broadcast: {
            Networking::MACAddress broadcastMAC = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
            Networking::Packet packet {
                .interfaceID = iface->ID,
                .length      = length,
                .data        = const_cast<uint8_t*>(data),
                .destMAC     = broadcastMAC,
                .srcMAC      = {}
            };
            return iface->txFunc(&packet);
        }

        case Networking::TransmitTargetType::Unicast: {
            // Check if a MAC was explicitly provided
            Networking::MACAddress zero = {};
            bool macProvided = false;
            for (int i = 0; i < 6; i++) {
                if (target->address.mac.octets[i] != zero.octets[i]) {
                    macProvided = true;
                    break;
                }
            }

            if (macProvided) {
                Networking::Packet packet {
                    .interfaceID = iface->ID,
                    .length      = length,
                    .data        = const_cast<uint8_t*>(data),
                    .destMAC     = target->address.mac,
                    .srcMAC      = {}
                };
                return iface->txFunc(&packet);
            }

            // Try to resolve via ARP
            Networking::MACAddress resolvedMAC;
            if (s_instance->_arpTable.Resolve(target->address.IP.v4, &resolvedMAC)) {
                Networking::Packet packet {
                    .interfaceID = iface->ID,
                    .length      = length,
                    .data        = const_cast<uint8_t*>(data),
                    .destMAC     = resolvedMAC,
                    .srcMAC      = {}
                };
                return iface->txFunc(&packet);
            }

            // Not in ARP table yet - mark as pending and send request
            s_instance->_arpTable.MarkPending(target->address.IP.v4);
            Tracker::SendARPRequest(target->address.IP.v4);
            
            // Return NOT_FOUND since we can't send now
            // The caller should retry after a short delay
            return STATUS::NOT_FOUND;
        }
        
        // Handle multicast similarly
        case Networking::TransmitTargetType::Multicast: {
            Networking::MACAddress multicastMAC = {};

            if (target->address.ipVersion == Networking::IPVersion::IPv4) {
                // IPv4 multicast MACs: 01:00:5E + lower 23 bits of the IP
                const uint8_t *ip = target->address.IP.v4.octets;
                multicastMAC = {
                    0x01, 0x00, 0x5E,
                    (uint8_t)(ip[1] & 0x7F),
                    ip[2],
                    ip[3]
                };
            } else {
                // IPv6 multicast MACs: 33:33 + lower 32 bits of the IPv6 address
                const uint8_t *ip = target->address.IP.v6.octets;
                multicastMAC = {
                    0x33, 0x33,
                    ip[12],
                    ip[13],
                    ip[14],
                    ip[15]
                };
            }

            Networking::Packet packet {
                .interfaceID = iface->ID,
                .length      = length,
                .data        = const_cast<uint8_t *>(data),
                .destMAC     = multicastMAC,
                .srcMAC      = {}
            };
            return iface->txFunc(&packet);
        }
    }

    return STATUS::INVALID_ARGUMENT;
}

STATUS Tracker::SendARPRequest(const Networking::IPv4Address &IP) {
    // The ARP request packet layout is 28 bytes in size:
    // Hardware type (2), Protocol type (2), HW addr len (1), Proto addr len (1),
    // Operation (2), Sender MAC (6), Sender IP (4), Target MAC (6), Target IP (4)
    uint8_t arpRequest[28] = {};

    arpRequest[2] = 0x08; arpRequest[3] = 0x00; // Protocol type (IPv4)
    arpRequest[4] = 6;                          // Hardware address length
    arpRequest[5] = 4;                          // Protocol address length
    arpRequest[6] = 0x00; arpRequest[7] = 0x01; // Operation (request)

    for (size_t i = 0; i < s_instance->_interfaces.Size(); i++) {
        Networking::NetworkInterface &iface = s_instance->_interfaces[i];
        if (iface.state != Networking::InterfaceState::Up) continue;

        // The hardware type depends on the interface
        switch (iface.type) {
            case Networking::InterfaceType::WiFi:
                arpRequest[0] = 0x00; arpRequest[1] = 0x06;
                break;

            case Networking::InterfaceType::Ethernet:
            default:
                arpRequest[0] = 0x00; arpRequest[1] = 0x01;
                break;
        }

        // Bytes 8–13 describe the sender's MAC
        for (int j = 0; j < 6; j++)
            arpRequest[8 + j] = iface.mac.octets[j];

        // Bytes 14–17 make up the sender's IP
        if (iface.ipVersion == Networking::IPVersion::IPv4) {
            arpRequest[14] = iface.ip.v4.octets[0];
            arpRequest[15] = iface.ip.v4.octets[1];
            arpRequest[16] = iface.ip.v4.octets[2];
            arpRequest[17] = iface.ip.v4.octets[3];
        }
        else {
            arpRequest[14] = iface.ip.v6.octets[0];
            arpRequest[15] = iface.ip.v6.octets[1];
            arpRequest[16] = iface.ip.v6.octets[2];
            arpRequest[17] = iface.ip.v6.octets[3];
        }

        // Bytes 24-27 make up the target IP
        arpRequest[24] = IP.octets[0];
        arpRequest[25] = IP.octets[1];
        arpRequest[26] = IP.octets[2];
        arpRequest[27] = IP.octets[3];

        // The target MAC (bytes 18–23) must be all zeros for a request
        // Bytes 24–27 describe the target IP
        Networking::Packet packet {
            .interfaceID = iface.ID,
            .length      = sizeof(arpRequest),
            .data        = arpRequest,
            .destMAC     = { .octets = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } },
            .srcMAC      = {}
        };
        return iface.txFunc(&packet);
    }

    return STATUS::NOT_FOUND;
}

STATUS Tracker::SubscribeReceive(const Networking::ReceiveFilter* filter, Networking::NetworkEventCallback callback, uint64_t* outSubscriptionID) {
    if (!filter || !callback || !outSubscriptionID) return STATUS::INVALID_ARGUMENT;

    uint64_t id = GenerateSubscriptionID();
    Networking::SubscriptionInfo sub {
        .interfaceID      = 0,
        .subscriptionID   = id,
        .netEventCallback = callback,
    };

    s_instance->_receiveHandlers.Add(sub);
    s_instance->_receiveFilters.Add(*filter);
    *outSubscriptionID = id;
    return STATUS::SUCCESS;
}

STATUS Tracker::UnsubscribeReceive(uint64_t subscriptionID) {
    for (size_t i = 0; i < s_instance->_receiveHandlers.Size(); i++) {
        if (s_instance->_receiveHandlers[i].subscriptionID == subscriptionID) {
            s_instance->_receiveHandlers.Remove(i);
            s_instance->_receiveFilters.Remove(i);
            return STATUS::SUCCESS;
        }
    }

    return STATUS::NOT_FOUND;
}

bool Tracker::MatchesFilter(const Networking::ReceiveFilter &filter, const Networking::NetworkEvent* event) {
    if (event->type != Networking::NetworkEventType::PacketReceived)
        return false;

    const uint8_t* frameData = event->packetEvent.packet.data;
    uint16_t frameLen = event->packetEvent.packet.length;

    switch (filter.type) {
        case Networking::ReceiveFilterType::All:
            return true;

        case Networking::ReceiveFilterType::Broadcast: {
            if (frameLen < 6) return false;
            for (int i = 0; i < 6; i++)
                if (frameData[i] != 0xFF) return false;

            return true;
        }

        case Networking::ReceiveFilterType::Multicast: {
            if (frameLen < 1) return false;
            return (frameData[0] & 0x01) != 0;
        }

        case Networking::ReceiveFilterType::Unicast: {
            if (frameLen < 6) return false;
            for (size_t i = 0; i < s_instance->_interfaces.Size(); i++) {
                bool match = true;

                for (int j = 0; j < 6; j++) {
                    if (s_instance->_interfaces[i].mac.octets[j] != frameData[j]) {
                        match = false;
                        break;
                    }
                }

                if (match) return true;
            }

            return false;
        }

        case Networking::ReceiveFilterType::FromMAC: {
            if (frameLen < 12) return false;
            for (int i = 0; i < 6; i++)
                if (frameData[6 + i] != filter.address.mac.octets[i]) return false;

            return true;
        }

        case Networking::ReceiveFilterType::FromIP:
            // TODO: implement after IP address is added to NetworkInterface
            return false;
    }

    return false;
}

// NIC management
STATUS Tracker::RegisterInterface(const Networking::NetworkInterface* interface) {
    if (!interface) return STATUS::INVALID_ARGUMENT;
    if (s_instance->HasInterface(interface->ID)) return STATUS::ALREADY_EXISTS;
    s_instance->_interfaces.Add(*interface);
    return STATUS::SUCCESS;
}

STATUS Tracker::UnregisterInterface(uint64_t interfaceID) {
    for (size_t i = 0; i < s_instance->_interfaces.Size(); i++) {
        if (s_instance->_interfaces[i].ID == interfaceID) {
            s_instance->_interfaces.Remove(i);
            for (int j = s_instance->_eventHandlers.Size() - 1; j >= 0; j--) {
                if (s_instance->_eventHandlers[j].interfaceID == interfaceID)
                    s_instance->_eventHandlers.Remove(j);
            }

            return STATUS::SUCCESS;
        }
    }

    return STATUS::NOT_FOUND;
}

bool Tracker::HasInterface(uint64_t interfaceID) {
    for (size_t i = 0; i < s_instance->_interfaces.Size(); i++)
        if (s_instance->_interfaces[i].ID == interfaceID)
            return true;

    return false;
}

STATUS Tracker::GetInterfaceInfo(uint64_t interfaceID, Networking::NetworkInterface* out) {
    if (!out) return STATUS::INVALID_ARGUMENT;
    for (size_t i = 0; i < s_instance->_interfaces.Size(); i++) {
        if (s_instance->_interfaces[i].ID == interfaceID) {
            *out = s_instance->_interfaces[i];
            return STATUS::SUCCESS;
        }
    }

    return STATUS::NOT_FOUND;
}

STATUS Tracker::SetInterfaceState(uint64_t interfaceID, Networking::InterfaceState state) {
    for (size_t i = 0; i < s_instance->_interfaces.Size(); i++) {
        if (s_instance->_interfaces[i].ID == interfaceID) {
            s_instance->_interfaces[i].state = state;
            Networking::NetworkEvent newEvent {
                .interfaceID = interfaceID,
                .type = (state == Networking::InterfaceState::Up)
                    ? Networking::NetworkEventType::InterfaceUp
                    : Networking::NetworkEventType::InterfaceDown,
                .packetEvent = {}
            };

            BroadcastEvent(&newEvent);
            return STATUS::SUCCESS;
        }
    }

    return STATUS::NOT_FOUND;
}

STATUS Tracker::EnumerateInterfaces(Utility::List<Networking::NetworkInterface>* interfaceList) {
    for (size_t i = 0; i < s_instance->_interfaces.Size(); i++)
        interfaceList->Add(s_instance->_interfaces[i]);

    return STATUS::SUCCESS;
}

STATUS Tracker::SubscribeEvents(Networking::SubscriptionInfo* subscription) {
    if (!subscription) return STATUS::INVALID_ARGUMENT;
    if (subscription->interfaceID != 0 && !s_instance->HasInterface(subscription->interfaceID))
        return STATUS::NOT_FOUND;

    subscription->subscriptionID = GenerateSubscriptionID();
    s_instance->_eventHandlers.Add(*subscription);

    return STATUS::SUCCESS;
}

STATUS Tracker::UnsubscribeEvents(const Networking::SubscriptionInfo &subscription) {
    for (size_t i = 0; i < s_instance->_eventHandlers.Size(); i++) {
        if (s_instance->_eventHandlers[i].subscriptionID == subscription.subscriptionID) {
            s_instance->_eventHandlers.Remove(i);
            return STATUS::SUCCESS;
        }
    }

    return STATUS::NOT_FOUND;
}

void Tracker::BroadcastEvent(const Networking::NetworkEvent* event) {
    // Process incoming ARP packets first
    if (event->type == Networking::NetworkEventType::PacketReceived) {
        const auto& packet = event->packetEvent.packet;
        if (IsARPPacket(&packet)) ProcessARPReply(&packet);
    }

    // Low level event subscribers
    for (size_t i = 0; i < s_instance->_eventHandlers.Size(); i++) {
        auto &handler = s_instance->_eventHandlers[i];
        if (handler.interfaceID == 0 || handler.interfaceID == event->interfaceID)
            handler.netEventCallback(event);
    }

    // High level receive subscribers are filtered
    for (size_t i = 0; i < s_instance->_receiveHandlers.Size(); i++) {
        if (MatchesFilter(s_instance->_receiveFilters[i], event))
            s_instance->_receiveHandlers[i].netEventCallback(event);
    }
}

uint64_t Tracker::GenerateSubscriptionID() {
    static uint64_t nextID = 1;
    return nextID++;
}