MAGIC equ 0x1badb002
FLAGS equ (1 << 0 | 1 << 1)
CHECKSUM equ -(MAGIC + FLAGS)





section .multiboot
    dd MAGIC
    dd FLAGS
    dd CHECKSUM


section .text
extern kernel_main

global loader
loader:
    cli
    mov esp, kernel_stack
    push eax
    push ebx
    call kernel_main

global _stop
_stop:
    hlt
    jmp _stop

section .bss
resb 2*1024* 1024
kernel_stack:

