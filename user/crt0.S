/* The kernel starts a process here with argc at the stack pointer and the
   argv pointers above it; main's return value becomes the exit status. */
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
