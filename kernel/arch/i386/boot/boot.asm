; Declare constants for the multiboot header.
%define ALIGN    (1 << 0)             ; align loaded modules on page boundaries
%define MEMINFO  (1 << 1)             ; provide memory map
%define FLAGS    (ALIGN | MEMINFO)    ; this is the Multiboot 'flag' field
%define MAGIC    0x1BADB002           ; 'magic number' lets bootloader find the header
%define CHECKSUM (-(MAGIC + FLAGS))   ; checksum of above, to prove we are multiboot


; Declare a header as in the Multiboot Standard.
section .multiboot
align 4
dd MAGIC
dd FLAGS
dd CHECKSUM


; Reserve a stack for the initial thread.
section .bss
alignb 16
stack_bottom:
    resb 16384 ; 16 KiB
stack_top:

; The kernel entry point.
extern _init
extern enable_paging
extern gdt_init
extern idt_init
extern kernel_main

section .text
global _start
_start:
    mov esp, stack_top

    ; Initialize the GDT before enabling paging.
    call gdt_init

    ; Enable paging (32-bit paging with 4MB pages).
    call enable_paging

    call idt_init

    ; Call the global constructors.
    call _init

    ; Transfer control to the main kernel.
    call kernel_main

    ; Hang if kernel_main unexpectedly returns.
    cli
hang:
    hlt
    jmp hang
