/* Reading kernel memory must kill the process; surviving hangs the self-test. */
int main(void)
{
    (void)*(volatile char *)0xffffffff80000000UL;
    for (;;);
}
