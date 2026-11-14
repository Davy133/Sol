#ifndef SOL_H
#define SOL_H

/* System calls; numbers and meaning match kernel/core/syscall.c. */
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_OPEN    3
#define SYS_CLOSE   4
#define SYS_SPAWN   5
#define SYS_WAIT    6
#define SYS_LIST    7

#define STDIN   0
#define STDOUT  1
#define STDERR  2

static inline long syscall3(long number, long a, long b, long c)
{
    long result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "D"(a), "S"(b), "d"(c) : "memory");
    return result;
}

__attribute__((noreturn))
static inline void sys_exit(int status)
{
    syscall3(SYS_EXIT, status, 0, 0);
    __builtin_unreachable();
}

static inline long sys_write(int fd, const void *buffer, unsigned long length)
{
    return syscall3(SYS_WRITE, fd, (long)buffer, (long)length);
}

static inline long sys_read(int fd, void *buffer, unsigned long length)
{
    return syscall3(SYS_READ, fd, (long)buffer, (long)length);
}

static inline int sys_open(const char *path)
{
    return (int)syscall3(SYS_OPEN, (long)path, 0, 0);
}

static inline int sys_close(int fd)
{
    return (int)syscall3(SYS_CLOSE, fd, 0, 0);
}

/* argv ends with a NULL entry. Returns the process id, or -1. */
static inline long sys_spawn(const char *path, char *const argv[])
{
    return syscall3(SYS_SPAWN, (long)path, (long)argv, 0);
}

static inline long sys_wait(long id)
{
    return syscall3(SYS_WAIT, id, 0, 0);
}

/* Writes the index-th initrd path into buffer; -1 past the last one. */
static inline long sys_list(long index, char *buffer, unsigned long length)
{
    return syscall3(SYS_LIST, index, (long)buffer, (long)length);
}

static inline unsigned long string_length(const char *s)
{
    unsigned long n = 0;

    while (s[n] != '\0')
        n++;
    return n;
}

static inline void print_to(int fd, const char *s)
{
    sys_write(fd, s, string_length(s));
}

static inline void print(const char *s)
{
    print_to(STDOUT, s);
}

static inline int equal(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

/* What follows prefix in s, or 0 when s does not start with it. */
static inline const char *skip_prefix(const char *s, const char *prefix)
{
    while (*prefix != '\0')
        if (*s++ != *prefix++)
            return 0;
    return s;
}

#endif
