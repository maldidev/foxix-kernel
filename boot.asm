bits 32
section .multiboot
    align 4
    dd 0x1BADB002              ; Magic
    dd 0x00000003              ; Flags
    dd -(0x1BADB002 + 0x03)    ; Checksum

section .text
global _start
extern foxix_main

_start:
    mov esp, stack_top
    call foxix_main
    hlt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
