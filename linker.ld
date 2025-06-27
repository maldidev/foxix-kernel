ENTRY(_start)

SECTIONS {
    . = 1M;

    /* Ensure Multiboot header is at very beginning */
    .boot : {
        *(.multiboot)
    }

    .text : ALIGN(4K) {
        *(.text)
    }

    .data : ALIGN(4K) {
        *(.data)
    }

    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}
