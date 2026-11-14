/* Must be killed, or the self-test hangs. */
int main(void)
{
    (void)*(volatile char *)0xffffffff80000000UL;
    for (;;);
}
