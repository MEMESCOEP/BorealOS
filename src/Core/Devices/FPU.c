/* LIBRARIES */
#include <stdbool.h>
#include <stddef.h>
#include <cpuid.h>
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

// Initialize SSE. An important thing to note is that we don't need to bother with FXSAVE or FXRSTOR because the OS is still
// currently single-threaded. Once support for threading and task switching are added, we have to make use of FXSAVE and FXRSTOR
// regardless of user mode threads or kernel mode threads. You need to save and restore the FPU/SSE state upon switching tasks
// See https://forum.osdev.org/viewtopic.php?p=302401&sid=d76ccb72a3bd70b81e075a8d22343826#p302401
void InitSSE()
{
    // Make sure the FPU is initialized before trying to initialize SSE.
    if (FPUInitialized == false)
    {
        KernelPanic(-10, "The FPU must be initialized before SSE can be initialized!");
    }

    // --- STEP 1 ---
    // Check if SSE v1+ is supported
    unsigned int eax, ebx, ecx, edx;
    TerminalDrawString("[INFO] >> Checking for SSE v1+ support...\n\r");

    // CPUID function 1 gets the processor info and feature bits
    // Setting EAX to 1 tells ther CPU to return its features
    eax = 1;  
    __cpuid(1, eax, ebx, ecx, edx);

    // CPUID.01h:EDX.SSE (bit 25) is set when SSE v1+ is supported
    TerminalDrawString("\tCPUID.01h:EDX.SSE\n\r");

    if (!(edx & (1 << 25)))
    {
        KernelPanic(-11, "SSE v1+ is not supported!");
    }

    TerminalDrawString("[INFO] >> SSE v1+ is supported.\n\r");



    // --- STEP 2 ---
    // Initialize SSE
    unsigned long cr0, cr4;

    // Read CR0 into RAX and into (CVAR)CR4
    TerminalDrawString("\tCR0 -> RAX -> (C_VAR)CR0\n\r");
    __asm__ volatile (
        "mov %%cr0, %%rax\n\t"
        "mov %%rax, %0"
        : "=r"(cr0)
        :
        : "rax"
    );

    // Disable emulation by clearing the EM bit (bit 2)
    // Tell the CPU to check for proper FPU usage upon switching tasks by setting the MP bit (bit 1)
    TerminalDrawString("\tCLEAR EM (BIT 2)\n\r");
    cr0 &= ~(1UL << 2); // Clear EM (bit 2)

    TerminalDrawString("\tSET MP (BIT 1)\n\r");
    cr0 |=  (1UL << 1); // Set MP (bit 1)

    // Write the updated CR0 value back into RAX & CR0
    TerminalDrawString("\tUPDATED CR0 -> RAX\n\r");
    __asm__ volatile (
        "mov %0, %%rax\n\t"
        "mov %%rax, %%cr0"
        :
        : "r"(cr0)
        : "rax"
    );

    // Read CR4 into RAX and into (CVAR)CR4
    TerminalDrawString("\tCR4 -> RAX -> (C_VAR)CR4\n\r");
    __asm__ volatile (
        "mov %%cr4, %%rax\n\t"
        "mov %%rax, %0"
        : "=r"(cr4)
        :
        : "rax"
    );

    // Enable FXSAVE and FXRSTOR for saving the state of the FPU/SSE (OSFXSR, bit 9)
    // Unmask SSE exceptions (OSXMMEXCPT, bit 10)
    TerminalDrawString("\tSET CR4 OSFXSR/OSXMMEXCPT BITS\n\r");
    cr4 |= (1UL << 9) | (1UL << 10); // Set OSFXSR and OSXMMEXCPT

    // Write the updated CR4 value back into RAX & CR4
    TerminalDrawString("\tSET CR4 OSFXSR/OSXMMEXCPT BITS\n\r");
    __asm__ volatile (
        "mov %0, %%rax\n\t"
        "mov %%rax, %%cr4"
        :
        : "r"(cr4)
        : "rax"
    );

    TerminalDrawString("\n\n\r");
}