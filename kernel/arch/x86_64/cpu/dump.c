#include <asm/dump.h>

#define VECTOR_PAGE_FAULT   14
#define PF_PRESENT          (1 << 0)
#define PF_WRITE            (1 << 1)
#define PF_USER             (1 << 2)
#define PF_FETCH            (1 << 4)

#define TRACE_DEPTH         10
#define HIGHER_HALF_START   0xFFFF800000000000

static void put_control_registers(void)
{
    uint64_t cr0, cr2, cr3, cr4;

    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %%cr4, %0" : "=r"(cr4));

    panic_put_register("CR0", cr0);
    panic_put_register("CR2", cr2);
    panic_put_char('\n');
    panic_put_register("CR3", cr3);
    panic_put_register("CR4", cr4);
    panic_put_char('\n');
}

void arch_stack_trace(uint64_t rbp)
{
    panic_put_string("\nStack trace:\n");

    for (int i = 0; i < TRACE_DEPTH; i++)
    {
        if (rbp < HIGHER_HALF_START || (rbp & 7))
            break;

        uint64_t *frame = (uint64_t *)rbp;
        if (frame[1] == 0)
            break;

        panic_put_string("  ");
        panic_put_hex(frame[1]);
        panic_put_char('\n');

        if (frame[0] <= rbp)
            break;
        rbp = frame[0];
    }
}

void arch_dump_registers(uint64_t pc)
{
    uint64_t rflags, rsp;
    asm volatile("pushfq; pop %0" : "=r"(rflags));
    asm volatile("mov %%rsp, %0" : "=r"(rsp));

    panic_put_register("RIP", pc);
    panic_put_register("RSP", rsp);
    panic_put_char('\n');
    panic_put_register("RFL", rflags);
    panic_put_char('\n');
    put_control_registers();
}

static void put_page_fault(uint64_t err_code)
{
    panic_put_string("Access: ");
    panic_put_string(err_code & PF_FETCH ? "instruction fetch" :
                     err_code & PF_WRITE ? "write" : "read");
    panic_put_string(err_code & PF_PRESENT ? ", protection violation" : ", page not present");
    panic_put_string(err_code & PF_USER ? ", user mode\n" : ", kernel mode\n");
}

void panic_exception(const registers_t *regs, const char *name)
{
    panic_begin();

    panic_put_string("Reason: ");
    panic_put_string(name);
    panic_put_string(" (vector ");
    panic_put_decimal(regs->int_no);
    panic_put_string(", error code ");
    panic_put_hex(regs->err_code);
    panic_put_string(")\n");

    if (regs->int_no == VECTOR_PAGE_FAULT)
        put_page_fault(regs->err_code);
    panic_put_char('\n');

    panic_put_register("RIP", regs->rip);
    panic_put_register("CS ", regs->cs);
    panic_put_register("RFL", regs->rflags);
    panic_put_char('\n');
    panic_put_register("RSP", regs->rsp);
    panic_put_register("SS ", regs->ss);
    panic_put_register("RBP", regs->rbp);
    panic_put_char('\n');
    panic_put_register("RAX", regs->rax);
    panic_put_register("RBX", regs->rbx);
    panic_put_register("RCX", regs->rcx);
    panic_put_char('\n');
    panic_put_register("RDX", regs->rdx);
    panic_put_register("RSI", regs->rsi);
    panic_put_register("RDI", regs->rdi);
    panic_put_char('\n');
    panic_put_register("R8 ", regs->r8);
    panic_put_register("R9 ", regs->r9);
    panic_put_register("R10", regs->r10);
    panic_put_char('\n');
    panic_put_register("R11", regs->r11);
    panic_put_register("R12", regs->r12);
    panic_put_register("R13", regs->r13);
    panic_put_char('\n');
    panic_put_register("R14", regs->r14);
    panic_put_register("R15", regs->r15);
    panic_put_char('\n');
    put_control_registers();

    panic_end(regs->rbp);
}
