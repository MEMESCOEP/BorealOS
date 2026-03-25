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
    .SendPacket          = SendPacket,
    .SubscribeEvents     = SubscribeEvents,
    .UnsubscribeEvents   = UnsubscribeEvents,
    .BroadcastEvent      = BroadcastEvent
} {
    s_instance = this;
}

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

            // Clean up any subscriptions for this interface
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
                .type        = (state == Networking::InterfaceState::Up)
                            ? Networking::NetworkEventType::InterfaceUp
                            : Networking::NetworkEventType::InterfaceDown,
                .packetEvent = {}  // zero-initialize the union
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

STATUS Tracker::SendPacket(const Networking::Packet* packet) {
    if (!packet) return STATUS::INVALID_ARGUMENT;
    for (size_t i = 0; i < s_instance->_interfaces.Size(); i++) {
        Networking::NetworkInterface &iface = s_instance->_interfaces[i];

        if (iface.ID != packet->interfaceID) continue;
        if (iface.state != Networking::InterfaceState::Up) return STATUS::DEVICE_NOT_READY;
        //if (packet->length > iface.mtu) return STATUS::PACKET_TOO_LARGE;
        if (!iface.txFunc) return STATUS::NOT_SUPPORTED;

        STATUS result = iface.txFunc(packet);
        if (result != STATUS::SUCCESS) return result;

        Networking::NetworkEvent newEvent {
            .interfaceID = packet->interfaceID,
            .type        = Networking::NetworkEventType::PacketSent,
            .packetEvent = { .packet = *packet },
        };

        BroadcastEvent(&newEvent);
        return STATUS::SUCCESS;
    }

    return STATUS::NOT_FOUND;
}

STATUS Tracker::SubscribeEvents(Networking::SubscriptionInfo* subscription) {
    if (!subscription) return STATUS::INVALID_ARGUMENT;

    // interfaceID 0 is the global subscription, so we can skip the existence check
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
    for (size_t i = 0; i < s_instance->_eventHandlers.Size(); i++) {
        auto &handler = s_instance->_eventHandlers[i];
        if (handler.interfaceID == 0 || handler.interfaceID == event->interfaceID)
            handler.netEventCallback(event);
    }
}

uint64_t Tracker::GenerateSubscriptionID() {
    static uint64_t nextID = 1;
    return nextID++;
}