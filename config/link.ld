ENTRY(_Start)
SECTIONS
{
    . = 1M;

    .multiboot : {
        KEEP(*(.multiboot))
    }

    . = 2M;
    _KernelStartMarker = .;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    _KernelEndMarker = .;
}