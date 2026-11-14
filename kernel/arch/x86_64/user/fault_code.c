/* Code is mapped read-only, so writing to it must kill the process; surviving
   hangs the self-test. */
void _start(void)
{
    *(volatile char *)(unsigned long)_start = 0;
    for (;;);
}
