%macro pushaq 0
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popaq 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
%endmacro

extern CPUExceptionHandler
extern IRQHandler

%macro isr_irq_stub 1
isr_stub_%+%1:
    push 0                ; push fake error code
    push %1         ; IRQ number
    pushaq
    mov rdi, rsp
    call IRQHandler       ; Call your custom IRQ handler
    popaq
    add rsp, 16
    iretq                 ; Return from interrupt (restore context)
%endmacro

%macro isr_err_stub 1
isr_stub_%+%1:
    push %1         ; IRQ number
    pushaq
    mov rdi, rsp
    call CPUExceptionHandler   ; Call exception handler for errors
    popaq
    add rsp, 16
    iretq
%endmacro

%macro isr_no_err_stub 1
isr_stub_%+%1:
    push 0                     ; push fake error code
    push %1                    ; IRQ number
    pushaq
    mov rdi, rsp
    call CPUExceptionHandler   ; Call exception handler for errors
    popaq
    add rsp, 16
    iretq
%endmacro

; IRQ 1 should use the custom handler (IRQHandler)
isr_irq_stub    33   ; IRQ 1 (keyboard) will use a custom handler
isr_irq_stub    44

; Exception ISRs
isr_no_err_stub 0    ; Division By Zero Exception
isr_no_err_stub 1    ; Debug Exception
isr_no_err_stub 2    ; Non Maskable Interrupt (NMI)
isr_no_err_stub 3    ; Breakpoint Exception
isr_no_err_stub 4    ; Overflow Exception
isr_no_err_stub 5    ; Out of Bounds Exception
isr_no_err_stub 6    ; Invalid Opcode Exception
isr_no_err_stub 7    ; Device not available Exception
isr_err_stub    8    ; Double Fault Exception
isr_no_err_stub 9    ; Coprocessor Segment Overrun Exception
isr_err_stub    10   ; Bad TSS Exception
isr_err_stub    11   ; Segment Not Present Exception
isr_err_stub    12   ; Stack Fault Exception
isr_err_stub    13   ; General Protection Fault Exception
isr_err_stub    14   ; Page Fault Exception
isr_no_err_stub 15   ; Reserved
isr_no_err_stub 16   ; x86 floating point Exception
isr_err_stub    17   ; Alignment Check Exception (486+)
isr_no_err_stub 18   ; Machine Check Exception (Pentium/586+)
isr_no_err_stub 19   ; SIMD floating point
isr_no_err_stub 20   ; Virtualization
isr_err_stub    21   ; Control protection
isr_no_err_stub 22   ; Reserved
isr_no_err_stub 23   ; Reserved
isr_no_err_stub 24   ; Reserved
isr_no_err_stub 25   ; Reserved
isr_no_err_stub 26   ; Reserved
isr_no_err_stub 27   ; Reserved
isr_no_err_stub 28   ; Hypervisor injection
isr_err_stub    29   ; VMM communication
isr_err_stub    30   ; Security
isr_no_err_stub 31   ; Reserved Exception
isr_no_err_stub 32
;isr_no_err_stub 33
isr_no_err_stub 34
isr_no_err_stub 35
isr_no_err_stub 36
isr_no_err_stub 37
isr_no_err_stub 38
isr_no_err_stub 39
isr_no_err_stub 40
isr_no_err_stub 41
isr_no_err_stub 42
isr_no_err_stub 43
;isr_no_err_stub 44
isr_no_err_stub 45
isr_no_err_stub 46
isr_no_err_stub 47
isr_no_err_stub 48

; Define interrupt vector table
global ISRStubTable
ISRStubTable:
%assign i 0 
%rep 48
    dq isr_stub_%+i    ; Interrupt Service Routine for each interrupt vector
%assign i i+1
%endrep