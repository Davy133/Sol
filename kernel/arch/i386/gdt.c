#include "gdt.h"

static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t gdtp;

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
    {
        gdt_set_gate(i, 0, 0, 0, 0);
    }

    gdt_set_gate(GDT_NULL, 0, 0, 0, 0); // isso aqui é obrigatorio

    gdt_set_gate(GDT_KERNEL_CODE, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    gdt_set_gate(GDT_KERNEL_DATA, 0, 0xFFFFFFFF, 0x92, 0xCF);

    gdt_set_gate(GDT_USER_CODE, 0, 0xFFFFFFFF,0xFA, 0xCF);

    gdt_set_gate(GDT_USER_DATA, 0, 0xFFFFFFFF,0xF2, 0xCF);

    //tem que ver essa TSS depois, deixa assim por enquanto
    gdt_set_gate(GDT_TSS, 0, 0x68, 0x89,0x00);

    gdtp.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdtp.base = (uint32_t)&gdt;

    gdt_flush(&gdtp);
}