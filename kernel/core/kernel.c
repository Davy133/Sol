#include <stdio.h>
#include <stdint.h>
#include <kernel/tty.h>
#include "../arch/i386/cpu/idt.h"

static volatile uint32_t ticks = 0;

static void timer_handler(registers_t *regs) {
    (void)regs;
    ticks++;
    if (ticks % 18 == 0)          /* ~1 segundo */
        printf("tick: %d s\n", ticks / 18);
}

void kernel_main(void) {
	terminal_initialize();
	printf("Hello, kernel World!\n");
  printf("Testando quebra de linha:\n");
  printf("Linha 1\nLinha 2\nLinha 3\n");
    
  printf("\nTestando o Tab:\n");
  printf("Coluna 1\tColuna 2\tColuna 3\n");
    
  printf("\nTestando o Carriage Return:\n");
  printf("Isso vai sumir\rIsso vai ficar\n");
}
