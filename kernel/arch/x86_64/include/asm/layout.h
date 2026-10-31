#ifndef LAYOUT_H
#define LAYOUT_H

/*
 * Virtual address space. Each process gets the lower half; the upper half is the
 * kernel's and looks the same in all of them.
 *
 * 0x0000000000000000 - 0x00007fffffffffff  user
 * 0xffff800000000000 - 0xffffbfffffffffff  physical memory (boot_info.physmap_offset)
 * 0xffffc00000000000 - 0xffffc07fffffffff  kernel heap
 * 0xffffffff80000000 - __kernel_end        kernel image
 *
 * The heap fills exactly one PML4 entry, created at boot and never changed, so a
 * process copying the upper half of the PML4 sees every later heap mapping.
 */

#define HEAP_START      0xffffc00000000000ULL
#define HEAP_END        0xffffc08000000000ULL

#endif
