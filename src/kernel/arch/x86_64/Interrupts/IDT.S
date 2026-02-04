.section .text
.intel_syntax noprefix
/* IDT handling for x86_64 architecture */
.global IRQHandler
.global ExceptionHandler

/* Save and restore all general-purpose registers */
.macro SAVE_REGS
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    rsi
    push    rdi
    push    rbp
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15
.endm

.macro RESTORE_REGS
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rbp
    pop     rdi
    pop     rsi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax
.endm

.macro mac_IRQStub vector
.global stub_ISRStub_\vector
.type stub_ISRStub_\vector, @function
stub_ISRStub_\vector:
    SAVE_REGS
    mov rdi, \vector
    sub     rsp, 8                  /* Align stack to 16 bytes */
    call    IRQHandler              /* Call the IRQ handler */
    add     rsp, 8                 /* Restore stack */
    RESTORE_REGS
    iretq
.endm

.macro mac_ISRErrStub vector
.global stub_ISRStub_\vector
.type stub_ISRStub_\vector, @function
stub_ISRStub_\vector:
    SAVE_REGS
    /* get the error code from the stack */
    mov rdi, \vector
    mov    rsi, QWORD PTR [rsp + 8]   /* Second argument: error code */

    sub     rsp, 8                  /* Align stack to 16 bytes */
    call    ExceptionHandler        /* Call the exception handler */
    add     rsp, 8                 /* Restore stack */

    RESTORE_REGS
    iretq
.endm

.macro mac_ISRNoErrStub vector
.global stub_ISRStub_\vector
.type stub_ISRStub_\vector, @function
stub_ISRStub_\vector:
    SAVE_REGS
    /* No error code, push a dummy 0 onto the stack */
    mov rdi, \vector
    mov     rsi, 0                   /* Second argument: error code = 0 */

    sub     rsp, 8                  /* Align stack to 16 bytes */
    call    ExceptionHandler        /* Call the exception handler */
    add     rsp, 8                 /* Restore stack */
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