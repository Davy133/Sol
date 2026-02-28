#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#define GDT_ENTRIES 6

typedef struct {
    uint16_t limit_low;     // Bits 0-15 do limit
    uint16_t base_low;      // Bits 0-15 da base
    uint8_t  base_mid;      // Bits 16-23 da base
    uint8_t  access;        // Access byte (P, DPL, S, Type)
    uint8_t  granularity;   // Flags + limit high (G, D/B, AVL + limit 16-19)
    uint8_t  base_high;     // Bits 24-31 da base
} __attribute__((packed)) gdt_entry_t;


typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

enum gdt_selectors {
    GDT_NULL        = 0,
    GDT_KERNEL_CODE = 1,
    GDT_KERNEL_DATA = 2,
    GDT_USER_CODE   = 3,
    GDT_USER_DATA   = 4,
    GDT_TSS         = 5,
};
typedef struct {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed)) tss_entry_t;

void gdt_init(void);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

void tss_set_kernel_stack(uint32_t stack);

extern void gdt_flush(gdt_ptr_t *gdtp);
extern void tss_flush(void);

#endif
