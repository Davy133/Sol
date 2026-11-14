#include <asm/layout.h>
#include <asm/paging.h>
#include <kernel/elf.h>
#include <kernel/log.h>
#include <kernel/pmm.h>
#include <stdbool.h>
#include <string.h>

/* Segments live in [PAGE_SIZE, USER_STACK_BOTTOM); page 0 stays unmapped so
   null pointers fault. */
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_SIZE)

static bool header_valid(const Elf64_Ehdr *header, size_t size)
{
    return memcmp(header->e_ident, "\x7f" "ELF", 4) == 0
        && header->e_ident[4] == ELFCLASS64
        && header->e_ident[5] == ELFDATA2LSB
        && header->e_ident[6] == EV_CURRENT
        && header->e_type == ET_EXEC
        && header->e_machine == EM_X86_64
        && header->e_phentsize == sizeof(Elf64_Phdr)
        && header->e_phoff <= size
        && header->e_phnum <= (size - header->e_phoff) / sizeof(Elf64_Phdr);
}

uint64_t elf_load(uint64_t space, const void *file, size_t size)
{
    Elf64_Ehdr header;

    if (size < sizeof(header))
        return 0;
    memcpy(&header, file, sizeof(header));
    if (!header_valid(&header, size)) {
        klog(KLOG_WARN, "elf: not a static x86-64 executable");
        return 0;
    }

    /* PT_LOAD segments are sorted by address, so requiring each to start past
       the previous one's last page rules out overlaps. */
    uint64_t next_free = PAGE_SIZE;
    bool entry_mapped = false;

    for (uint16_t i = 0; i < header.e_phnum; i++) {
        Elf64_Phdr segment;
        memcpy(&segment, (const char *)file + header.e_phoff + i * sizeof(segment), sizeof(segment));

        if (segment.p_type == PT_INTERP) {
            klog(KLOG_WARN, "elf: dynamically linked executables are not supported");
            return 0;
        }
        if (segment.p_type != PT_LOAD || segment.p_memsz == 0)
            continue;

        if (segment.p_filesz > segment.p_memsz
                || segment.p_offset > size
                || segment.p_filesz > size - segment.p_offset
                || (segment.p_vaddr & ~(PAGE_SIZE - 1)) < next_free
                || segment.p_vaddr > USER_STACK_BOTTOM
                || segment.p_memsz > USER_STACK_BOTTOM - segment.p_vaddr) {
            klog(KLOG_WARN, "elf: segment %u is out of bounds", i);
            return 0;
        }

        uint64_t flags = 0;
        if (segment.p_flags & PF_W)
            flags |= PAGE_WRITE;
        if (!(segment.p_flags & PF_X))
            flags |= PAGE_NX;

        if (!paging_map_user(space, segment.p_vaddr, segment.p_memsz,
                             (const char *)file + segment.p_offset, segment.p_filesz, flags))
            return 0;

        next_free = (segment.p_vaddr + segment.p_memsz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        /* A non-canonical entry would fault in the kernel's iretq, not in the process. */
        if ((segment.p_flags & PF_X) && header.e_entry >= segment.p_vaddr
                && header.e_entry < segment.p_vaddr + segment.p_memsz)
            entry_mapped = true;
    }

    if (!entry_mapped) {
        klog(KLOG_WARN, "elf: entry point 0x%lx is not in executable code", header.e_entry);
        return 0;
    }
    return header.e_entry;
}
