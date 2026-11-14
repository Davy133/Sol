#ifndef LAYOUT_H
#define LAYOUT_H

/*
 * Virtual address space; the upper half is the kernel's, the same in every process.
 *
 * 0x0000000000000000 - 0x00007fffffffffff  user: ELF segments from 0x1000,
 *                                           USER_STACK_SIZE of stack below USER_STACK_TOP
 * 0xffff800000000000 - 0xffffbfffffffffff  physical memory (boot_info.physmap_offset)
 * 0xffffc00000000000 - 0xffffc07fffffffff  kernel heap
 * 0xffffffff80000000 - __kernel_end        kernel image
 *
 * The heap fills exactly one PML4 entry, created at boot and never changed, so a
 * process copying the upper half of the PML4 sees every later heap mapping.
 */

#define USER_STACK_TOP  0x800000ULL
#define USER_STACK_SIZE 0x4000ULL
#define USER_END        0x0000800000000000ULL

#define HEAP_START      0xffffc00000000000ULL
#define HEAP_END        0xffffc08000000000ULL

#endif
