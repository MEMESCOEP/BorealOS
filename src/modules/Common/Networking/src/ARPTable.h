#ifndef BOREALOS_ARP_TABLE_H
#define BOREALOS_ARP_TABLE_H

#include <Utility/List.h>
#include "../Service.h"

namespace Networking {

    enum class ARPEntryState : uint8_t {
        Pending,   // The ARP request was sent, we're now waiting for a reply
        Resolved,  // The MAC address is known for this IP
        Static     // Manually added, never expires (e.g. loopback)
    };

    struct ARPEntry {
        IPv4Address  ip;
        MACAddress   mac;
        ARPEntryState state;
    };

    class ARPTable {
    public:
        // Look up a MAC for a given IP
        // Returns true and fills *outMAC if resolved, false if pending or unknown
        bool Resolve(const IPv4Address &ip, MACAddress *outMAC);

        // Add or update an entry, e.g. when an ARP reply arrives
        void Insert(const IPv4Address &ip, const MACAddress &mac, ARPEntryState state = ARPEntryState::Resolved);

        // Add a static entry that never gets removed, e.g. for loopback
        void InsertStatic(const IPv4Address &ip, const MACAddress &mac);

		// Mark an entry as pending while we wait for an ARP reply
        void MarkPending(const IPv4Address &ip);

        // Remove an entry, e.g. when a device leaves the network or is unreachable
        void Remove(const IPv4Address &ip);

        // Returns true if there is a resolved entry for the given IP
        bool IsMACResolved(const IPv4Address &ip);

    private:
        ARPEntry *Find(const IPv4Address &ip);
        Utility::List<ARPEntry> _entries;
    };

} // namespace Networking

#endif // BOREALOS_ARP_TABLE_H