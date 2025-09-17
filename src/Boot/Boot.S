# Multiboot2 header and kernel setup for x86 (32-bit)
.code32

.section .multiboot, "a"
.align 8

multiboot2_header:
    .long 0xe85250d6                # Multiboot2 magic number
    .long 0                         # Architecture: i386
    .long multiboot2_header_end - multiboot2_header # Header length
    .long 0x100000000 - (0xe85250d6 + 0 + (multiboot2_header_end - multiboot2_header)) # Checksum

    # Framebuffer request tag
    .word 5                         # Tag type 5 (framebuffer)
    .word 0                         # Flags
    .long 24                        # Tag size
    .long 0                         # Width (any)
    .long 0                         # Height (any)
    .long 32                        # Depth (32 bpp)
    .word 0                         # Padding
    .word 0                         # Padding

    .word 6                         # Tag type 6 (memory map)
    .word 0                         # Flags
    .word 8                         # Size

    # End tag
    .word 0                         # Tag type 0 (end)
    .word 0                         # Flags
    .long 8                         # Size

    .zero 4                         # Padding to align to 8 bytes
multiboot2_header_end:

.section .bss
.align 16
StackBottom:
    .space 16384                    # 16 KB stack
StackTop:

.section .rodata
.align 4
tbl_GDT:
mkr_GDTNull:
    .long 0
    .long 0
mkr_GDTCode:
    .word 0xFFFF                    # Limit low
    .word 0x0000                    # Base low
    .byte 0                         # Base middle
    .byte 0x9A                      # Access byte (code segment)
    .byte 0xCF                      # Flags (4K granularity, 32-bit)
    .byte 0                         # Base high
mkr_GDTData:
    .word 0xFFFF                    # Limit low
    .word 0x0000                    # Base low
    .byte 0                         # Base middle
    .byte 0x92                      # Access byte (data segment)
    .byte 0xCF                      # Flags (4K granularity, 32-bit)
    .byte 0                         # Base high
mkr_GDTEnd:
ptr_GDTP:
    .word mkr_GDTEnd - tbl_GDT - 1  # GDT size
    .long tbl_GDT                   # GDT base address

.section .text
.global _Start
.extern EntryPoint

_Start:
    # Set up stack pointer
    movl $StackTop, %esp
    movl %esp, %ebp

    # Preserve registers and load GDT
    pushal
    cli
    call _LoadGDT
    popal

    # Push Multiboot2 info (magic and pointer)
    pushl %ebx
    pushl %eax

    # Enter kernel main
    call EntryPoint

.hang:
    cli
    hlt
    jmp .hang

_LoadGDT:
    lgdt (ptr_GDTP)
    ljmp $0x08, $.flushSegments     # Far jump to code segment

.flushSegments:
    movw $0x10, %ax                 # Load data segment selector
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    ret