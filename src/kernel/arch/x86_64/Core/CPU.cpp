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

    // Initialize the FPU for drastically improved floating point math speed
    void CPU::InitializeFPU() {
        uint16_t statusWord = 0x55AA;
        
        // First, we try to initialize the FPU by calling "fninit"
        asm volatile(
            "fninit\n\t"
            "fnstsw %0"
            : "=m"(statusWord)
            :
            : "memory"
        );

        // Then, to detect the presence of the FPU, check the status word to make sure it has changed
        if (statusWord == 0x55AA) PANIC("This system doesn't have an FPU!");;
        LOG_DEBUG("FPU status: 0x%x16", statusWord);

        // Finally, set the NE bit (bit 5) for native exceptions
        unsigned long cr0 = ReadCR0();
        cr0 |=  (1 << 5);
        WriteCR0(cr0);

        LOG_INFO("FPU initialized.");
    }

    void CPU::InitializeSSE() {
        // Clear the EM bit (bit 2) and set the MP bit (bit 1) in CR0 to allow SSE instructions to execute without throwing invalid opcode exceptions
        unsigned long cr0 = ReadCR0();
        cr0 &= ~(1 << 2);
        cr0 |=  (1 << 1);
        WriteCR0(cr0);

        // These registers aren't important until multitasking is implemented:
        // FXSAVE / FXRSTOR / MXCSR / LDMXCSR / STMXCSR

        // Set the OSFXSR (bit 9) and OSXMMEXCPT (bit 10) bits in CR4
        unsigned long cr4 = ReadCR4();
        cr4 |= (1 << 9);
        cr4 |= (1 << 10);
        WriteCR4(cr4);

        // Test SSE if the kernel is in testing mode
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

    void CPU::Initialize() {
        // Get the  CPU's vendor ID
        LOG_INFO("Getting CPU properties...");

        // Get the processor's vendor ID
        if (__get_cpuid(CPUIDLeaves::HFP_ManufacturerID, &featureEAX, &featureEBX, &featureECX, &featureEDX)) {
            // The register order is intentional here!
            *reinterpret_cast<uint32_t*>(&vendorID[0]) = featureEBX;
            *reinterpret_cast<uint32_t*>(&vendorID[4]) = featureEDX;
            *reinterpret_cast<uint32_t*>(&vendorID[8]) = featureECX;
        }
        else {
            // Fill the vendor ID buffer with '?' if we couldn't get it from the CPU
            LOG_WARNING("Failed to get CPU vendor ID!");

            for (uint8_t charIndex = 0; charIndex < 12; charIndex++)
                vendorID[charIndex] = '?';
        }

        vendorID[12] = '\0';

        // Get the processor's name
        if (__get_cpuid(CPUIDLeaves::ExtendedFeature, &maxExtendedLeaf, &tempEBX, &tempECX, &tempEDX)) {
            uint32_t *strPtr = (uint32_t *)processorName;

            // Make sure the processor supports the brand string leaves
            if (maxExtendedLeaf < CPUID_BRAND_STRING_END) {
                for (uint8_t charIndex = 0; charIndex < 48; charIndex++) processorName[charIndex] = '?';
            }
            else {
                // Walk through the 3 CPUID leaves that make up the entire brand string
                // The CPUID leaves are 0x80000002, 0x80000003, and 0x80000004
                for (uint32_t leaf = CPUID_BRAND_STRING_START; leaf <= CPUID_BRAND_STRING_END; leaf++) {
                    // For each leaf, the CPU fills EAX/EBX/ECX/EDX; since each register is 32 bits, that's a total of 16 bytes per leaf
                    __get_cpuid(leaf, &tempEAX, &tempEBX, &tempECX, &tempEDX);
                    *strPtr++ = tempEAX;
                    *strPtr++ = tempEBX;
                    *strPtr++ = tempECX;
                    *strPtr++ = tempEDX;
                }

                // Remove trailing non-visible characters
                int lastTrailIndex = 47;
                for (int charIndex = 47; charIndex >= 0; charIndex--) {
                    if (processorName[charIndex] != ' ' && processorName[charIndex] != '\0') break;
                    lastTrailIndex = charIndex;
                }

                processorName[lastTrailIndex] = '\0';
            }
        }
        else {
            LOG_WARNING("Failed to get processor name!");
            for (uint8_t charIndex = 0; charIndex < 48; charIndex++) processorName[charIndex] = '?';
        }

        // The brand string isn't guaranteed to be null terminated
        processorName[48] = '\0';
        LOG_INFO("The installed processor is a \"%s\", its vendor ID is \"%s\".", processorName, vendorID);
        
        // Cache the CPU features
        if (!__get_cpuid(CPUIDLeaves::Features, &featureEAX, &featureEBX, &featureECX, &featureEDX)) PANIC("Failed to get CPU features!");

        // Check if the CPU supports SSE and FXSR
        if (!CPUHasFeature(CPUFeatures::SSE)) PANIC("This system doesn't support SSE, which is required by x86_64!");
        if (!CPUHasFeature(CPUFeatures::FXSR)) PANIC("This system doesn't support FXSR, which is required by x86_64!");

        // Initialize SSE
        InitializeSSE();

        // Initialize the FPU if there is one in the system
        InitializeFPU();
    }

    // This implementation is only safe for leaf 1, because it assumes CPUID set up the registers for leaf 1
    bool CPU::CPUHasFeature(CPUFeatures::Feature feature) {
        // Make sure the bit is between 0 and 31
        if (feature.bit > 31) return false;

        // Read the value from the proper register
        switch (feature.reg) {
            case CPUFeatures::Register::ECX:
                return (featureECX & (1u << feature.bit)) != 0;

            case CPUFeatures::Register::EDX:
                return (featureEDX & (1u << feature.bit)) != 0;
        }

        return false;
    }
}