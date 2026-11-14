default rel

global gdt_flush
global tss_flush

gdt_flush:
    lgdt [rdi]

    ; Long mode has no far jump to an immediate.
    push qword 0x08
    lea rax, [.reload]
    push rax
    retfq

.reload:
    mov ax, 0x10            ; kernel data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

tss_flush:
    mov ax, 0x28        ; GDT_TSS << 3
    ltr ax
    ret
