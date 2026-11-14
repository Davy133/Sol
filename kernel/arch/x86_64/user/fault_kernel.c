/* Reading kernel memory must kill the process; surviving hangs the self-test. */
void _start(void)
{
    (void)*(volatile char *)0xffffffff80000000UL;
    for (;;);
}
