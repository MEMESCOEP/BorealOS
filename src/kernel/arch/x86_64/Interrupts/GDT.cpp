#include "GDT.h"

#include "TSS.h"

extern "C" {
    extern void LoadGDT(uint64_t gdt_pointer);
    extern void LoadTSS(uint16_t tss_selector);
}

namespace Interrupts {
    uint64_t GDT::entries[5]; // Null, Code, Data, TSS low and high
    GDT::GDTPointer GDT::gdtp;

    void GDT::SetEntry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
        GDTEntry entry{};

        entry.LimitLow = limit & 0xFFFF;
        entry.BaseLow = base & 0xFFFF;
        entry.BaseMiddle = (base >> 16) & 0xFF;
        entry.Access = access;
        entry.Granularity = (limit >> 16) & 0x0F;
        entry.Granularity |= granularity & 0xF0;
        entry.BaseHigh = (base >> 24) & 0xFF;

        entries[index] = *reinterpret_cast<uint64_t*>(&entry);
    }

    void GDT::SetTSSDescriptor(int index, uint64_t base, uint32_t limit) {
        uint64_t low = 0;
        uint64_t high = 0;

        low |= (limit & 0xFFFFULL);
        low |= (base & 0xFFFFFFULL) << 16;
        low |= (0x89ULL) << 40;
        low |= ((limit >> 16) & 0xFULL) << 48;
        low |= ((base >> 24) & 0xFFULL) << 56;

        high |= (base >> 32) & 0xFFFFFFFFULL;

        entries[index] = low;
        entries[index + 1] = high;
    }

    void GDT::Initialize() {
        TSS::Initialize();
        auto tss_address = reinterpret_cast<uintptr_t>(TSS::GetTSSStruct());

        // Null descriptor
        SetEntry(0, 0, 0, 0, 0);

        // Code segment descriptor
        SetEntry(1, 0, 0xFFFFF, 0x9A, 0xA0);

        // Data segment descriptor
        SetEntry(2, 0, 0xFFFFF, 0x92, 0xA0);

        // TSS descriptor
        SetTSSDescriptor(3, tss_address, sizeof(TSS::TSSStruct) - 1);

        gdtp.Limit = sizeof(entries) - 1;
        gdtp.Base = reinterpret_cast<uint64_t>(&entries);

        LoadGDT(reinterpret_cast<uint64_t>(&gdtp));
        LoadTSS(0x18); // TSS selector is at offset 0x18 in the GDT (3rd entry, 3 * 8 = 0x18)
    }

    GDT::GDTPointer * GDT::GetGDTPointer() {
        return &gdtp;
    }
} // Interrupts