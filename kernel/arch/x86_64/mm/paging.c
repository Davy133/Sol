#include <asm/paging.h>
#include <cpuid.h>
#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define PAGE_ENTRIES    512
#define PAGE_ADDRESS    0x000ffffffffff000ULL

/* Selects PAT entry 5, write-combining in PAT_VALUE. */
#define PAGE_WC         (PAGE_PAT | PAGE_PWT)

#define MSR_PAT         0x277
#define MSR_EFER        0xc0000080
#define EFER_NXE        (1ULL << 11)
#define CPUID_NX        (1U << 20)

/* WB, WT, UC-, UC, WP, WC, UC-, UC: the power-on layout with WC added. */
#define PAT_VALUE       0x0007010500070406ULL

/* From the linker script. */
extern char __kernel_start[], __text_start[], __rodata_start[], __data_start[], __kernel_end[];

static struct boot_info *boot;
static uint64_t no_execute;
static uint64_t *kernel_pml4;
static bool pmm_ready;

/* The map as the bootloader gave it. alloc_table() shrinks the live one, and
   the pages it removes must still be reachable through the physical map. */
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

/* Taken from the region itself, so the allocator never sees these pages. */
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

    uint64_t address = pmm_alloc_page();
    if (address == 0)
        panic("out of memory for page tables");
    return address;
}

static uint64_t alloc_table(void)
{
    uint64_t address = alloc_page();

    memset(phys_to_virt(address), 0, PAGE_SIZE);
    return address;
}

/* Every level has to allow user access, so PAGE_USER reaches existing tables too. */
static uint64_t *next_table(uint64_t *table, size_t index, uint64_t user)
{
    if (!(table[index] & PAGE_PRESENT))
        table[index] = alloc_table() | PAGE_PRESENT | PAGE_WRITE;

    table[index] |= user;
    return phys_to_virt(table[index] & PAGE_ADDRESS);
}

/* Permissions live in the last level; the ones above allow everything. */
static void map_page(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags)
{
    uint64_t user = flags & PAGE_USER;
    uint64_t *pdpt = next_table(pml4, (virt >> 39) & (PAGE_ENTRIES - 1), user);
    uint64_t *pd = next_table(pdpt, (virt >> 30) & (PAGE_ENTRIES - 1), user);
    uint64_t *pt = next_table(pd, (virt >> 21) & (PAGE_ENTRIES - 1), user);
    uint64_t *entry = &pt[(virt >> 12) & (PAGE_ENTRIES - 1)];
    bool replaced = *entry & PAGE_PRESENT;

    *entry = phys | flags;
    if (replaced)
        asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

/* NULL when a level on the way to virt does not exist. */
static uint64_t *find_entry(uint64_t *pml4, uint64_t virt)
{
    uint64_t *table = pml4;

    for (int shift = 39; shift > 12; shift -= 9) {
        uint64_t entry = table[(virt >> shift) & (PAGE_ENTRIES - 1)];

        if (!(entry & PAGE_PRESENT))
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
    uint64_t offset = (uint64_t)(start - __kernel_start);
    uint64_t length = ((uint64_t)(end - start) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    map_range(pml4, (uint64_t)start, boot->kernel_physical_base + offset, length, flags);
}

/* Reserved regions stay out: they can hold MMIO, which must not be cached. */
static void map_physical_memory(uint64_t *pml4)
{
    uint64_t data = PAGE_PRESENT | PAGE_WRITE | no_execute;

    for (size_t i = 0; i < boot->memory_region_count; i++) {
        const struct boot_memory_region region = regions[i];

        if (region.type == BOOT_MEMORY_RESERVED || region.type == BOOT_MEMORY_BAD)
            continue;

        uint64_t flags = region.type == BOOT_MEMORY_FRAMEBUFFER ? data | PAGE_WC : data;
        uint64_t start = region.base & ~(PAGE_SIZE - 1);
        uint64_t end = (region.base + region.length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

        map_range(pml4, start + boot->physmap_offset, start, end - start, flags);
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

void paging_init(struct boot_info *info)
{
    boot = info;
    memcpy(regions, info->memory_regions, sizeof(regions));
    enable_no_execute();
    wrmsr(MSR_PAT, PAT_VALUE);

    uint64_t pml4_phys = alloc_table();
    uint64_t *pml4 = phys_to_virt(pml4_phys);

    kernel_pml4 = pml4;

    map_kernel_segment(pml4, __kernel_start, __text_start, PAGE_PRESENT | PAGE_WRITE | no_execute);
    map_kernel_segment(pml4, __text_start, __rodata_start, PAGE_PRESENT);
    map_kernel_segment(pml4, __rodata_start, __data_start, PAGE_PRESENT | no_execute);
    map_kernel_segment(pml4, __data_start, __kernel_end, PAGE_PRESENT | PAGE_WRITE | no_execute);

    /* Also keeps the stack alive: it is still the one the bootloader gave us. */
    map_physical_memory(pml4);

    asm volatile("mov %0, %%cr3" : : "r"(pml4_phys) : "memory");

    /* Last, so the bitmap sits past the tables and sees the final memory map. */
    pmm_init(info);
    pmm_ready = true;
}

void paging_map_page(uint64_t virt, uint64_t phys, uint64_t flags)
{
    if (!no_execute)
        flags &= ~PAGE_NX;

    map_page(kernel_pml4, virt, phys, flags | PAGE_PRESENT);
}

void paging_unmap_page(uint64_t virt)
{
    uint64_t *entry = find_entry(kernel_pml4, virt);

    if (entry == NULL || !(*entry & PAGE_PRESENT))
        return;

    *entry = 0;
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}
