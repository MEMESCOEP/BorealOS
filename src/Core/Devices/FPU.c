/* LIBRARIES */
#include <stdbool.h>
#include <stddef.h>
#include "Core/Graphics/Terminal.h"
#include "Kernel.h"
#include "FPU.h"


/* VARIABLES */
bool FPUInitialized = false;


/* FUNCTIONS */
void SetFPUCW(uint16_t CW)
{
    asm volatile ("fldcw %0" :: "m"(CW));
}

void InitFPU()
{
    TerminalDrawString("[INFO] >> Setting up CR4 register...\n\r");
    size_t Reg_cr4;

    asm volatile ("mov %%cr4, %0" : "=r"(Reg_cr4));

    Reg_cr4 |= 0x200;

    asm volatile ("mov %0, %%cr4" :: "r"(Reg_cr4));
    FPUInitialized = true;
}

void InitSSE()
{
    // Make sure the FPU is initialized before trying to initialize SSE.
    if (FPUInitialized == false)
    {
        KernelPanic(-10, "The FPU must be initialized before SSE can be initialized!");
    }

    int eax, edx;
    
    // Make sure SSE is supported before we try to initialize it.
    __asm__ (
        "movl $1, %%eax;"
        "cpuid;"
        "testl $0x2000000, %%edx;"
        "jz .noSSE;"
        ".noSSE:"
        : "=d" (edx)
        :
        : "%eax"
    );

    if ((edx & (1 << 25)) == 0)
    {
        KernelPanic(-11, "SSE is not supported!");
    }

    // SSE is supported by the CPU, now we can initialize it.
    // Read CR4
    unsigned long cr4;
    asm volatile ("mov %%cr4, %0" : "=r"(cr4));

    // Set OSFXSR bit in CR4
    cr4 |= (1 << 10);

    // Write back CR4
    asm volatile ("mov %0, %%cr4" : : "r"(cr4));

    // Read CR0
    unsigned long cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));

    // Clear EM bit in CR0
    cr0 &= ~(1 << 2);

    // Write back CR0
    asm volatile ("mov %0, %%cr0" : : "r"(cr0));
}