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

void gdt_init(void);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

//vai vir do assembly
extern void gdt_flush(gdt_ptr_t *gdtp);

#endif
