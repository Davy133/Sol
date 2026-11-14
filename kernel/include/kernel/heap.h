#ifndef _KERNEL_HEAP_H
#define _KERNEL_HEAP_H

#include <stddef.h>

void* kmalloc(size_t size);
void kfree(void* pointer);

void heap_selftest(void);

#endif
