#include <asm/gdt.h>
#include <asm/irq.h>
#include <asm/layout.h>
#include <asm/paging.h>
#include <kernel/elf.h>
#include <kernel/heap.h>
#include <kernel/keyboard.h>
#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <kernel/sched.h>
#include <stdio.h>
#include <string.h>

#define TASK_STACK_SIZE 16384
#define RFLAGS_IF       0x202

#define SYS_EXIT        0
#define SYS_WRITE       1
#define SYS_READ        2

/* A circular list; boot_task never exits, so it always has one task to run. */
struct task {
    registers_t regs;
    uint64_t cr3;
    uint64_t stack_top;
    bool dead;
    struct task *next;
};

static struct task boot_task = { .next = &boot_task };
static struct task *current = &boot_task;

/* sched_tick() frees the task once it runs on another task's stack. */
void task_exit(void)
{
    current->dead = true;

    asm volatile("sti" ::: "memory");
    for (;;)
        asm volatile("hlt");
}

static struct task *task_alloc(void)
{
    struct task *task = kmalloc(sizeof(*task));
    void *stack = kmalloc(TASK_STACK_SIZE);

    if (task == NULL || stack == NULL) {
        kfree(task);
        kfree(stack);
        return NULL;
    }

    memset(task, 0, sizeof(*task));
    task->stack_top = (uint64_t)stack + TASK_STACK_SIZE;
    task->regs.rflags = RFLAGS_IF;
    return task;
}

static void task_free(struct task *task)
{
    if (task->cr3 != 0)
        paging_destroy_space(task->cr3);

    kfree((void *)(task->stack_top - TASK_STACK_SIZE));
    kfree(task);
}

static void task_add(struct task *task)
{
    uint64_t flags = irq_save();
    task->next = boot_task.next;
    boot_task.next = task;
    irq_restore(flags);
}

bool task_create(void (*entry)(void))
{
    struct task *task = task_alloc();

    if (task == NULL)
        return false;

    uint64_t *top = (uint64_t *)task->stack_top;
    *--top = (uint64_t)task_exit;

    task->regs.rip = (uint64_t)entry;
    task->regs.cs = GDT_KERNEL_CODE << 3;
    task->regs.rsp = (uint64_t)top;
    task->regs.ss = GDT_KERNEL_DATA << 3;

    task_add(task);
    return true;
}

bool process_create(const void *elf, size_t size)
{
    struct task *task = task_alloc();

    if (task == NULL)
        return false;

    task->cr3 = paging_create_space();
    uint64_t entry = task->cr3 != 0 ? elf_load(task->cr3, elf, size) : 0;

    if (entry == 0
            || !paging_map_user(task->cr3, USER_STACK_TOP - PAGE_SIZE, PAGE_SIZE, NULL, 0, PAGE_WRITE | PAGE_NX)) {
        task_free(task);
        return false;
    }

    task->regs.rip = entry;
    task->regs.cs = GDT_USER_CODE << 3 | 3;
    task->regs.rsp = USER_STACK_TOP;
    task->regs.ss = GDT_USER_DATA << 3 | 3;

    task_add(task);
    return true;
}

/* Runs on the interrupted task's stack and address space, so that one is the
   only dead task it cannot free yet. */
void sched_tick(registers_t *regs)
{
    current->regs = *regs;

    struct task *next;
    while ((next = current->next)->dead && next != current) {
        current->next = next->next;
        task_free(next);
    }

    current = next;
    *regs = current->regs;
    paging_switch(current->cr3);
    if (current->stack_top != 0)
        tss_set_kernel_stack(current->stack_top);
}

void syscall_handler(registers_t *regs)
{
    switch (regs->rax) {
    case SYS_EXIT:
        task_exit();

    case SYS_WRITE: {
        /* Interrupts are off in here, so a long write would stall everything. */
        uint64_t length = regs->rsi < PAGE_SIZE ? regs->rsi : PAGE_SIZE;

        if (!paging_user_accessible(regs->rdi, length, 0)) {
            regs->rax = -1;
            return;
        }

        for (uint64_t i = 0; i < length; i++)
            putchar(((const char *)regs->rdi)[i]);
        regs->rax = length;
        return;
    }

    case SYS_READ: {
        /* Blocks for one key, then also takes the keys already typed. */
        char *buffer = (char *)regs->rdi;
        uint64_t length = regs->rsi < PAGE_SIZE ? regs->rsi : PAGE_SIZE;
        uint64_t count = 0;
        int c;

        if (!paging_user_accessible(regs->rdi, length, PAGE_WRITE)) {
            regs->rax = -1;
            return;
        }
        if (length == 0) {
            regs->rax = 0;
            return;
        }

        /* sti takes effect once hlt has started, so a key arriving after the
           check still wakes it.
           shortcut: halts through its time slices; use a wait queue once tasks can block. */
        while ((c = keyboard_getchar()) < 0)
            asm volatile("sti; hlt; cli" ::: "memory");

        do
            buffer[count++] = (char)c;
        while (count < length && (c = keyboard_getchar()) >= 0);

        regs->rax = count;
        return;
    }

    default:
        regs->rax = -1;
    }
}

void sched_wait(void)
{
    while (boot_task.next != &boot_task)
        asm volatile("hlt" ::: "memory");
}

extern const char user_hello_start[], user_hello_end[];
extern const char user_fault_kernel_start[], user_fault_kernel_end[];
extern const char user_fault_code_start[], user_fault_code_end[];

#define PROGRAM(name) user_##name##_start, (uintptr_t)user_##name##_end - (uintptr_t)user_##name##_start

static volatile bool test_ran;

static void test_task(void)
{
    test_ran = true;
}

static void run_test_tasks(void)
{
    test_ran = false;

    if (!task_create(test_task)
            || !process_create(PROGRAM(hello))
            || !process_create(PROGRAM(fault_kernel))
            || !process_create(PROGRAM(fault_code)))
        panic("scheduler self-test: could not start a task");

    /* 0x2010 cuts hello's third segment short after two are mapped, so the
       leak check also covers a load that fails halfway. */
    if (process_create(user_hello_start, sizeof(Elf64_Ehdr) - 1)
            || process_create(user_hello_start, 0x2010))
        panic("scheduler self-test: a truncated ELF was loaded");

    sched_wait();
    if (!test_ran)
        panic("scheduler self-test: a kernel task never ran");
}

/* The first round may create page tables, which stay for reuse; the second must not. */
void sched_selftest(void)
{
    run_test_tasks();
    uint64_t pages = pmm_free_page_count();

    run_test_tasks();
    if (pmm_free_page_count() != pages)
        panic("scheduler self-test: finished tasks leaked memory");
}
