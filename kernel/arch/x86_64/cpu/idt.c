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

void idt_set_gate(uint8_t num, uint64_t base, uint16_t selector, uint8_t flags)
{
    idt[num].offset_low  = base & 0xFFFF;
    idt[num].offset_mid  = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector    = selector;
    idt[num].ist         = 0;
    idt[num].flags       = flags;
    idt[num].reserved    = 0;
}

void idt_init(void)
{
    idtp.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idtp.base  = (uint64_t)&idt;

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
        idt_set_gate(i, (uint64_t)isr_stubs[i], GDT_KERNEL_CODE << 3, flags);
    }

    /* Remap PIC and register IRQ gates (32–47) */
    pic_init();

    static void (*irq_stubs[16])(void) = {
        irq0,  irq1,  irq2,  irq3,  irq4,  irq5,  irq6,  irq7,
        irq8,  irq9,  irq10, irq11, irq12, irq13, irq14, irq15,
    };

    for (int i = 0; i < 16; i++)
        idt_set_gate(32 + i, (uint64_t)irq_stubs[i], GDT_KERNEL_CODE << 3, IDT_INTERRUPT_GATE);

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
        printf("EXCEPTION: %s (int %ld, err %ld)\n",
               exception_messages[regs->int_no],
               regs->int_no,
               regs->err_code);

        printf("RAX: %016lx RBX: %016lx RCX: %016lx\n",
               regs->rax, regs->rbx, regs->rcx);
        printf("RDX: %016lx RSI: %016lx RDI: %016lx\n",
               regs->rdx, regs->rsi, regs->rdi);
        printf("RBP: %016lx RSP: %016lx R8:  %016lx\n",
               regs->rbp, regs->rsp, regs->r8);
        printf("R9:  %016lx R10: %016lx R11: %016lx\n",
               regs->r9, regs->r10, regs->r11);
        printf("R12: %016lx R13: %016lx R14: %016lx\n",
               regs->r12, regs->r13, regs->r14);
        printf("R15: %016lx\n", regs->r15);
        printf("RIP: %016lx CS: %016lx RFLAGS: %016lx\n",
               regs->rip, regs->cs, regs->rflags);
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
