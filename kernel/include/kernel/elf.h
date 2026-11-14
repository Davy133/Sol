#ifndef _KERNEL_ELF_H
#define _KERNEL_ELF_H

#include <stddef.h>
#include <stdint.h>

#define EI_NIDENT       16
#define ELFCLASS64      2
#define ELFDATA2LSB     1
#define EV_CURRENT      1
#define ET_EXEC         2
#define EM_X86_64       62

#define PT_LOAD         1
#define PT_INTERP       3

#define PF_X            1
#define PF_W            2

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

/* Returns the entry point, or 0; pages mapped before a failure stay in space. */
uint64_t elf_load(uint64_t space, const void* file, size_t size);

#endif
