#ifndef LAYOUT_H
#define LAYOUT_H

/*
 * 0x0000000000001000 -                     user ELF segments, stack below USER_STACK_TOP
 * 0xffff800000000000 - 0xffffbfffffffffff  physical memory
 * 0xffffc00000000000 - 0xffffc07fffffffff  kernel heap, one PML4 entry shared by every process
 * 0xffffffff80000000 - __kernel_end        kernel image
 */

#define USER_STACK_TOP  0x800000ULL
#define USER_STACK_SIZE 0x4000ULL
#define USER_END        0x0000800000000000ULL

#define HEAP_START      0xffffc00000000000ULL
#define HEAP_END        0xffffc08000000000ULL

#endif
