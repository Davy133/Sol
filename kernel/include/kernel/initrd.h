#ifndef _KERNEL_INITRD_H
#define _KERNEL_INITRD_H

#include <stdbool.h>
#include <stddef.h>

void initrd_init(const void* archive, size_t size);

bool initrd_find(const char* path, const void** data, size_t* size);

bool initrd_name(size_t index, char* name, size_t size);

#endif
