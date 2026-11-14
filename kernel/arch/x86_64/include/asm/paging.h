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

void paging_init(struct boot_info *info);

bool paging_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
uint64_t paging_unmap_page(uint64_t virt);

uint64_t paging_create_space(void);
bool paging_map_user(uint64_t space, uint64_t virt, size_t size, const void* data, size_t data_size, uint64_t flags);
bool paging_user_accessible(uint64_t virt, uint64_t size, uint64_t flags);
/* space must not be the loaded one. */
void paging_destroy_space(uint64_t space);
/* 0 is the kernel's own space. */
void paging_switch(uint64_t space);

#endif
