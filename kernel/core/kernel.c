#include <stdio.h>

#include <kernel/tty.h>

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
