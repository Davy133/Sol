#include <asm/paging.h>
#include <kernel/heap.h>
#include <kernel/initrd.h>
#include <kernel/keyboard.h>
#include <kernel/pmm.h>
#include <kernel/sched.h>
#include <stdio.h>
#include <string.h>

/* The number is in rax and the arguments in rdi, rsi and rdx; the result goes
   back in rax, with -1 for any error. */
#define SYS_EXIT    0
#define SYS_WRITE   1   /* (fd, buffer, length) -> bytes written */
#define SYS_READ    2   /* (fd, buffer, length) -> bytes read, 0 at the end of a file */
#define SYS_OPEN    3   /* (path) -> fd */
#define SYS_CLOSE   4   /* (fd) -> 0 */
#define SYS_SPAWN   5   /* (path, argv) -> process id */
#define SYS_WAIT    6   /* (process id) -> 0 once it has exited */
#define SYS_LIST    7   /* (index, buffer, length) -> length of the index-th initrd path */

/* 0 to 2 are the console; opened files start after them. */
#define FD_STDIN    0
#define FD_STDOUT   1
#define FD_STDERR   2
#define FD_FIRST    3

#define PATH_MAX    256

/* sti takes effect once hlt has started, so an interrupt arriving after the
   caller's check still wakes it.
   shortcut: halts through its time slices; use a wait queue once tasks can block. */
static void wait_for_interrupt(void)
{
    asm volatile("sti; hlt; cli" ::: "memory");
}

/* Copies a NUL-terminated user string, checking each page before reading it. */
static bool copy_string(uint64_t user, char *buffer, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        if ((i == 0 || (user + i) % PAGE_SIZE == 0) && !paging_user_accessible(user + i, 1, 0))
            return false;
        buffer[i] = ((const char *)user)[i];
        if (buffer[i] == '\0')
            return true;
    }
    return false;
}

static struct file *open_file(uint64_t fd)
{
    if (fd < FD_FIRST || fd - FD_FIRST >= PROCESS_MAX_FILES)
        return NULL;

    struct file *file = &sched_files()[fd - FD_FIRST];
    return file->data != NULL ? file : NULL;
}

static int64_t sys_write(uint64_t fd, uint64_t buffer, uint64_t length)
{
    /* Interrupts are off in here, so a long write would stall everything. */
    if (length > PAGE_SIZE)
        length = PAGE_SIZE;
    if ((fd != FD_STDOUT && fd != FD_STDERR) || !paging_user_accessible(buffer, length, 0))
        return -1;

    for (uint64_t i = 0; i < length; i++)
        putchar(((const char *)buffer)[i]);
    return (int64_t)length;
}

/* Blocks for one key, then also takes the keys already typed. */
static int64_t read_keyboard(char *buffer, uint64_t length)
{
    uint64_t count = 0;
    int c;

    while ((c = keyboard_getchar()) < 0)
        wait_for_interrupt();

    do
        buffer[count++] = (char)c;
    while (count < length && (c = keyboard_getchar()) >= 0);
    return (int64_t)count;
}

static int64_t sys_read(uint64_t fd, uint64_t buffer, uint64_t length)
{
    if (length > PAGE_SIZE)
        length = PAGE_SIZE;
    if (!paging_user_accessible(buffer, length, PAGE_WRITE))
        return -1;
    if (length == 0)
        return 0;
    if (fd == FD_STDIN)
        return read_keyboard((char *)buffer, length);

    struct file *file = open_file(fd);

    if (file == NULL)
        return -1;
    if (length > file->size - file->offset)
        length = file->size - file->offset;
    memcpy((char *)buffer, file->data + file->offset, length);
    file->offset += length;
    return (int64_t)length;
}

static int64_t sys_open(uint64_t user_path)
{
    char path[PATH_MAX];
    const void *data;
    size_t size;

    if (!copy_string(user_path, path, sizeof(path)) || !initrd_find(path, &data, &size))
        return -1;

    struct file *files = sched_files();

    for (int64_t i = 0; i < PROCESS_MAX_FILES; i++) {
        if (files[i].data == NULL) {
            files[i] = (struct file){ .data = data, .size = size, .offset = 0 };
            return FD_FIRST + i;
        }
    }
    return -1;
}

static int64_t sys_close(uint64_t fd)
{
    struct file *file = open_file(fd);

    if (file == NULL)
        return -1;
    file->data = NULL;
    return 0;
}

/* The strings go into one page, which also bounds what map_arguments() gets. */
static int64_t sys_spawn(uint64_t user_path, uint64_t user_argv)
{
    char path[PATH_MAX];
    const char *argv[PROCESS_MAX_ARGS + 1];
    size_t argc = 0;
    size_t used = 0;
    int64_t id = -1;

    if (!copy_string(user_path, path, sizeof(path)))
        return -1;

    char *strings = kmalloc(PAGE_SIZE);
    if (strings == NULL)
        return -1;

    for (; user_argv != 0; argc++) {
        uint64_t entry = user_argv + argc * sizeof(uint64_t);

        if (!paging_user_accessible(entry, sizeof(uint64_t), 0))
            goto out;

        uint64_t string = *(const uint64_t *)entry;

        if (string == 0)
            break;
        if (argc == PROCESS_MAX_ARGS || !copy_string(string, strings + used, PAGE_SIZE - used))
            goto out;
        argv[argc] = strings + used;
        used += strlen(strings + used) + 1;
    }
    argv[argc] = NULL;

    uint64_t spawned = process_spawn(path, argv);
    if (spawned != 0)
        id = (int64_t)spawned;
out:
    kfree(strings);
    return id;
}

static int64_t sys_wait(uint64_t id)
{
    /* The boot task (0) never exits, and nothing could wake a task waiting on itself. */
    if (id == 0 || id == sched_current_id())
        return -1;

    while (sched_task_running(id))
        wait_for_interrupt();
    return 0;
}

static int64_t sys_list(uint64_t index, uint64_t buffer, uint64_t length)
{
    char name[PATH_MAX];

    if (!initrd_name(index, name, sizeof(name)))
        return -1;

    size_t size = strlen(name) + 1;

    if (size > length || !paging_user_accessible(buffer, size, PAGE_WRITE))
        return -1;
    memcpy((char *)buffer, name, size);
    return (int64_t)size - 1;
}

void syscall_handler(registers_t *regs)
{
    int64_t result;

    switch (regs->rax) {
    case SYS_EXIT:
        task_exit();
    case SYS_WRITE:
        result = sys_write(regs->rdi, regs->rsi, regs->rdx);
        break;
    case SYS_READ:
        result = sys_read(regs->rdi, regs->rsi, regs->rdx);
        break;
    case SYS_OPEN:
        result = sys_open(regs->rdi);
        break;
    case SYS_CLOSE:
        result = sys_close(regs->rdi);
        break;
    case SYS_SPAWN:
        result = sys_spawn(regs->rdi, regs->rsi);
        break;
    case SYS_WAIT:
        result = sys_wait(regs->rdi);
        break;
    case SYS_LIST:
        result = sys_list(regs->rdi, regs->rsi, regs->rdx);
        break;
    default:
        result = -1;
    }
    regs->rax = (uint64_t)result;
}
