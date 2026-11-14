#ifndef _KERNEL_HEAP_H
#define _KERNEL_HEAP_H

#include <stddef.h>

/* Memory is 16-byte aligned and not cleared. Needs paging_init(). Runs with
   interrupts off, so tasks can share it. */

/* Returns NULL when size is 0 or memory runs out. */
void* kmalloc(size_t size);
/* Ignores NULL. Panics on anything kmalloc() did not return or already got back. */
void kfree(void* pointer);

/* Panics when the heap or the page allocator misbehaves. */
void heap_selftest(void);

#endif
