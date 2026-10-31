#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#define GDT_ENTRIES 7

#define TSS_STACK_SIZE 16384

/* Value for the IDT gate's ist field; IST numbers start at 1. */
#define TSS_IST_DOUBLE_FAULT 1

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  flags;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

/* User data comes before user code because sysret derives both selectors
   from a single base in that order. The TSS descriptor takes two entries. */
enum gdt_selectors {
    GDT_NULL        = 0,
    GDT_KERNEL_CODE = 1,
    GDT_KERNEL_DATA = 2,
    GDT_USER_DATA   = 3,
    GDT_USER_CODE   = 4,
    GDT_TSS         = 5,
    GDT_TSS_HIGH    = 6,
};
typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed)) tss_entry_t;

void gdt_init(void);
void gdt_set_gate(uint32_t num, uint64_t base, uint32_t limit, uint8_t access, uint8_t gran);

void tss_set_kernel_stack(uint64_t stack);

extern void gdt_flush(gdt_ptr_t *gdtp);
extern void tss_flush(void);

#endif
