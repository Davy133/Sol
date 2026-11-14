/* Code is mapped read-only, so writing to it must kill the process; surviving
   hangs the self-test. */
int main(void)
{
    *(volatile char *)(unsigned long)main = 0;
    for (;;);
}
