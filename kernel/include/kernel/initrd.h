#ifndef _KERNEL_INITRD_H
#define _KERNEL_INITRD_H

#include <stdbool.h>
#include <stddef.h>

/* The initrd is a ustar archive the bootloader loaded; it must stay mapped. */
void initrd_init(const void* archive, size_t size);

/* Finds the regular file at path, written as in the archive ("bin/sh"). */
bool initrd_find(const char* path, const void** data, size_t* size);

/* Writes the path of the index-th regular file into name; false past the last
   file or when name is too small. */
bool initrd_name(size_t index, char* name, size_t size);

#endif
