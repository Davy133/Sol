#ifndef _KERNEL_PMM_H
#define _KERNEL_PMM_H

#include <kernel/boot.h>

#define PAGE_SIZE 4096ULL

/* Physical page allocator. Addresses are physical; reach them through
   physmap_offset. Not safe to call from interrupt handlers yet. */
void pmm_init(const struct boot_info* info);

/* Returns 0 when memory runs out; page 0 is never handed out. */
uint64_t pmm_alloc_page(void);
void pmm_free_page(uint64_t address);

uint64_t pmm_free_page_count(void);

#endif
