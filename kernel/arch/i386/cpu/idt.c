#include "idt.h"
#include "gdt.h"
#include "pic.h"
#include <stdio.h>

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idtp;

static irq_handler_t irq_handlers[16] = { 0 };

static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
};

void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags)
{
    idt[num].offset_low  = base & 0xFFFF;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
    idt[num].selector    = selector;
    idt[num].reserved    = 0;
    idt[num].flags       = flags;
}

void idt_init(void)
{
    idtp.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idtp.base  = (uint32_t)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++)
        idt_set_gate(i, 0, 0, 0);

    static void (*isr_stubs[32])(void) = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
    };

    for (int i = 0; i < 32; i++)
    {
        uint8_t flags = IDT_INTERRUPT_GATE;
        idt_set_gate(i, (uint32_t)isr_stubs[i], GDT_KERNEL_CODE << 3, flags);
    }

    /* Remap PIC and register IRQ gates (32–47) */
    pic_init();

    static void (*irq_stubs[16])(void) = {
        irq0,  irq1,  irq2,  irq3,  irq4,  irq5,  irq6,  irq7,
        irq8,  irq9,  irq10, irq11, irq12, irq13, irq14, irq15,
    };

    for (int i = 0; i < 16; i++)
        idt_set_gate(32 + i, (uint32_t)irq_stubs[i], GDT_KERNEL_CODE << 3, IDT_INTERRUPT_GATE);

    idt_flush(&idtp);
}

void irq_register_handler(uint8_t irq, irq_handler_t handler) {
    if (irq < 16)
        irq_handlers[irq] = handler;
}

void isr_handler(registers_t *regs)
{
    if (regs->int_no < 32)
    {
        printf("EXCEPTION: %s (int %d, err %d)\n",
               exception_messages[regs->int_no],
               regs->int_no,
               regs->err_code);

        printf("EAX: %08x EBX: %08x ECX: %08x EDX: %08x\n",
               regs->eax, regs->ebx, regs->ecx, regs->edx);
        printf("ESI: %08x EDI: %08x EBP: %08x ESP: %08x\n",
               regs->esi, regs->edi, regs->ebp, regs->esp);
        printf("EIP: %08x CS: %08x EFLAGS: %08x\n",
               regs->eip, regs->cs, regs->eflags);
        for (;;);
    }
}

void irq_handler(registers_t *regs)
{
    uint8_t irq = regs->int_no - 32;

    if (irq_handlers[irq])
        irq_handlers[irq](regs);

    pic_send_eoi(irq);
}
