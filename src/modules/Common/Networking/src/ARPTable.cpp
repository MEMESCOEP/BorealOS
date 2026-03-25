#include "ARPTable.h"

namespace Networking {

    static bool IPv4Equal(const IPv4Address &a, const IPv4Address &b) {
        return a.octets[0] == b.octets[0] &&
               a.octets[1] == b.octets[1] &&
               a.octets[2] == b.octets[2] &&
               a.octets[3] == b.octets[3];
    }

    ARPEntry *ARPTable::Find(const IPv4Address &ip) {
        for (size_t i = 0; i < _entries.Size(); i++)
            if (IPv4Equal(_entries[i].ip, ip))
                return &_entries[i];
			
        return nullptr;
    }

    bool ARPTable::Resolve(const IPv4Address &ip, MACAddress *outMAC) {
        ARPEntry *entry = Find(ip);
        if (!entry || entry->state != ARPEntryState::Resolved)
            return false;

        *outMAC = entry->mac;
        return true;
    }

    void ARPTable::Insert(const IPv4Address &ip, const MACAddress &mac, ARPEntryState state) {
        ARPEntry *entry = Find(ip);
        if (entry) {
            entry->mac   = mac;
            entry->state = state;
        }
		else {
            _entries.Add({ .ip = ip, .mac = mac, .state = state });
        }
    }

    void ARPTable::InsertStatic(const IPv4Address &ip, const MACAddress &mac) {
        Insert(ip, mac, ARPEntryState::Static);
    }

    void ARPTable::MarkPending(const IPv4Address &ip) {
        ARPEntry *entry = Find(ip);
        if (entry) {
            entry->state = ARPEntryState::Pending;
        }
		else {
            MACAddress empty = {};
            _entries.Add({ .ip = ip, .mac = empty, .state = ARPEntryState::Pending });
        }
    }

    void ARPTable::Remove(const IPv4Address &ip) {
        for (size_t i = 0; i < _entries.Size(); i++) {
            if (IPv4Equal(_entries[i].ip, ip)) {
                _entries.Remove(i);
                return;
            }
        }
    }

    bool ARPTable::IsMACResolved(const IPv4Address &ip) {
        ARPEntry *entry = Find(ip);
        return entry && entry->state == ARPEntryState::Resolved;
    }

} // namespace Networking