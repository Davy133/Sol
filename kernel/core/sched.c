#include <asm/gdt.h>
#include <asm/irq.h>
#include <asm/layout.h>
#include <asm/paging.h>
#include <kernel/elf.h>
#include <kernel/heap.h>
#include <kernel/initrd.h>
#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <kernel/sched.h>
#include <string.h>

#define TASK_STACK_SIZE 16384
#define RFLAGS_IF       0x202

/* Circular list; boot_task never exits. */
struct task {
    uint64_t id;
    registers_t regs;
    uint64_t cr3;
    uint64_t stack_top;
    bool dead;
    struct file files[PROCESS_MAX_FILES];
    struct task *next;
};

static struct task boot_task = { .next = &boot_task };
static struct task *current = &boot_task;
static uint64_t next_id = 1;

/* sched_tick() frees it later, from another stack. */
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
    task->id = next_id++;
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

/* System V stack: argc, argv, NULL, empty envp, AT_NULL auxv. */
static uint64_t map_arguments(uint64_t space, const char *const *argv)
{
    char *page = kmalloc(PAGE_SIZE);
    uint64_t base = USER_STACK_TOP - PAGE_SIZE;
    uint64_t pointers[PROCESS_MAX_ARGS];
    size_t top = PAGE_SIZE;
    size_t argc = 0;

    if (page == NULL)
        return 0;
    memset(page, 0, PAGE_SIZE);

    for (; argv != NULL && argv[argc] != NULL; argc++) {
        size_t length = strlen(argv[argc]) + 1;

        if (argc == PROCESS_MAX_ARGS || length >= top) {
            kfree(page);
            return 0;
        }
        top -= length;
        memcpy(page + top, argv[argc], length);
        pointers[argc] = base + top;
    }

    size_t words = argc + 5;

    if (words * sizeof(uint64_t) + 16 > top) {
        kfree(page);
        return 0;
    }

    size_t sp = (top - words * sizeof(uint64_t)) & ~(size_t)15;
    uint64_t *stack = (uint64_t *)(page + sp);

    stack[0] = argc;
    for (size_t i = 0; i < argc; i++)
        stack[1 + i] = pointers[i];

    bool mapped = paging_map_user(space, base, PAGE_SIZE, page, PAGE_SIZE, PAGE_WRITE | PAGE_NX);
    kfree(page);
    return mapped ? base + sp : 0;
}

uint64_t process_create(const void *elf, size_t size, const char *const *argv)
{
    struct task *task = task_alloc();

    if (task == NULL)
        return 0;

    task->cr3 = paging_create_space();
    uint64_t entry = task->cr3 != 0 ? elf_load(task->cr3, elf, size) : 0;
    uint64_t stack = 0;

    if (entry != 0 && paging_map_user(task->cr3, USER_STACK_TOP - USER_STACK_SIZE,
                                      USER_STACK_SIZE - PAGE_SIZE, NULL, 0, PAGE_WRITE | PAGE_NX))
        stack = map_arguments(task->cr3, argv);
    if (stack == 0) {
        task_free(task);
        return 0;
    }

    task->regs.rip = entry;
    task->regs.cs = GDT_USER_CODE << 3 | 3;
    task->regs.rsp = stack;
    task->regs.ss = GDT_USER_DATA << 3 | 3;

    uint64_t id = task->id;
    task_add(task);
    return id;
}

uint64_t process_spawn(const char *path, const char *const *argv)
{
    const void *elf;
    size_t size;

    return initrd_find(path, &elf, &size) ? process_create(elf, size, argv) : 0;
}

/* Runs on the current task's stack, so it can't free that one. */
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

void sched_wait(void)
{
    while (boot_task.next != &boot_task)
        asm volatile("hlt" ::: "memory");
}

bool sched_task_running(uint64_t id)
{
    uint64_t flags = irq_save();
    struct task *task = &boot_task;
    bool running = false;

    do {
        if (task->id == id && !task->dead)
            running = true;
        task = task->next;
    } while (task != &boot_task);

    irq_restore(flags);
    return running;
}

uint64_t sched_current_id(void)
{
    return current->id;
}

struct file *sched_files(void)
{
    return current->files;
}

static volatile bool test_ran;

static void test_task(void)
{
    test_ran = true;
}

static void run_test_tasks(void)
{
    const void *hello;
    size_t size;

    test_ran = false;

    if (!task_create(test_task)
            || !process_spawn("test/hello", NULL)
            || !process_spawn("test/fault_kernel", NULL)
            || !process_spawn("test/fault_code", NULL)
            || !process_spawn("bin/echo", (const char *const[]){ "echo", "argv", "reached", "echo", NULL })
            || !process_spawn("bin/ls", (const char *const[]){ "ls", "test", NULL }))
        panic("scheduler self-test: could not start a task");

    if (process_spawn("test/missing", NULL) || process_spawn("test/hell", NULL) || process_spawn("test", NULL))
        panic("scheduler self-test: started a file not in the initrd");

    /* 0x2010 cuts hello off mid-load, so leaks on partial loads show up. */
    if (!initrd_find("test/hello", &hello, &size) || size <= 0x2010)
        panic("scheduler self-test: test/hello is missing or too small");
    if (process_create(hello, sizeof(Elf64_Ehdr) - 1, NULL)
            || process_create(hello, 0x2010, NULL))
        panic("scheduler self-test: a truncated ELF was loaded");

    sched_wait();
    if (!test_ran)
        panic("scheduler self-test: a kernel task never ran");
}

/* The first round may leave page tables for reuse. */
void sched_selftest(void)
{
    run_test_tasks();
    uint64_t pages = pmm_free_page_count();

    run_test_tasks();
    if (pmm_free_page_count() != pages)
        panic("scheduler self-test: finished tasks leaked memory");
}
