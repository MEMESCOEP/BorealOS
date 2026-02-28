ENTRY(_start)

SECTIONS {
    . = ALIGN(4096);
    .text : { *(.text) }
    .data : { *(.data) }
    .bss : { *(.bss) }
    .rodata : { *(.rodata) }
    .module : { *(.module) }
    .module_reliances : { *(.module_reliances) }

    /DISCARD/ : { *(.comment) *(.note*) }
}