; Declare constants for the 32-bit paging
%define PG_ENABLED_CR0  (1 << 31)   ; Enable paging (bit 31 in CR0)
%define PSE_ENABLED_CR4 (1 << 4)    ; Enable 4MB pages via PSE (bit 4 in CR4)

; Page Directory Entry flags for a 4MB identity-mapped page
%define PDE_PRESENT     (1 << 0)    ; Page is present
%define PDE_RW          (1 << 1)    ; Read/write
%define PDE_PS          (1 << 7)    ; Page size = 4MB (requires PSE in CR4)
%define PDE_FLAGS       (PDE_PRESENT | PDE_RW | PDE_PS)

section .bss
align 4096
page_directory:
    resd 1024   ; 1024 entries * 4 bytes = 4KB page directory

section .text

global enable_paging

enable_paging:
    ; --- Build identity-mapped page directory using 4MB pages ---
    ; Each entry covers 4MB, so 1024 entries map the full 4GB address space.
    mov ecx, 1024
    mov edi, page_directory
    mov eax, PDE_FLAGS          ; First entry: physical address 0x0 + flags
.map_loop:
    mov [edi], eax
    add eax, 0x400000           ; Advance to next 4MB boundary
    add edi, 4
    loop .map_loop

    ; --- Load CR3 with the page directory address ---
    mov eax, page_directory
    mov cr3, eax

    ; --- Enable PSE (4MB pages) in CR4 ---
    mov eax, cr4
    or eax, PSE_ENABLED_CR4
    mov cr4, eax

    ; --- Enable paging in CR0 ---
    mov eax, cr0
    or eax, PG_ENABLED_CR0
    mov cr0, eax

    ; NOTE: Do NOT re-enable interrupts here (sti).
    ; The IDT has not been set up yet. Interrupts will be
    ; enabled by the kernel after the IDT is initialized.
    ret

