.extern syscall_kernel_rsp
.extern KernelSyscallHandler

.section .data
user_rsp_scratch:
    .quad 0

.section .text
.global SyscallHandler
.type SyscallHandler,@function
SyscallHandler:
    // Save user RSP and switch to kernel stack
    mov %rsp, user_rsp_scratch(%rip)
    mov syscall_kernel_rsp(%rip), %rsp

    // Construct frame: RIP, RFLAGS, RSP, then GPRs
    push %rcx                   // User RIP
    push %r11                   // User RFLAGS
    push user_rsp_scratch(%rip) // User RSP
    push %rax
    push %rdi
    push %rsi
    push %rdx
    push %r10
    push %r8
    push %r9
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, %rdi
    call KernelSyscallHandler

    // Restore GPRs
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx
    pop %r9
    pop %r8
    pop %r10
    pop %rdx
    pop %rsi
    pop %rdi
    pop %rax    // Popped syscall number (RAX)

    // Restore context without switching RSP prematurely
    pop %rdx    // Pop User RSP into a scratch register (RDX)
    pop %r11    // Pop User RFLAGS into R11 (required for sysretq)
    pop %rcx    // Pop User RIP into RCX (required for sysretq)

    // Final switch to user stack and return
    mov %rdx, %rsp
    sysretq

//     extern void EnterUserspace(uint64_t entryPoint, uint64_t userStack);
.global EnterUserspace
.type EnterUserspace,@function
EnterUserspace:
    // RDI = entry point, RSI = user stack
    cli
    push $0x33 // SS selector (GDT entry 3)
    push %rsi // User stack pointer
    pushfq
    pop %rax
    or $0x200, %rax // Set IF in RFLAGS to enable interrupts in userspace
    push %rax
    push $0x3B // CS selector (GDT entry 4)
    push %rdi // User instruction pointer
    mov $0, %rax
    iretq