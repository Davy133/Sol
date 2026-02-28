extern main
MEMORY_ALIGN equ 1 << 0
MEM_INFO equ 1 << 1
FLAGS equ MEMORY_ALIGN | MEM_INFO
MAGIC equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
resb 32768
stack_top:

section .text
global _start:function (_start.end - _start)
_start:
    mov esp, stack_top
    call main
    cli
    .hang:	hlt
        jmp .hang
    .end: