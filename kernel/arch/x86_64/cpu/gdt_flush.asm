default rel

global gdt_flush
global tss_flush

; void gdt_flush(gdt_ptr_t *gdtp);
; Argument passed in rdi
gdt_flush:
    lgdt [rdi]

    ; Long mode has no far jump to an immediate, so reload CS (kernel code
    ; selector 0x08) with a far return instead.
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

; void tss_flush(void);
; Carrega o seletor da TSS (GDT_TSS=5, RPL=0) no registrador TR
tss_flush:
    mov ax, 0x28        ; GDT_TSS << 3 = 5 * 8 = 0x28
    ltr ax
    ret
