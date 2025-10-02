.text
.intel_syntax noprefix

.globl irq_handler
.globl exception_handler

.macro SAVE_REGS
    push    eax
    push    ebx
    push    ecx
    push    edx
    push    esi
    push    edi
    push    ebp


    mov     eax, cr2 /* store CR2 in eax */
    push    eax     /* push CR2 */
    mov     eax, cr3 /* store CR3 in eax */
    push    eax     /* push CR3 */
.endm

.macro RESTORE_REGS
    pop     eax     /* pop CR3 */
    mov     cr3, eax /* restore CR3 */
    pop     eax     /* pop CR2 */
    mov     cr2, eax /* restore CR2 */
    pop     ebp
    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     ebx
    pop     eax
.endm

.macro mac_IRQStub vector
.global stub_ISRStub_\vector
.type stub_ISRStub_\vector, @function
stub_ISRStub_\vector:
    SAVE_REGS

    /* push Register State */
    lea    eax, [esp] /* eax points to old esp, before SAVE_REGS */
    push   eax            /* push old esp, which is now the pointer to RegisterState */

    /* push argument (vector number) */
    push    \vector

    call    irq_handler

    add     esp, 8

    RESTORE_REGS
    iret
.endm

.macro mac_ISRErrStub vector
.global stub_ISRStub_\vector
.type stub_ISRStub_\vector, @function
stub_ISRStub_\vector:
    /* error code is already pushed by CPU */

    SAVE_REGS

    /* push Register State */
    lea    eax, [esp] /* eax points to old esp, before SAVE_REGS */
    push   eax            /* push old esp, which is now the pointer to RegisterState */

    /* push argument (vector number) */
    push    \vector


    call    exception_handler

    add     esp, 8

    RESTORE_REGS
    iret
.endm

.macro mac_ISRNoErrStub vector
.global stub_ISRStub_\vector
.type stub_ISRStub_\vector, @function
stub_ISRStub_\vector:
    SAVE_REGS

    /* push Register State */
    lea    eax, [esp] /* eax points to old esp, before SAVE_REGS */
    push   eax            /* push old esp, which is now the pointer to RegisterState */

    /* push argument (vector number) */
    push    \vector

    call    exception_handler
    add     esp, 8
    RESTORE_REGS
    iret
.endm

/* Define stubs 0–31 (exceptions) and 32–47 (IRQs) */
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

.section .rodata
.align 4
.global isr_stub_table
isr_stub_table:
    .long stub_ISRStub_0
    .long stub_ISRStub_1
    .long stub_ISRStub_2
    .long stub_ISRStub_3
    .long stub_ISRStub_4
    .long stub_ISRStub_5
    .long stub_ISRStub_6
    .long stub_ISRStub_7
    .long stub_ISRStub_8
    .long stub_ISRStub_9
    .long stub_ISRStub_10
    .long stub_ISRStub_11
    .long stub_ISRStub_12
    .long stub_ISRStub_13
    .long stub_ISRStub_14
    .long stub_ISRStub_15
    .long stub_ISRStub_16
    .long stub_ISRStub_17
    .long stub_ISRStub_18
    .long stub_ISRStub_19
    .long stub_ISRStub_20
    .long stub_ISRStub_21
    .long stub_ISRStub_22
    .long stub_ISRStub_23
    .long stub_ISRStub_24
    .long stub_ISRStub_25
    .long stub_ISRStub_26
    .long stub_ISRStub_27
    .long stub_ISRStub_28
    .long stub_ISRStub_29
    .long stub_ISRStub_30
    .long stub_ISRStub_31
    .long stub_ISRStub_32
    .long stub_ISRStub_33
    .long stub_ISRStub_34
    .long stub_ISRStub_35
    .long stub_ISRStub_36
    .long stub_ISRStub_37
    .long stub_ISRStub_38
    .long stub_ISRStub_39
    .long stub_ISRStub_40
    .long stub_ISRStub_41
    .long stub_ISRStub_42
    .long stub_ISRStub_43
    .long stub_ISRStub_44
    .long stub_ISRStub_45
    .long stub_ISRStub_46
    .long stub_ISRStub_47

.text
