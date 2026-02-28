#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define IDT_ENTRIES 256

#define IDT_GATE_TYPE(x)  ((x) & 0x0F)
#define IDT_PRES(x)       ((x) << 0x07)
#define IDT_PRIV(x)      (((x) & 0x03) << 0x05)

#define IDT_INTERRUPT_GATE  (IDT_PRES(1) | IDT_PRIV(0) | IDT_GATE_TYPE(0xE))
#define IDT_TRAP_GATE       (IDT_PRES(1) | IDT_PRIV(0) | IDT_GATE_TYPE(0xF))
#define IDT_USER_INT_GATE   (IDT_PRES(1) | IDT_PRIV(3) | IDT_GATE_TYPE(0xE))

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  reserved;
    uint8_t  flags;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed)) registers_t;

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);
void isr_handler(registers_t *regs);

extern void idt_flush(idt_ptr_t *idtp);

extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);

#endif
