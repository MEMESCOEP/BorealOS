#include "Syscall.h"

#include "Kernel.h"
#include "KernelData.h"
#include "TSS.h"

#define MSR_EFER 0xC0000080
#define MSR_STAR 0xC0000081
#define MSR_LSTAR 0xC0000082
#define MSR_FMASK 0xC0000084

extern "C" {
    extern void SyscallHandler();
    extern void EnterUserspace(uint64_t entryPoint, uint64_t userStack);

    void KernelSyscallHandler(Interrupts::Syscall::SyscallFrame* frame) {
        // For testing purposes, we'll just log the syscall number and return it incremented by 1.
        LOG_INFO("Syscall invoked with hex: 0x%x64", frame->rax);
        frame->rax += 1; // Increment the syscall number as a dummy "result"
    }

    extern char __user_trampoline_start[];
    extern char __user_trampoline_end[];
    uint64_t syscall_kernel_rsp;
}

// TODO: This needs to be rewritten when we support actual user processes, but for now this is just a test.
__attribute__((naked, used, section(".user_trampoline")))
static void UserTrampoline() {
    __asm__ volatile (
        "mov $0xDEADBEEF, %rax\n"
        "syscall\n"
        "jmp .\n" // Infinite loop to prevent falling through
    );
}

namespace Interrupts {
    void Syscall::Initialize() {
        // SCE bit (system call extensions) in EFER MSR
        uint64_t efer = Core::CPU::ReadMSR(MSR_EFER);
        efer |= 1; // Set SCE bit
        Core::CPU::WriteMSR(MSR_EFER, efer);

        // Set STAR MSR to define syscall/sysret segments and CPL3 code segment selector
        Core::CPU::WriteMSR(MSR_STAR, ((uint64_t)0x28 << 48) | ((uint64_t)0x08 << 32));
        Core::CPU::WriteMSR(MSR_LSTAR, (uint64_t)&SyscallHandler); // called when syscall is invoked with syscall instruction
        Core::CPU::WriteMSR(MSR_FMASK, 1 << 9);
    }

    void Syscall::Trampoline() {
        // TODO: This needs to be changed when we support actual user processes, maybe make this function take in an address to jump to? For now, this is just a test to make sure we can jump to userspace and execute code there.
        auto kernel = Kernel<KernelData>::GetInstance();
        constexpr uint64_t VIRT = 0x400000;
        auto physical = kernel->ArchitectureData->Pmm.AllocatePages(1);

        // This would be a per process page table, but since we dont have processes yet we'll just make a single one.
        auto trampolinePageState = kernel->ArchitectureData->Paging.CreatePagingStateForProcess(); // Create a new paging state for the trampoline.

        // Map the trampoline's code
        kernel->ArchitectureData->Paging.MapPage(trampolinePageState, VIRT, physical, Memory::PageFlags::Present | Memory::PageFlags::ReadWrite | Memory::PageFlags::User);
        LOG_INFO("user trampoline size: %u64", reinterpret_cast<uint64_t>(__user_trampoline_end) - reinterpret_cast<uint64_t>(__user_trampoline_start));

        // Map the trampoline's stack
        constexpr uint64_t USER_STACK = 0x800000;
        auto stackPhysical = kernel->ArchitectureData->Pmm.AllocatePages(1);
        kernel->ArchitectureData->Paging.MapPage(trampolinePageState, USER_STACK, stackPhysical, Memory::PageFlags::Present | Memory::PageFlags::ReadWrite | Memory::PageFlags::User);

        // Switch to the trampoline's page table, and copy the trampoline code to the mapped page, then just jump to it.
        kernel->ArchitectureData->Paging.SwitchToPageTable(trampolinePageState);
        memcpy((void*)VIRT, reinterpret_cast<void*>(__user_trampoline_start), reinterpret_cast<uint64_t>(__user_trampoline_end) - reinterpret_cast<uint64_t>(__user_trampoline_start));

        syscall_kernel_rsp = TSS::GetTSSStruct()->RSP0;
        EnterUserspace(VIRT, USER_STACK + Architecture::KernelPageSize);
    }
} // Interrupts