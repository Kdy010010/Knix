ENTRY(_start)

SECTIONS
{
    . = 0x8000;

    .multiboot_header ALIGN(8) : {
            *(.multiboot_header)
    }

    .text : {
        *(.text.start)
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(COMMON)
        *(.bss)
    }
}