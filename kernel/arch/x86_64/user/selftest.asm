default rel

section .rodata

global user_hello_start, user_hello_end
global user_fault_start, user_fault_end

; Copied to USER_CODE, so it must only use rip-relative addressing.
user_hello_start:
    mov rax, 1
    mov rdi, 0xffffffff80000000
    mov rsi, 1
    int 0x80
    cmp rax, -1
    jne $

    mov rax, 1
    lea rdi, [message]
    mov rsi, message_length
    int 0x80

    mov rax, 0
    int 0x80
message:
    db "Hello from user mode", 10
message_length equ $ - message
user_hello_end:

user_fault_start:
    mov rbx, 0xffffffff80000000
    mov rax, [rbx]
    jmp $
user_fault_end:
