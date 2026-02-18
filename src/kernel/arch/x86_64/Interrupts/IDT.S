.section .text
.intel_syntax noprefix
/* IDT handling for x86_64 architecture */
.global IRQHandler
.global ExceptionHandler

/* Save all general-purpose registers in reverse order (r15 … rax) */
.macro SAVE_REGS
    push    r15
    push    r14
    push    r13
    push    r12
    push    r11
    push    r10
    push    r9
    push    r8
    push    rbp
    push    rdi
    push    rsi
    push    rdx
    push    rcx
    push    rbx
    push    rax
.endm

/* Restore registers in the opposite order (rax … r15) */
.macro RESTORE_REGS
    pop     rax
    pop     rbx
    pop     rcx
    pop     rdx
    pop     rsi
    pop     rdi
    pop     rbp
    pop     r8
    pop     r9
    pop     r10
    pop     r11
    pop     r12
    pop     r13
    pop     r14
    pop     r15
.endm

/* IRQ stub (no error code) */
.macro mac_IRQStub vector
.global stub_ISRStub_\vector
.type stub_ISRStub_\vector, @function
stub_ISRStub_\vector:
    SAVE_REGS
    mov     rdi, \vector          /* first argument: vector number */
    xor     rsi, rsi              /* second argument: error code = 0 */
    mov     rdx, rsp              /* third argument: pointer to saved registers */
    /* Align stack to 16 bytes and reserve space for call's return address */
    mov     rbp, rsp              /* save original rsp after SAVE_REGS */
    and     rsp, -16              /* align to 16 bytes */
    sub     rsp, 8                /* now rsp ≡ 8 (mod 16) for the upcoming call */
    call    IRQHandler
    mov     rsp, rbp              /* restore original rsp */
    RESTORE_REGS
    iretq
.endm

/* Exception stub with CPU‑pushed error code */
.macro mac_ISRErrStub vector
.global stub_ISRStub_\vector
.type stub_ISRStub_\vector, @function
stub_ISRStub_\vector:
    SAVE_REGS
    mov     rdi, \vector
    mov     rsi, QWORD PTR [rsp + 15*8]   /* error code is 15 registers above */
    mov     rdx, rsp
    mov     rbp, rsp
    and     rsp, -16
    sub     rsp, 8
    call    ExceptionHandler
    mov     rsp, rbp
    RESTORE_REGS
    add     rsp, 8                /* discard the error code */
    iretq
.endm

/* Exception stub without error code */
.macro mac_ISRNoErrStub vector
.global stub_ISRStub_\vector
.type stub_ISRStub_\vector, @function
stub_ISRStub_\vector:
    SAVE_REGS
    mov     rdi, \vector
    xor     rsi, rsi              /* error code = 0 */
    mov     rdx, rsp
    mov     rbp, rsp
    and     rsp, -16
    sub     rsp, 8
    call    ExceptionHandler
    mov     rsp, rbp
    RESTORE_REGS
    iretq
.endm

/* Define ISR and IRQ stubs */
mac_ISRNoErrStub 0
mac_ISRNoErrStub 1
mac_ISRNoErrStub 2
mac_ISRNoErrStub 3
mac_ISRNoErrStub 4
mac_ISRNoErrStub 5
mac_ISRNoErrStub 6
mac_ISRNoErrStub 7
mac_ISRErrStub 8
mac_ISRNoErrStub 9
mac_ISRErrStub 10
mac_ISRErrStub 11
mac_ISRErrStub 12
mac_ISRErrStub 13
mac_ISRErrStub 14
mac_ISRNoErrStub 15
mac_ISRNoErrStub 16
mac_ISRErrStub 17
mac_ISRNoErrStub 18
mac_ISRNoErrStub 19
mac_ISRNoErrStub 20
mac_ISRNoErrStub 21
mac_ISRNoErrStub 22
mac_ISRNoErrStub 23
mac_ISRNoErrStub 24
mac_ISRNoErrStub 25
mac_ISRNoErrStub 26
mac_ISRNoErrStub 27
mac_ISRNoErrStub 28
mac_ISRNoErrStub 29
mac_ISRErrStub 30
mac_ISRNoErrStub 31
mac_IRQStub 32
mac_IRQStub 33
mac_IRQStub 34
mac_IRQStub 35
mac_IRQStub 36
mac_IRQStub 37
mac_IRQStub 38
mac_IRQStub 39
mac_IRQStub 40
mac_IRQStub 41
mac_IRQStub 42
mac_IRQStub 43
mac_IRQStub 44
mac_IRQStub 45
mac_IRQStub 46
mac_IRQStub 47

/* Now we need an ISR stub table to reference these stubs */
.section .rodata
.align 8
.global ISRStubTable
ISRStubTable:
    .quad    stub_ISRStub_0
    .quad    stub_ISRStub_1
    .quad    stub_ISRStub_2
    .quad    stub_ISRStub_3
    .quad    stub_ISRStub_4
    .quad    stub_ISRStub_5
    .quad    stub_ISRStub_6
    .quad    stub_ISRStub_7
    .quad    stub_ISRStub_8
    .quad    stub_ISRStub_9
    .quad    stub_ISRStub_10
    .quad    stub_ISRStub_11
    .quad    stub_ISRStub_12
    .quad    stub_ISRStub_13
    .quad    stub_ISRStub_14
    .quad    stub_ISRStub_15
    .quad    stub_ISRStub_16
    .quad    stub_ISRStub_17
    .quad    stub_ISRStub_18
    .quad    stub_ISRStub_19
    .quad    stub_ISRStub_20
    .quad    stub_ISRStub_21
    .quad    stub_ISRStub_22
    .quad    stub_ISRStub_23
    .quad    stub_ISRStub_24
    .quad    stub_ISRStub_25
    .quad    stub_ISRStub_26
    .quad    stub_ISRStub_27
    .quad    stub_ISRStub_28
    .quad    stub_ISRStub_29
    .quad    stub_ISRStub_30
    .quad    stub_ISRStub_31
    .quad    stub_ISRStub_32
    .quad    stub_ISRStub_33
    .quad    stub_ISRStub_34
    .quad    stub_ISRStub_35
    .quad    stub_ISRStub_36
    .quad    stub_ISRStub_37
    .quad    stub_ISRStub_38
    .quad    stub_ISRStub_39
    .quad    stub_ISRStub_40
    .quad    stub_ISRStub_41
    .quad    stub_ISRStub_42
    .quad    stub_ISRStub_43
    .quad    stub_ISRStub_44
    .quad    stub_ISRStub_45
    .quad    stub_ISRStub_46
    .quad    stub_ISRStub_47

/* End of IDT.S */