#include "CPU.h"

#include <Core/Kernel.h>
#include <Utility/StringTools.h>

CPUSpec KernelCPU = {
    .Vendor = {0},
    .VendorID = CPU_VENDOR_VMWARE,
    .FeaturesEAX = 0,
    .FeaturesEBX = 0,
    .FeaturesECX = 0,
    .FeaturesEDX = 0
};

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

CPUVendor cpu_vendor_from_string(const char* str) {
    if (strncmp(str, "GenuineIntel", 12) == 0) {
        return CPU_VENDOR_INTEL;
    } else if (strncmp(str, "AuthenticAMD", 12) == 0) {
        return CPU_VENDOR_AMD;
    } else if (strncmp(str, " KVMKVMKVM  ", 12) == 0) {
        return CPU_VENDOR_KVM;
    } else if (strncmp(str, "TCGTCGTCGTCG", 12) == 0) {
        return CPU_VENDOR_QEMU;
    } else if (strncmp(str, "VBoxVBoxVBox", 12) == 0) {
        return CPU_VENDOR_VIRTUALBOX;
    } else if (strncmp(str, "VMwareVMware", 12) == 0) {
        return CPU_VENDOR_VMWARE;
    } else {
        return CPU_VENDOR_INTEL; // Default to Intel if unknown
    }
}

Status CPUInit() {
    // Get CPU vendor string
    uint32_t eax, ebx, ecx, edx;
    ASM("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );
    *((uint32_t*)&KernelCPU.Vendor[0]) = ebx;
    *((uint32_t*)&KernelCPU.Vendor[4]) = edx;
    *((uint32_t*)&KernelCPU.Vendor[8]) = ecx;
    KernelCPU.Vendor[12] = '\0';
    KernelCPU.VendorID = cpu_vendor_from_string(KernelCPU.Vendor);

    ASM("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );

    KernelCPU.FeaturesEAX = eax;
    KernelCPU.FeaturesEBX = ebx;
    KernelCPU.FeaturesECX = ecx;
    KernelCPU.FeaturesEDX = edx;

    if (CPUHasFeature(KernelCPU.FeaturesEDX, CPUID_FEAT_EDX_PAT))
        CPUSetupPAT();

    return STATUS_SUCCESS;
}

Status CPUSetupPAT() {
    if (!(CPUHasFeature(KernelCPU.FeaturesEDX, CPUID_FEAT_EDX_PAT))) {
        LOG(LOG_WARNING, "CPU does not support PAT, skipping PAT setup.\n");
        return STATUS_UNSUPPORTED;
    }

    uint64_t pat = CPUReadMSR(0x277);
    pat &= ~(0xFULL << 8);  // clear old value
    pat |= (uint64_t)1 << 8; // set WC

    CPUWriteMSR(0x277, pat);

    return STATUS_SUCCESS;
}

bool CPUHasFeature(uint32_t reg, uint32_t feature) {
    // Simple bitmask
    return (reg & feature) != 0;
}
