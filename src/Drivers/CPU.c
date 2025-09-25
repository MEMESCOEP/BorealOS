#include "CPU.h"

uint64_t CPUReadMSR(uint32_t msr) {
    uint32_t low, high;
    ASM("rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr)
    );
    return ((uint64_t)high << 32) | low;
}

void CPUWriteMSR(uint32_t msr, uint64_t value) {
    ASM("wrmsr"
        :
        : "c"(msr), "a"((uint32_t)(value & 0xFFFFFFFF)), "d"((uint32_t)(value >> 32))
    );
}

bool CPUHasPAT() {
    uint32_t eax, ebx, ecx, edx;
    ASM("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );
    return (edx & (1 << 16)) != 0; // Check if the PAT bit (bit 16) is set in EDX
}

Status CPUSetupPAT() {
    if (!CPUHasPAT()) {
        return STATUS_FAILURE; // PAT not supported
    }

    uint64_t pat = CPUReadMSR(0x277);
    pat &= ~(0xFULL << 8);  // clear old value
    pat |= (uint64_t)1 << 8; // set WC

    CPUWriteMSR(0x277, pat);

    return STATUS_SUCCESS;
}
