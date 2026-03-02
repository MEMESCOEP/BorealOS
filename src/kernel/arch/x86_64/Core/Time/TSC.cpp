#include "TSC.h"

namespace Core::Time {
    TSC::TSC(HPET *hpet, CPU *cpu) {
        if (!cpu->HasInvariantTSC()) {
            PANIC("CPU does not have an invariant TSC, cannot initialize TSC!");
        }

        // First try to get the frequency from the TSC frequency leaf.
        // If the returned values in EBX and ECX of leaf 15h are both nonzero, then the TSC (Time Stamp Counter) frequency in Hz is given by TSCFreq = ECX*(EBX/EAX).
        uint32_t eax, ebx, ecx, edx;
        __cpuid(0x15, eax, ebx, ecx, edx);
        if (ebx != 0 && ecx != 0) {
            frequency = (uint64_t)ecx * ((uint64_t)ebx / (uint64_t)eax);
            LOG_DEBUG("Frequency derived from CPUID leaf 0x15: %u64hz", frequency);
            return;
        }

        // On some processors (e.g. Intel Skylake),
        // CPUID_15h_ECX is zero but CPUID_16h_EAX is present and not zero.
        // On all known processors where this is the case,[132] the TSC frequency is equal to the Processor Base Frequency, and the Core Crystal Clock Frequency in Hz can be computed as
        // CoreCrystalFreq = (CPUID_16h_EAX * 10000000) * (CPUID_15h_EAX/CPUID_15h_EBX).
        __cpuid(0x16, eax, ebx, ecx, edx);
        if (eax != 0) {
            frequency = (uint64_t)eax * 10000000 * ((uint64_t)ebx / (uint64_t)eax);
            LOG_DEBUG("Frequency derived from CPUID leaf 0x16: %u64hz", frequency);
            return;
        }

        // Fall back to measuring the TSC frequency. (most inaccurate but whatever)
        frequency = CalculateFrequency(hpet);
        LOG_DEBUG("Frequency derived from measurement: %u64hz", frequency);
    }

    uint64_t TSC::GetFrequency() const {
        return frequency;
    }

    uint64_t TSC::GetTicks() const {
        uint32_t low, high;

        // Use lfence to prevent the CPU from executing rdtsc too early
        asm volatile("lfence\n\t"
                     "rdtsc\n\t"
                     : "=a"(low), "=d"(high));

        return ((uint64_t)high << 32) | low;
    }

    uint64_t TSC::GetNanoseconds() const {
        uint64_t ticks = GetTicks();
        uint64_t seconds = ticks / frequency;
        uint64_t residualTicks = ticks % frequency;
        uint64_t residualNs = (residualTicks * 1'000'000'000ULL) / frequency;

        return (seconds * 1'000'000'000ULL) + residualNs;
    }

    uint64_t TSC::CalculateFrequency(HPET *hpet) const {
        // We calculate the frequency by measuring how many ticks passed.
        asm volatile ("cli");

        auto hpetStart = hpet->GetCounter();
        auto tscStart = GetTicks();

        auto ns = 100 * 1000 * 1000; // 100ms
        hpet->BusyWait(ns);

        auto tscEnd = GetTicks();
        auto hpetEnd = hpet->GetCounter();

        auto hpetDelta = hpetEnd - hpetStart;
        auto tscDelta = tscEnd - tscStart;

        asm volatile ("sti");

        // frequency = ticks / seconds
        return tscDelta * 1'000'000'000ULL / (hpetDelta * 1'000'000'000ULL / hpet->GetFrequency());
    }
}
