#include "syscall.h"

#define KERNEL_ADDRESS 0xffffffff80000000L

static const char message[] = "Hello from user mode\n";

void _start(void)
{
    /* The kernel must refuse to print its own memory; if it does not, hang so the self-test fails. */
    if (syscall2(SYS_WRITE, KERNEL_ADDRESS, 1) != -1)
        for (;;);

    syscall2(SYS_WRITE, (long)message, sizeof(message) - 1);
    syscall2(SYS_EXIT, 0, 0);
    for (;;);
}
