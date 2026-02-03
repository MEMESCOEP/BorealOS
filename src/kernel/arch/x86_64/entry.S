/* Entry point of this kernel for x86_64 architecture, this is done to load the stack from our linker (_kernel_stack_top) and call the main kernel function */
.section .text
.global main
.extern kmain
.type main,@function
main:
    /* Load the stack pointer */
    lea _kernel_stack_top(%rip), %rsp

    /* Call the main kernel function */
    call kmain

    /* Halt the CPU if kernel_main returns */
    cli
.halt:
    hlt
    jmp .halt