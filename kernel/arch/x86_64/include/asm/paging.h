#ifndef PAGING_H
#define PAGING_H

#include <stdbool.h>
#include <kernel/boot.h>

#define PAGE_PRESENT    (1ULL << 0)
#define PAGE_WRITE      (1ULL << 1)
#define PAGE_USER       (1ULL << 2)
#define PAGE_PWT        (1ULL << 3)
#define PAGE_PAT        (1ULL << 7)     /* In a 4 KiB entry */
#define PAGE_NX         (1ULL << 63)

/* Replaces the bootloader's page tables with the kernel's own and starts the
   physical allocator. The tables' pages are removed from info->memory_regions. */
void paging_init(struct boot_info *info);

/* Both work on the kernel's address space and take page-aligned addresses.
   PAGE_NX in flags is dropped on CPUs without it. Mapping fails when a page
   table cannot be allocated; unmapping returns the page it removed, or 0. */
bool paging_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
uint64_t paging_unmap_page(uint64_t virt);

/* A new address space sharing the kernel half; 0 when memory runs out. */
uint64_t paging_create_space(void);
/* Maps fresh zeroed pages holding a copy of data, which may be NULL. */
bool paging_map_user(uint64_t space, uint64_t virt, const void* data, size_t size, uint64_t flags);
/* Whether the current address space lets user mode reach every byte of the range. */
bool paging_user_accessible(uint64_t virt, uint64_t size);
/* Frees the space with every user page and table in it. It must not be loaded. */
void paging_destroy_space(uint64_t space);
/* 0 is the kernel's own space. */
void paging_switch(uint64_t space);

#endif
