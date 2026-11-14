#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2

static inline long syscall2(long number, long a, long b)
{
    long result;
    asm volatile("int $0x80" : "=a"(result) : "a"(number), "D"(a), "S"(b) : "memory");
    return result;
}

#endif
