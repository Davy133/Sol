global gdt_flush
global tss_flush

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

; void tss_flush(void);
; Carrega o seletor da TSS (GDT_TSS=5, RPL=0) no registrador TR
tss_flush:
    mov ax, 0x28        ; GDT_TSS << 3 = 5 * 8 = 0x28
    ltr ax
    ret
