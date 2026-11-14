#include <asm/layout.h>
#include <asm/paging.h>
#include <cpuid.h>
#include <kernel/log.h>
#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define PAGE_ENTRIES    512
#define PAGE_ADDRESS    0x000ffffffffff000ULL

#define PAGE_LARGE      (1ULL << 7)
#define LARGE_PAGE_SIZE 0x200000ULL

/* PAT entry 5: write-combining. */
#define PAGE_WC         (PAGE_PAT | PAGE_PWT)

#define MSR_PAT         0x277
#define MSR_EFER        0xc0000080
#define EFER_NXE        (1ULL << 11)
#define CPUID_NX        (1U << 20)
#define CPUID_SMEP      (1U << 7)
#define CR4_SMEP        (1ULL << 20)
#define CR4_SMAP        (1ULL << 21)

/* WB, WT, UC-, UC, WP, WC, UC-, UC: the power-on layout with WC added. */
#define PAT_VALUE       0x0007010500070406ULL

extern char __kernel_start[], __text_start[], __rodata_start[], __data_start[], __kernel_end[];

static struct boot_info *boot;
static uint64_t no_execute;
static uint64_t *kernel_pml4;
static uint64_t kernel_space;
static bool pmm_ready;

/* The memory map before alloc_boot_page() shrank it. */
static struct boot_memory_region regions[BOOT_MAX_MEMORY_REGIONS];

