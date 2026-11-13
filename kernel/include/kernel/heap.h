#ifndef _KERNEL_HEAP_H
#define _KERNEL_HEAP_H

#include <stddef.h>

/* Kernel heap. Memory is 16-byte aligned and not cleared. Works from the first
   call, once paging_init() is done. Not safe to call from interrupt handlers yet. */

/* Returns NULL when size is 0 or memory runs out. */
void* kmalloc(size_t size);
/* Ignores NULL. Panics on anything kmalloc() did not return or already got back. */
void kfree(void* pointer);

#endif
