#include <Kernel.h>
#include <Module.h>
#include <Core/ServiceManager.h>
#include <KernelData.h>

#include "../Service.h"
#include "Tracker.h"

MODULE(NETWORKING_MODULE_NAME, NETWORKING_MODULE_DESCRIPTION, NETWORKING_MODULE_VERSION, NETWORKING_MODULE_IMPORTANCE);

COMPATIBLE_FUNC() {
    // The networking service is compatible with all systems, since it is abstract.
    return true;
}

LOAD_FUNC() {
    auto tracker = new Tracker();
    Core::ServiceManager::GetInstance()->RegisterService(NETWORKING_SERVICE_NAME, tracker->GetService());

    // This example demonstrates how a NIC driver would interact with the networking service
    /*auto *service = static_cast<Networking::NetworkService *>(
        Core::ServiceManager::GetInstance()->GetService(NETWORKING_SERVICE_NAME));

    // Register a NIC as an interface
    Networking::NetworkInterface testInterface {
        .ID            = 1,
        .type          = Networking::InterfaceType::Ethernet,
        .state         = Networking::InterfaceState::Down,
        .mac           = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01 },
        .vendorID      = 0x8086,
        .productID     = 0x100E,
        .interfaceName = "eth0",
        .manufacturer  = "Intel",
        .description   = "Intel 82540EM Gigabit Ethernet (test)",
        .mtu           = 1500,
        .ipVersion     = Networking::IPVersion::IPv4,
        .ip            = { 127, 0, 0, 1 },
        .txFunc        = [](const Networking::Packet *packet) -> STATUS {
            LOG_DEBUG("txFunc called: sending %u16 bytes on interface %u64", packet->length, packet->interfaceID);
            return STATUS::SUCCESS;
        }
    };

    service->RegisterInterface(&testInterface);
    LOG_DEBUG("Registered interface '%s' (ID %u64)", testInterface.interfaceName, testInterface.ID);

    // Subscribe to all interface events
    Networking::SubscriptionInfo globalSub {
        .interfaceID      = 0, // 0 = all interfaces
        .subscriptionID   = 0, // This is filled in by SubscribeEvents
        .netEventCallback = [](const Networking::NetworkEvent *event) {
            switch (event->type) {
                case Networking::NetworkEventType::InterfaceUp:
                    LOG_DEBUG("Interface %u64 is now up", event->interfaceID);
                    break;

                case Networking::NetworkEventType::InterfaceDown:
                    LOG_DEBUG("Interface %u64 is now down", event->interfaceID);
                    break;

                case Networking::NetworkEventType::PacketReceived:
                    LOG_DEBUG("Packet received on interface %u64 (%u32 bytes)", event->interfaceID, event->packetEvent.packet.length);
                    break;

                case Networking::NetworkEventType::PacketSent:
                    LOG_DEBUG("Packet sent on interface %u64 (%u32 bytes)", event->interfaceID, event->packetEvent.packet.length);
                    break;

                case Networking::NetworkEventType::LinkSpeedChanged:
                    LOG_DEBUG("Link speed changed on interface %u64: %u32 Mbps", event->interfaceID, event->linkSpeedEvent.speedMbps);
                    break;
            }
        }
    };

    service->SubscribeEvents(&globalSub);
    LOG_DEBUG("Subscribed globally (subscriptionID %u64)", globalSub.subscriptionID);

    // Bring the interface up; this fires InterfaceUp event
    service->SetInterfaceState(testInterface.ID, Networking::InterfaceState::Up);

    // Send a packet to a specific MAC using SendTo
    uint8_t testData[] = { 0x01, 0x02, 0x03, 0x04 };
    Networking::TransmitTarget target {
        .type    = Networking::TransmitTargetType::Unicast,
        .address = {
            .ipVersion = Networking::IPVersion::IPv4,
            .IP        = {},
            .mac       = { 0xCA, 0xFE, 0xBA, 0xBE, 0x00, 0x01 },
        }
    };

    service->SendTo(&target, testData, sizeof(testData));

    // Send a broadcast
    Networking::TransmitTarget broadcast {
        .type    = Networking::TransmitTargetType::Broadcast,
        .address = {}
    };

    service->SendTo(&broadcast, testData, sizeof(testData));

    // Simulate a driver receiving a packet by broadcasting a PacketReceived event
    Networking::Packet rxPacket {
        .interfaceID = testInterface.ID,
        .length      = sizeof(testData),
        .data        = testData,
        .destMAC     = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01 },
        .srcMAC      = { 0xCA, 0xFE, 0xBA, 0xBE, 0x00, 0x01 },
    };

    Networking::NetworkEvent rxEvent {
        .interfaceID = testInterface.ID,
        .type        = Networking::NetworkEventType::PacketReceived,
        .packetEvent = { .packet = rxPacket },
    };

    service->BroadcastEvent(&rxEvent);

    // Enumerate all registered interfaces
    Utility::List<Networking::NetworkInterface> interfaces;
    service->EnumerateInterfaces(&interfaces);
    LOG_DEBUG("Enumerated %u32 interface(s)", interfaces.Size());

    // Clean up after ourselves by unsubscribing from events, bringing the interface down, and unregistering it
    service->UnsubscribeEvents(globalSub);
    service->SetInterfaceState(testInterface.ID, Networking::InterfaceState::Down);
    service->UnregisterInterface(testInterface.ID);
    */
    
    return STATUS::SUCCESS;
}