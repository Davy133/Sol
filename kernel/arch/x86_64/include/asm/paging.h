#ifndef PAGING_H
#define PAGING_H

#include <kernel/boot.h>

#define PAGE_PRESENT    (1ULL << 0)
#define PAGE_WRITE      (1ULL << 1)
#define PAGE_USER       (1ULL << 2)
#define PAGE_PWT        (1ULL << 3)
#define PAGE_PAT        (1ULL << 7)     /* In a 4 KiB entry */
#define PAGE_NX         (1ULL << 63)

/* Replaces the bootloader's page tables with 4 KiB mappings of the kernel and
   of physical memory, then starts the physical allocator. The tables' pages are
   removed from info->memory_regions. */
void paging_init(struct boot_info *info);

/* Both work on the kernel's address space and take page-aligned addresses.
   PAGE_NX in flags is dropped on CPUs without it. */
void paging_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void paging_unmap_page(uint64_t virt);

#endif
