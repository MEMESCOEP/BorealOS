; Multiboot 2 header for 32-bit i386 kernel
BITS 32

section .multiboot
align 8

multiboot2_header:
    dd 0xe85250d6                ; Multiboot2 magic
    dd 0                         ; Architecture: 0 = i386
    dd multiboot2_header_end - multiboot2_header ; Header length
    dd 0x100000000 - (0xe85250d6 + 0 + (multiboot2_header_end - multiboot2_header)) ; Checksum

    ; Optional: framebuffer request tag
    dw 5                         ; Tag type 5 = framebuffer
    dw 0                         ; Flags (0 = optional)
    dd 24                        ; Size of this tag (padded from 20 to 24)
    dd 0                         ; Width = 0 = any
    dd 0                         ; Height = 0 = any
    dd 32                        ; Depth (bits per pixel)
    dw 0                         ; Padding 2 bytes
    dw 0                         ; Padding 2 bytes

    dw 6                         ; Tag type 6 = memory map
    dw 0                         ; Flags (0 = optional)
    dw 8

    ; End tag (required)
    dw 0                         ; Tag type 0 = end
    dw 0                         ; Flags
    dd 8                         ; Size of end tag

    ; Padding to align header length to multiple of 8
    db 0,0,0,0

multiboot2_header_end:
; ----------------------------------------------------------

section .bss
align 16
StackBottom:
    resb 16384                        ; 16 KB stack
StackTop:

; ----------------------------------------------------------

section .rodata
align 4
tbl_GDT:
mkr_GDTNull:
    dd 0, 0
mkr_GDTCode:
    dw 0xFFFF                         ; Limit low
    dw 0x0000                         ; Base low
    db 0                              ; Base middle
    db 0x9A                           ; Access byte
    db 0xCF                           ; Flags (4K, 32-bit)
    db 0                              ; Base high
mkr_GDTData:
    dw 0xFFFF
    dw 0x0000
    db 0
    db 0x92
    db 0xCF
    db 0
mkr_GDTEnd:
ptr_GDTP:
    dw mkr_GDTEnd - mkr_GDTNull - 1  ; GDT size
    dd mkr_GDTNull                   ; GDT base address

; ----------------------------------------------------------

section .text
global _Start
extern KernelStart

_Start:
    ; Set up the stack
    mov esp, StackTop
    mov ebp, esp

    pushad
    cli
    call _LoadGDT
    popad

    ; Multiboot2 passes: eax = magic, ebx = pointer to struct
    push ebx
    push eax

    call KernelStart

.hang:
    cli
    hlt
    jmp .hang

_LoadGDT:
    lgdt [ptr_GDTP]
    jmp 0x08:.flushSegments
    
.flushSegments:
    mov eax, 0x10
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax
    ret