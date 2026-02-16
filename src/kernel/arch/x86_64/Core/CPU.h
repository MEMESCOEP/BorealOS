#ifndef BOREALOS_CPU_H
#define BOREALOS_CPU_H

#include <Definitions.h>
#include <cpuid.h>

namespace Core {
    namespace CPUIDLeaves {
        constexpr unsigned int HFP_ManufacturerID = 0;
        constexpr unsigned int Features = 1;
        constexpr unsigned int Cache_TLB = 2;
        constexpr unsigned int SerialNumber = 3; // This feature is not implemented in any AMD CPUs or any Intel CPUs released after the Pentium III, use is not recommended
        constexpr unsigned int ExtendedFeature = 0x80000000;
    }

    namespace CPUFeatures {
        enum class Register : uint8_t {
            ECX,
            EDX
        };

        struct Feature {
            Register reg;
            uint8_t  bit;
        };

        constexpr Feature SSE3        { Register::ECX,  0 };
        constexpr Feature PCLMUL      { Register::ECX,  1 };
        constexpr Feature DTES64      { Register::ECX,  2 };
        constexpr Feature MONITOR     { Register::ECX,  3 };
        constexpr Feature DS_CPL      { Register::ECX,  4 };
        constexpr Feature VMX         { Register::ECX,  5 };
        constexpr Feature SMX         { Register::ECX,  6 };
        constexpr Feature EST         { Register::ECX,  7 };
        constexpr Feature TM2         { Register::ECX,  8 };
        constexpr Feature SSSE3       { Register::ECX,  9 };
        constexpr Feature CID         { Register::ECX, 10 };
        constexpr Feature SDBG        { Register::ECX, 11 };
        constexpr Feature FMA         { Register::ECX, 12 };
        constexpr Feature CX16        { Register::ECX, 13 };
        constexpr Feature XTPR        { Register::ECX, 14 };
        constexpr Feature PDCM        { Register::ECX, 15 };
        constexpr Feature PCID        { Register::ECX, 17 };
        constexpr Feature DCA         { Register::ECX, 18 };
        constexpr Feature SSE4_1      { Register::ECX, 19 };
        constexpr Feature SSE4_2      { Register::ECX, 20 };
        constexpr Feature X2APIC      { Register::ECX, 21 };
        constexpr Feature MOVBE       { Register::ECX, 22 };
        constexpr Feature POPCNT      { Register::ECX, 23 };
        constexpr Feature TSC_DEADLINE{ Register::ECX, 24 };
        constexpr Feature AES         { Register::ECX, 25 };
        constexpr Feature XSAVE       { Register::ECX, 26 };
        constexpr Feature OSXSAVE     { Register::ECX, 27 };
        constexpr Feature AVX         { Register::ECX, 28 };
        constexpr Feature F16C        { Register::ECX, 29 };
        constexpr Feature RDRAND      { Register::ECX, 30 };
        constexpr Feature HYPERVISOR  { Register::ECX, 31 };
        constexpr Feature FPU         { Register::EDX,  0 };
        constexpr Feature VME         { Register::EDX,  1 };
        constexpr Feature DE          { Register::EDX,  2 };
        constexpr Feature PSE         { Register::EDX,  3 };
        constexpr Feature TSC         { Register::EDX,  4 };
        constexpr Feature MSR         { Register::EDX,  5 };
        constexpr Feature PAE         { Register::EDX,  6 };
        constexpr Feature MCE         { Register::EDX,  7 };
        constexpr Feature CX8         { Register::EDX,  8 };
        constexpr Feature APIC        { Register::EDX,  9 };
        constexpr Feature SEP         { Register::EDX, 11 };
        constexpr Feature MTRR        { Register::EDX, 12 };
        constexpr Feature PGE         { Register::EDX, 13 };
        constexpr Feature MCA         { Register::EDX, 14 };
        constexpr Feature CMOV        { Register::EDX, 15 };
        constexpr Feature PAT         { Register::EDX, 16 };
        constexpr Feature PSE36       { Register::EDX, 17 };
        constexpr Feature PSN         { Register::EDX, 18 };
        constexpr Feature CLFLUSH     { Register::EDX, 19 };
        constexpr Feature DS          { Register::EDX, 21 };
        constexpr Feature ACPI        { Register::EDX, 22 };
        constexpr Feature MMX         { Register::EDX, 23 };
        constexpr Feature FXSR        { Register::EDX, 24 };
        constexpr Feature SSE         { Register::EDX, 25 };
        constexpr Feature SSE2        { Register::EDX, 26 };
        constexpr Feature SS          { Register::EDX, 27 };
        constexpr Feature HTT         { Register::EDX, 28 };
        constexpr Feature TM          { Register::EDX, 29 };
        constexpr Feature IA64        { Register::EDX, 30 };
        constexpr Feature PBE         { Register::EDX, 31 };
    }
    
    class CPU {
        public:
        void Initialize();
        bool CPUHasFeature(CPUFeatures::Feature feature);
        char processorName[49];
        char vendorID[13];

        private:
        static constexpr unsigned int CPUID_BRAND_STRING_START = 0x80000002;
        static constexpr unsigned int CPUID_BRAND_STRING_END = 0x80000004;

        void WriteCR0(unsigned long value);
        unsigned long ReadCR0(void);
        void WriteCR4(unsigned long value);
        unsigned long ReadCR4(void);
        void InitializeSSE();
        void InitializeFPU();
        uint32_t featureEAX, featureEBX, featureECX, featureEDX; // These should only be used for the CPUID supported features leaf
        uint32_t tempEAX, tempEBX, tempECX, tempEDX; // Temporary registers
        uint32_t maxExtendedLeaf;
    };
}

#endif //BOREALOS_CPU_H