/* LIBRARIES */
#include <Limine.h>
#include <stddef.h>
#include <cpuid.h>
#include "Utilities/StrUtils.h"
#include "CPU.h"


/* LIMINE REQUESTS */
__attribute__((used, section(".limine_requests")))
struct limine_mp_request MPRequest = {
    .id = LIMINE_MP_REQUEST,
    .revision = 0,
};


/* VARIABLES */
int ProcessorCount = 1;


/* FUNCTIONS */
void GetCPUVendorID(char* OutBuffer)
{
    unsigned int eax, ebx, ecx, edx;

    // --- STEP 1 ---
    // Issue the CPUID command to get the CPU's vendor ID; setting EAX to 0 tells
    // the CPU to return its brand string
    __cpuid(0, eax, ebx, ecx, edx);



    // --- STEP 2 ---
    // Copy EBX, EDX, ECX into the vendor string (cast char* to unsigned int* and treat the buffer as if it were an array of 3 unsigned ints)
    ((unsigned int *)OutBuffer)[0] = ebx;  // OutBuffer[0..3]  = EBX
    ((unsigned int *)OutBuffer)[1] = edx;  // OutBuffer[4..7]  = EDX
    ((unsigned int *)OutBuffer)[2] = ecx;  // OutBuffer[8..11] = ECX
    OutBuffer[12] = '\0';
}

void GetCPUBrandStr(char* OutBuffer)
{
    // Fill the output string with spaces so we can remove trailing characters later
    OutBuffer[48] = '\0';

    for (int I = 0; I < 48; I++)
    {
        OutBuffer[I] = ' ';
    }

    unsigned int regs[12];

    unsigned int dummy;
    __cpuid(0x80000000, dummy, regs[0], regs[1], regs[2]);

    // Check if the extended brand string functions are supported
    if (dummy < 0x80000004)
    {
        OutBuffer[0] = '\0'; // Return empty string
        return;
    }

    __cpuid(0x80000002, regs[0], regs[1], regs[2], regs[3]);
    __cpuid(0x80000003, regs[4], regs[5], regs[6], regs[7]);
    __cpuid(0x80000004, regs[8], regs[9], regs[10], regs[11]);

    // Copy each 4-byte register into OutBuffer as a char array
    for (int i = 0; i < 12; ++i)
    {
        ((unsigned int*)OutBuffer)[i] = regs[i];
    }

    // Strip any trailing whitespace characters from the final string (assuming there are any, if not we set \0 at index 48)
    TrimTrailingWhitespace(OutBuffer);
}

// Get the number of processors in the system
int GetProcessorCount()
{
    // This call may fail if there's only 1 CPU, or the system doesn't support SMP. If it does, we only report one CPU (multiple CPUs or CPU cores requires SMP to be supported). This is
    // a safe default because there always has to be at least one processor available (assuming a standard x86_64 configuration)
    if (MPRequest.response == NULL)
    {
        ProcessorCount = 1;
    }
    else
    {
        ProcessorCount = MPRequest.response->cpu_count;
    }

    return ProcessorCount;
}