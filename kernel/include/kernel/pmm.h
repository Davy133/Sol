#ifndef _KERNEL_PMM_H
#define _KERNEL_PMM_H

#include <kernel/boot.h>

#define PAGE_SIZE 4096ULL

void pmm_init(const struct boot_info* info);

/* 0 when out of memory. */
uint64_t pmm_alloc_page(void);
void pmm_free_page(uint64_t address);

uint64_t pmm_free_page_count(void);

#endif
