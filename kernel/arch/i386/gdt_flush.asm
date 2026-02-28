global gdt_flush

; void gdt_flush(gdt_ptr_t *gdtp);
; Argument passed on the stack at [esp+4]
gdt_flush:
    mov eax, [esp+4]        ; pointer to gdt_ptr_t
    lgdt [eax]

    ; Far jump para recarregar CS (kernel code selector 0x08)
    jmp 0x08:.reload

.reload:
    mov ax, 0x10            ; kernel data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret
