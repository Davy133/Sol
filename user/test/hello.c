#include "../sol.h"

#define KERNEL_ADDRESS ((void *)0xffffffff80000000UL)

int main(void);

/* The kernel must refuse each of these; if it does not, hang so the self-test fails. */
static int kernel_refuses_bad_pointers(void)
{
    return sys_write(STDOUT, KERNEL_ADDRESS, 1) == -1
        && sys_read(STDIN, (void *)(unsigned long)main, 1) == -1
        && sys_open(KERNEL_ADDRESS) == -1
        && sys_spawn(KERNEL_ADDRESS, 0) == -1
        && sys_spawn("bin/echo", KERNEL_ADDRESS) == -1
        && sys_list(0, KERNEL_ADDRESS, 256) == -1
        && sys_wait(0) == -1;
}

int main(void)
{
    if (!kernel_refuses_bad_pointers())
        for (;;);

    print("Hello from user mode\n");
    return 0;
}
