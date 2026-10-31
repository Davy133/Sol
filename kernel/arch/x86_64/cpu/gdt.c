#include "gdt.h"
#include <string.h>

static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t gdtp;
static tss_entry_t tss;
static uint8_t kernel_stack[TSS_STACK_SIZE] __attribute__((aligned(16)));
static uint8_t double_fault_stack[TSS_STACK_SIZE] __attribute__((aligned(16)));

void gdt_set_gate(uint32_t num, uint64_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_mid = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].flags = (limit >> 16) & 0x0F;

    gdt[num].flags |= gran & 0xF0;
    gdt[num].access = access;
}

void gdt_init(void)
{
    for (uint32_t i = 0; i < GDT_ENTRIES; i++)
        gdt_set_gate(i, 0, 0, 0, 0);

    gdt_set_gate(GDT_KERNEL_CODE, 0, 0xFFFFFFFF, 0x9A, 0xAF);

    gdt_set_gate(GDT_KERNEL_DATA, 0, 0xFFFFFFFF, 0x92, 0xCF);

    gdt_set_gate(GDT_USER_DATA, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    gdt_set_gate(GDT_USER_CODE, 0, 0xFFFFFFFF, 0xFA, 0xAF);

    memset(&tss, 0, sizeof(tss_entry_t));

    tss.iomap_base = sizeof(tss_entry_t);

    /* Stacks grow down, so each one starts at the end of its buffer. */
    tss_set_kernel_stack((uint64_t)kernel_stack + sizeof(kernel_stack));
    tss.ist[TSS_IST_DOUBLE_FAULT - 1] = (uint64_t)double_fault_stack + sizeof(double_fault_stack);

    uint64_t tss_base = (uint64_t)&tss;
    gdt_set_gate(GDT_TSS, tss_base, sizeof(tss_entry_t) - 1, 0x89, 0x00);

    gdt[GDT_TSS_HIGH].limit_low = (tss_base >> 32) & 0xFFFF;
    gdt[GDT_TSS_HIGH].base_low = (tss_base >> 48) & 0xFFFF;

    gdtp.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdtp.base = (uint64_t)&gdt;

    gdt_flush(&gdtp);
    tss_flush();
}

void tss_set_kernel_stack(uint64_t stack)
{
    tss.rsp0 = stack;
}
