#include "CPU.h"
#include "Settings.h"

namespace Core {
    void CPU::WriteCR0(unsigned long value) {
        __asm__ volatile (
            "mov %0, %%cr0"
            :
            : "r"(value)
            : "memory"
        );
    }

    unsigned long CPU::ReadCR0(void) {
        unsigned long value;
        __asm__ volatile (
            "mov %%cr0, %0"
            : "=r"(value)
            :
            : "memory"
        );
        return value;
    }

    void CPU::WriteCR4(unsigned long value) {
        __asm__ volatile (
            "mov %0, %%cr4"
            :
            : "r"(value)
            : "memory"
        );
    }

    unsigned long CPU::ReadCR4(void) {
        unsigned long value;
        __asm__ volatile (
            "mov %%cr4, %0"
            : "=r"(value)
            :
            : "memory"
        );
        return value;
    }

    int TestSSE() {
        float a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
        float b[4] = {5.0f, 6.0f, 7.0f, 8.0f};
        float r[4];

        // Perform SSE arithmetic
        __asm__ volatile (
            "movaps (%0), %%xmm0\n\t"
            "movaps (%1), %%xmm1\n\t"
            "addps %%xmm1, %%xmm0\n\t"
            "movaps %%xmm0, (%2)\n\t"
            :
            : "r"(a), "r"(b), "r"(r)
            : "xmm0", "xmm1"
        );

        // Compare results to expected constants
        int fail = 0;
        float expected[4] = {6.0f, 8.0f, 10.0f, 12.0f};
        for (int i = 0; i < 4; i++) {
            if (r[i] != expected[i]) {
                LOG_DEBUG("Mismatch at %d: got %f, expected %f", i, r[i], expected[i]);
                fail += 1;
            }
        }

        return fail;
    }

    void CPU::Initialize() {
        // Get the  CPU's vendor ID
        LOG_INFO("Getting CPU vendor ID and features...");
        if (__get_cpuid(CPUIDLeaves::HFP_ManufacturerID, &eax, &ebx, &ecx, &edx)) {
            *reinterpret_cast<uint32_t*>(&VendorID[0]) = ebx;
            *reinterpret_cast<uint32_t*>(&VendorID[4]) = edx;
            *reinterpret_cast<uint32_t*>(&VendorID[8]) = ecx;
        }
        else {
            LOG_WARNING("Failed to get CPU vendor ID!");

            for (uint8_t charIndex = 0; charIndex < 12; charIndex++)
                VendorID[charIndex] = '?';
        }

        VendorID[12] = '\0';
        LOG_INFO("CPU vendor ID is \"%s\"", VendorID);
        
        // Cache the CPU features
        if (!__get_cpuid(CPUIDLeaves::Features, &eax, &ebx, &ecx, &edx)) PANIC("Failed to get CPU features!");

        // Check if the CPU has SSE
        if (CPUHasFeature(CPUFeatures::SSE)) {
            LOG_INFO("Initializing SSE...");

            // Clear the EM bit (bit 2) in CR0 to allow SSE instructions to execute without throwing invalid opcode exceptions
            unsigned long cr0 = ReadCR0();
            cr0 &= ~(1 << 2);
            WriteCR0(cr0);

            // Set the OSFXSR (bit 9) and OSXMMEXCPT (bit 10) bits in CR4
            unsigned long cr4 = ReadCR4();
            cr4 |= (1 << 9);
            cr4 |= (1 << 10);
            WriteCR4(cr4);

            // FXSAVE / FXRSTOR / MXCSR / LDMXCSR / STMXCSR
            // These registers aren't important until multitasking is implemented
            FXSRSupported = CPUHasFeature(CPUFeatures::FXSR);
            if (FXSRSupported) {
                LOG_INFO("FXSAVE and FXRSTOR registers are supported.");
            }
            else {
                LOG_WARNING("This CPU does not support FXSAVE or FXRSTOR. SSE multitasking will not work.");
            }

            #if SETTING_TEST_MODE
            LOG_DEBUG("Testing SSE...");
            int SSETestResults = TestSSE();
            
            if (SSETestResults != 0) {
                LOG_ERROR("SSE failed %i64 test(s).", SSETestResults);
                PANIC("SSE test failed!");
            }

            #endif

            LOG_INFO("SSE initialized.");
        }
        else {
            LOG_WARNING("This CPU does not support SSE.");
        }
    }

    // This implementation is only safe for leaf 1, because it assumes CPUID set up the registers for leaf 1
    bool CPU::CPUHasFeature(CPUFeatures::Feature feature) {
        // Make sure the bit is between 0 and 31
        if (feature.bit > 31) return false;

        // Read the value from the proper register
        switch (feature.reg) {
            case CPUFeatures::Register::ECX:
                return (ecx & (1u << feature.bit)) != 0;

            case CPUFeatures::Register::EDX:
                return (edx & (1u << feature.bit)) != 0;
        }

        return false;
    }
}