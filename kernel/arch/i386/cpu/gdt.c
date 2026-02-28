#include "gdt.h"
#include <string.h>

static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t gdtp;
static tss_entry_t tss;

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_mid = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

void gdt_init(void)
{
    for (int i = 0; i < GDT_ENTRIES; i++)
        gdt_set_gate(i, 0, 0, 0, 0);

    gdt_set_gate(GDT_KERNEL_CODE, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    gdt_set_gate(GDT_KERNEL_DATA, 0, 0xFFFFFFFF, 0x92, 0xCF);

    gdt_set_gate(GDT_USER_CODE, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    gdt_set_gate(GDT_USER_DATA, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    memset(&tss, 0, sizeof(tss_entry_t));

    tss.ss0 = GDT_KERNEL_DATA << 3;
    tss.iomap_base = sizeof(tss_entry_t);

    gdt_set_gate(GDT_TSS, (uint32_t)&tss, sizeof(tss_entry_t) - 1, 0x89, 0x00);

    gdtp.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdtp.base = (uint32_t)&gdt;

    gdt_flush(&gdtp);
    tss_flush();
}

void tss_set_kernel_stack(uint32_t stack)
{
    tss.esp0 = stack;
}
