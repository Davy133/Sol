#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <stdbool.h>
#include <string.h>

#define BITS_PER_WORD   64

/* One bit per physical page; a set bit means the page is in use. */
static uint64_t *bitmap;
static size_t word_count;
static uint64_t page_count;
static uint64_t free_pages;
static size_t next_word;

static bool page_is_used(uint64_t page)
{
    return bitmap[page / BITS_PER_WORD] & (1ULL << (page % BITS_PER_WORD));
}

static void mark_used(uint64_t page)
{
    if (page_is_used(page))
        return;

    bitmap[page / BITS_PER_WORD] |= 1ULL << (page % BITS_PER_WORD);
    free_pages--;
}

static void mark_free(uint64_t page)
{
    bitmap[page / BITS_PER_WORD] &= ~(1ULL << (page % BITS_PER_WORD));
    free_pages++;
}

static uint64_t highest_usable_address(const struct boot_info *info)
{
    uint64_t highest = 0;

    for (size_t i = 0; i < info->memory_region_count; i++) {
        const struct boot_memory_region *region = &info->memory_regions[i];

        if (region->type == BOOT_MEMORY_USABLE && region->base + region->length > highest)
            highest = region->base + region->length;
    }

    return highest;
}

static uint64_t find_bitmap_space(const struct boot_info *info, uint64_t size)
{
    for (size_t i = 0; i < info->memory_region_count; i++) {
        const struct boot_memory_region *region = &info->memory_regions[i];

        if (region->type == BOOT_MEMORY_USABLE && region->length >= size)
            return region->base;
    }

    panic("no memory for the page bitmap");
}

void pmm_init(const struct boot_info *info)
{
    page_count = highest_usable_address(info) / PAGE_SIZE;
    word_count = (page_count + BITS_PER_WORD - 1) / BITS_PER_WORD;

    uint64_t bitmap_size = (word_count * sizeof(uint64_t) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uint64_t bitmap_phys = find_bitmap_space(info, bitmap_size);

    /* Everything starts used, so holes in the memory map can never be handed out. */
    bitmap = (uint64_t *)(bitmap_phys + info->physmap_offset);
    memset(bitmap, 0xff, word_count * sizeof(uint64_t));

    for (size_t i = 0; i < info->memory_region_count; i++) {
        const struct boot_memory_region *region = &info->memory_regions[i];

        if (region->type != BOOT_MEMORY_USABLE)
            continue;

        uint64_t start = (region->base + PAGE_SIZE - 1) / PAGE_SIZE;
        uint64_t end = (region->base + region->length) / PAGE_SIZE;

        for (uint64_t page = start; page < end; page++)
            mark_free(page);
    }

    for (uint64_t offset = 0; offset < bitmap_size; offset += PAGE_SIZE)
        mark_used((bitmap_phys + offset) / PAGE_SIZE);

    /* 0 is what pmm_alloc_page() returns on failure. */
    mark_used(0);
}

uint64_t pmm_alloc_page(void)
{
    for (size_t i = 0; i < word_count; i++) {
        size_t word = (next_word + i) % word_count;

        if (bitmap[word] == UINT64_MAX)
            continue;

        uint64_t page = word * BITS_PER_WORD + __builtin_ctzll(~bitmap[word]);
        mark_used(page);
        next_word = word;
        return page * PAGE_SIZE;
    }

    return 0;
}

void pmm_free_page(uint64_t address)
{
    uint64_t page = address / PAGE_SIZE;

    if (address % PAGE_SIZE != 0 || page >= page_count || !page_is_used(page))
        panic("bad physical page freed");

    mark_free(page);
    if (page / BITS_PER_WORD < next_word)
        next_word = page / BITS_PER_WORD;
}

uint64_t pmm_free_page_count(void)
{
    return free_pages;
}
