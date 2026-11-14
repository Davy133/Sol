/* Must be killed, or the self-test hangs. */
int main(void)
{
    *(volatile char *)(unsigned long)main = 0;
    for (;;);
}
