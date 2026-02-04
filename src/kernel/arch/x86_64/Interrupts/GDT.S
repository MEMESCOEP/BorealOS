/*     extern void LoadGDT(uint64_t gdt_pointer);
       extern void LoadTSS(uint16_t tss_selector); */
.section .text

.global LoadGDT
.type LoadGDT,@function
LoadGDT:
    /* Load the GDT using the LGDT instruction */
    lgdt (%rdi)

    /* Far jump to flush the instruction pipeline and load the new code segment */
    pushq $0x08
    leaq 1f(%rip), %rax
    pushq %rax
    lretq

1:
    /* Reload data segments with selector 0x10 (GDT entry 2) */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0, %ax
    movw %ax, %fs
    movw %ax, %gs
    ret

.global LoadTSS
.type LoadTSS,@function
LoadTSS:
    movw %di, %ax
    ltr %ax
    ret
