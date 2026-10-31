#ifndef PAGING_H
#define PAGING_H

#include <kernel/boot.h>

/* Replaces the bootloader's page tables with 4 KiB mappings of the kernel and
   of physical memory. The tables' pages are removed from info->memory_regions. */
void paging_init(struct boot_info *info);

#endif
