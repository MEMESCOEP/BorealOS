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

    if (CPUHasFeature(KernelCPU.FeaturesEDX, CPUID_FEAT_EDX_FPU))
        CPUSetupFPU();

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

Status CPUSetupFPU() {
    if (!(CPUHasFeature(KernelCPU.FeaturesEDX, CPUID_FEAT_EDX_FPU))) {
        LOG(LOG_WARNING, "CPU does not have an FPU, cannot enable it.\n");
        return STATUS_UNSUPPORTED;
    }

    /*
    Check the FPU bit in CPUID
    Check the EM bit in CR0, if it is set then the FPU is not meant to be used.
    Check the ET bit in CR0, if it is clear, then the CPU did not detect an 80387 on boot
    Probe for an FPU
    */

    uint32_t cr0;
    ASM("mov %%cr0, %0" : "=r"(cr0));

    if (cr0 & (1 << 2)) {
        LOG(LOG_WARNING, "FPU Emulation bit is set in CR0, cannot enable FPU.\n");
        return STATUS_UNSUPPORTED;
    }
    if (!(cr0 & (1 << 4))) {
        LOG(LOG_WARNING, "FPU Extension Type bit is clear in CR0, CPU did not detect an FPU on boot.\n");
        return STATUS_UNSUPPORTED;
    }
    // Clear EM (bit 2) and TS (bit 3) to enable FPU
    cr0 &= ~(1 << 2);
    cr0 &= ~(1 << 3);
    ASM("mov %0, %%cr0" : : "r"(cr0));
    ASM("finit"); // Initialize the FPU

    // Test it by doing a simple operation
    ASM("fld1"); // Load 1.0 onto the FPU stack
    ASM("fld1"); // Load another 1.0 onto the FPU stack
    ASM("fadd"); // Add the two values together
    float result;
    ASM("fstp %0" : "=m"(result)); // Store the result and pop the stack
    if (result != 2.0f) {
        LOGF(LOG_WARNING, "FPU test failed, result was %f instead of 2.0\n", result);
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}

bool CPUHasFeature(uint32_t reg, uint32_t feature) {
    // Simple bitmask
    return (reg & feature) != 0;
}
