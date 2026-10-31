#include <asm/paging.h>
#include <cpuid.h>
#include <kernel/panic.h>
#include <string.h>

#define PAGE_SIZE       4096ULL
#define PAGE_ENTRIES    512

#define PAGE_PRESENT    (1ULL << 0)
#define PAGE_WRITE      (1ULL << 1)
#define PAGE_PWT        (1ULL << 3)
#define PAGE_PAT        (1ULL << 7)     /* In a 4 KiB entry */
#define PAGE_NX         (1ULL << 63)
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

/* Taken from the region itself, so nothing later can hand these pages out. */
static uint64_t alloc_table(void)
{
    for (size_t i = 0; i < boot->memory_region_count; i++) {
        struct boot_memory_region *region = &boot->memory_regions[i];

        if (region->type != BOOT_MEMORY_USABLE || region->length < PAGE_SIZE)
            continue;

        uint64_t address = region->base;
        region->base += PAGE_SIZE;
        region->length -= PAGE_SIZE;
        memset(phys_to_virt(address), 0, PAGE_SIZE);
        return address;
    }

    panic("out of memory for page tables");
}

static uint64_t *next_table(uint64_t *table, size_t index)
{
    if (!(table[index] & PAGE_PRESENT))
        table[index] = alloc_table() | PAGE_PRESENT | PAGE_WRITE;

    return phys_to_virt(table[index] & PAGE_ADDRESS);
}

/* Permissions live in the last level; the ones above allow everything. */
static void map_page(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags)
{
    uint64_t *pdpt = next_table(pml4, (virt >> 39) & (PAGE_ENTRIES - 1));
    uint64_t *pd = next_table(pdpt, (virt >> 30) & (PAGE_ENTRIES - 1));
    uint64_t *pt = next_table(pd, (virt >> 21) & (PAGE_ENTRIES - 1));

    pt[(virt >> 12) & (PAGE_ENTRIES - 1)] = phys | flags;
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

    map_kernel_segment(pml4, __kernel_start, __text_start, PAGE_PRESENT | PAGE_WRITE | no_execute);
    map_kernel_segment(pml4, __text_start, __rodata_start, PAGE_PRESENT);
    map_kernel_segment(pml4, __rodata_start, __data_start, PAGE_PRESENT | no_execute);
    map_kernel_segment(pml4, __data_start, __kernel_end, PAGE_PRESENT | PAGE_WRITE | no_execute);

    /* Also keeps the stack alive: it is still the one the bootloader gave us. */
    map_physical_memory(pml4);

    asm volatile("mov %0, %%cr3" : : "r"(pml4_phys) : "memory");
}
