#ifndef BOREALOS_SYSCALL_H
#define BOREALOS_SYSCALL_H

#include <Definitions.h>

namespace Interrupts {

class Syscall {
public:
    static void Initialize();
    static void Trampoline(); // This jumps into userspace! With a test function at the moment.

    struct SyscallFrame {
        uint64_t r15, r14, r13, r12, rbp, rbx;
        uint64_t r9, r8, r10, rdx, rsi, rdi;
        uint64_t rax; // syscall number

        // CPU context saved by syscall instruction
        uint64_t user_rsp;
        uint64_t user_rflags; // r11
        uint64_t user_rip; // rcx
    };
};

} // Interrupts

#endif //BOREALOS_SYSCALL_H