static uint64_t rdmsr(uint32_t msr)
{
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static void wrmsr(uint32_t msr, uint64_t value)
{
    asm volatile("wrmsr" : : "c"(msr), "a"((uint32_t)value), "d"((uint32_t)(value >> 32)));
}

static uint64_t *phys_to_virt(uint64_t address)
{
    return (uint64_t *)(address + boot->physmap_offset);
}

static uint64_t alloc_boot_page(void)
{
    for (size_t i = 0; i < boot->memory_region_count; i++) {
        struct boot_memory_region *region = &boot->memory_regions[i];

        if (region->type != BOOT_MEMORY_USABLE || region->length < PAGE_SIZE)
            continue;

        uint64_t address = region->base;
        region->base += PAGE_SIZE;
        region->length -= PAGE_SIZE;
        return address;
    }

    panic("out of memory for page tables");
}

static uint64_t alloc_page(void)
{
    if (!pmm_ready)
        return alloc_boot_page();

    return pmm_alloc_page();
}

static uint64_t alloc_table(void)
{
    uint64_t address = alloc_page();

    if (address != 0)
        memset(phys_to_virt(address), 0, PAGE_SIZE);
    return address;
}

/* PAGE_USER must be set at every level. */
static uint64_t *next_table(uint64_t *table, size_t index, uint64_t user)
{
    if (!(table[index] & PAGE_PRESENT)) {
        uint64_t address = alloc_table();

        if (address == 0)
            return NULL;
        table[index] = address | PAGE_PRESENT | PAGE_WRITE;
    }
    if (table[index] & PAGE_LARGE)
        panic("mapping inside a large page");

    table[index] |= user;
    return phys_to_virt(table[index] & PAGE_ADDRESS);
}

static bool map_page(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags)
{
    uint64_t user = flags & PAGE_USER;
    uint64_t *table = pml4;

    for (int shift = 39; shift > 12 && table != NULL; shift -= 9)
        table = next_table(table, (virt >> shift) & (PAGE_ENTRIES - 1), user);
    if (table == NULL)
        return false;

    uint64_t *entry = &table[(virt >> 12) & (PAGE_ENTRIES - 1)];
    bool replaced = *entry & PAGE_PRESENT;

    *entry = phys | flags;
    if (replaced)
        asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
    return true;
}

static void map_large_page(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags)
{
    uint64_t *pdpt = next_table(pml4, (virt >> 39) & (PAGE_ENTRIES - 1), 0);
    uint64_t *pd = next_table(pdpt, (virt >> 30) & (PAGE_ENTRIES - 1), 0);

    pd[(virt >> 21) & (PAGE_ENTRIES - 1)] = phys | flags | PAGE_LARGE;
}

static uint64_t *find_entry(uint64_t *pml4, uint64_t virt)
{
    uint64_t *table = pml4;

    for (int shift = 39; shift > 12; shift -= 9) {
        uint64_t entry = table[(virt >> shift) & (PAGE_ENTRIES - 1)];

        if (!(entry & PAGE_PRESENT) || (entry & PAGE_LARGE))
            return NULL;
        table = phys_to_virt(entry & PAGE_ADDRESS);
    }

    return &table[(virt >> 12) & (PAGE_ENTRIES - 1)];
}

static void map_range(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t length, uint64_t flags)
{
    for (uint64_t offset = 0; offset < length; offset += PAGE_SIZE)
        map_page(pml4, virt + offset, phys + offset, flags);
}

static void map_kernel_segment(uint64_t *pml4, const char *start, const char *end, uint64_t flags)
{
    /* Linker symbols can't be subtracted as pointers. */
    uint64_t offset = (uint64_t)start - (uint64_t)__kernel_start;
    uint64_t length = ((uint64_t)end - (uint64_t)start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    map_range(pml4, (uint64_t)start, boot->kernel_physical_base + offset, length, flags);
}

/* Reserved regions may be MMIO, so they stay unmapped. */
static void map_physical_memory(uint64_t *pml4)
{
    uint64_t data = PAGE_PRESENT | PAGE_WRITE | no_execute;

    for (size_t i = 0; i < boot->memory_region_count; i++) {
        const struct boot_memory_region region = regions[i];

        if (region.type == BOOT_MEMORY_RESERVED || region.type == BOOT_MEMORY_BAD)
            continue;

        bool framebuffer = region.type == BOOT_MEMORY_FRAMEBUFFER;
        uint64_t flags = framebuffer ? data | PAGE_WC : data;
        uint64_t address = region.base & ~(PAGE_SIZE - 1);
        uint64_t end = (region.base + region.length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

        /* Only fully inside the region, and not the framebuffer: PAT is another bit there. */
        while (address < end) {
            uint64_t virt = address + boot->physmap_offset;

            if (!framebuffer && (address | virt) % LARGE_PAGE_SIZE == 0 && address >= region.base
                    && address + LARGE_PAGE_SIZE <= region.base + region.length) {
                map_large_page(pml4, virt, address, flags);
                address += LARGE_PAGE_SIZE;
            } else {
                map_page(pml4, virt, address, flags);
                address += PAGE_SIZE;
            }
        }
    }
}

static void enable_no_execute(void)
{
    uint32_t eax, ebx, ecx, edx;

    if (!__get_cpuid(0x80000001, &eax, &ebx, &ecx, &edx) || !(edx & CPUID_NX))
        return;

    wrmsr(MSR_EFER, rdmsr(MSR_EFER) | EFER_NXE);
    no_execute = PAGE_NX;
}

/* SMAP off: syscalls read user buffers directly. */
static void set_user_protection(void)
{
    uint32_t eax, ebx, ecx, edx;
    uint64_t cr4;

    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 &= ~CR4_SMAP;
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx) && (ebx & CPUID_SMEP)) {
        cr4 |= CR4_SMEP;
        klog(KLOG_INFO, "SMEP enabled");
    }
    asm volatile("mov %0, %%cr4" : : "r"(cr4));
}

void paging_init(struct boot_info *info)
{
    boot = info;
    memcpy(regions, info->memory_regions, sizeof(regions));
    enable_no_execute();
    set_user_protection();
    wrmsr(MSR_PAT, PAT_VALUE);

    uint64_t pml4_phys = alloc_table();
    uint64_t *pml4 = phys_to_virt(pml4_phys);

    kernel_pml4 = pml4;
    kernel_space = pml4_phys;

    /* Must exist before any PML4 copies it. */
    next_table(pml4, (HEAP_START >> 39) & (PAGE_ENTRIES - 1), 0);

    map_kernel_segment(pml4, __kernel_start, __text_start, PAGE_PRESENT | PAGE_WRITE | no_execute);
    map_kernel_segment(pml4, __text_start, __rodata_start, PAGE_PRESENT);
    map_kernel_segment(pml4, __rodata_start, __data_start, PAGE_PRESENT | no_execute);
    map_kernel_segment(pml4, __data_start, __kernel_end, PAGE_PRESENT | PAGE_WRITE | no_execute);

    /* Also maps the bootloader's stack we still run on. */
    map_physical_memory(pml4);

    asm volatile("mov %0, %%cr3" : : "r"(pml4_phys) : "memory");

    /* Last, so it sees the final memory map. */
    pmm_init(info);
    pmm_ready = true;
}

bool paging_map_page(uint64_t virt, uint64_t phys, uint64_t flags)
{
    if (!(kernel_pml4[(virt >> 39) & (PAGE_ENTRIES - 1)] & PAGE_PRESENT))
        panic("kernel mapping outside the PML4 entries every space shares");
    if (!no_execute)
        flags &= ~PAGE_NX;

    return map_page(kernel_pml4, virt, phys, flags | PAGE_PRESENT);
}

uint64_t paging_create_space(void)
{
    uint64_t phys = pmm_alloc_page();

    if (phys == 0)
        return 0;

    uint64_t *pml4 = phys_to_virt(phys);
    memset(pml4, 0, PAGE_SIZE / 2);
    memcpy(pml4 + PAGE_ENTRIES / 2, kernel_pml4 + PAGE_ENTRIES / 2, PAGE_SIZE / 2);
    return phys;
}

bool paging_map_user(uint64_t space, uint64_t virt, size_t size, const void *data, size_t data_size, uint64_t flags)
{
    if (!no_execute)
        flags &= ~PAGE_NX;

    for (uint64_t page = virt & ~(PAGE_SIZE - 1); page < virt + size; page += PAGE_SIZE) {
        uint64_t phys = pmm_alloc_page();

        if (phys == 0)
            return false;

        char *memory = (char *)phys_to_virt(phys);
        memset(memory, 0, PAGE_SIZE);

        uint64_t from = page > virt ? page : virt;
        uint64_t to = page + PAGE_SIZE < virt + data_size ? page + PAGE_SIZE : virt + data_size;
        if (from < to)
            memcpy(memory + (from - page), (const char *)data + (from - virt), to - from);

        if (!map_page(phys_to_virt(space), page, phys, flags | PAGE_PRESENT | PAGE_USER)) {
            pmm_free_page(phys);
            return false;
        }
    }

    return true;
}

static void free_table(uint64_t phys, int level)
{
    uint64_t *table = phys_to_virt(phys);

    for (size_t i = 0; i < PAGE_ENTRIES; i++) {
        if (!(table[i] & PAGE_PRESENT))
            continue;
        if (level > 1)
            free_table(table[i] & PAGE_ADDRESS, level - 1);
        else
            pmm_free_page(table[i] & PAGE_ADDRESS);
    }

    pmm_free_page(phys);
}

void paging_destroy_space(uint64_t space)
{
    uint64_t *pml4 = phys_to_virt(space);

    for (size_t i = 0; i < PAGE_ENTRIES / 2; i++)
        if (pml4[i] & PAGE_PRESENT)
            free_table(pml4[i] & PAGE_ADDRESS, 3);

    pmm_free_page(space);
}

void paging_switch(uint64_t space)
{
    uint64_t cr3;

    if (space == 0)
        space = kernel_space;

    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    if (cr3 != space)
        asm volatile("mov %0, %%cr3" : : "r"(space) : "memory");
}

bool paging_user_accessible(uint64_t virt, uint64_t size, uint64_t flags)
{
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));

    if (virt + size < virt || virt + size > USER_END)
        return false;

    for (uint64_t page = virt & ~(PAGE_SIZE - 1); page < virt + size; page += PAGE_SIZE) {
        uint64_t *entry = find_entry(phys_to_virt(cr3 & PAGE_ADDRESS), page);

        uint64_t required = PAGE_PRESENT | PAGE_USER | flags;

        if (entry == NULL || (*entry & required) != required)
            return false;
    }

    return true;
}

uint64_t paging_unmap_page(uint64_t virt)
{
    uint64_t *entry = find_entry(kernel_pml4, virt);

    if (entry == NULL || !(*entry & PAGE_PRESENT))
        return 0;

    uint64_t phys = *entry & PAGE_ADDRESS;
    *entry = 0;
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
    return phys;
}
