/* argc is at the stack pointer, argv above it. */
.global _start
_start:
    mov (%rsp), %rdi
    lea 8(%rsp), %rsi
    call main
    mov %eax, %edi
    mov $0, %eax
    int $0x80
    ud2

.section .note.GNU-stack, "", @progbits
