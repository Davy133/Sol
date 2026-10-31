#include <asm/layout.h>
#include <asm/paging.h>
#include <kernel/heap.h>
#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <stdbool.h>

#define HEAP_ALIGN      16ULL
#define HEAP_GROW_MIN   (16 * PAGE_SIZE)

#define MAGIC_USED      0x5553454455534544ULL
#define MAGIC_FREE      0x4652454546524545ULL

/* Sits right before the memory it describes, so its size keeps that aligned.
   The list follows address order and covers the whole heap without gaps. */
struct heap_block {
    uint64_t magic;
    size_t size;                /* Not counting this header */
    struct heap_block *prev;
    struct heap_block *next;
};

_Static_assert(sizeof(struct heap_block) % HEAP_ALIGN == 0, "misaligned heap header");

static struct heap_block *first_block;
static struct heap_block *last_block;
static uint64_t heap_end = HEAP_START;

static void check_block(const struct heap_block *block)
{
    if (block->magic != MAGIC_USED && block->magic != MAGIC_FREE)
        panic("heap corrupted");
}

/* Enough for the pages and, in the worst case, every table they could need. */
static bool has_memory_for(uint64_t pages)
{
    return pmm_free_page_count() >= pages + pages / 256 + 4;
}

static void map_heap_page(uint64_t virt)
{
    uint64_t phys = pmm_alloc_page();

    if (phys == 0)
        panic("out of memory growing the heap");

    paging_map_page(virt, phys, PAGE_WRITE | PAGE_NX);
}

/* Returns a free block of at least size bytes at the end of the heap, or NULL. */
static struct heap_block *grow_heap(size_t size)
{
    uint64_t length = (size + sizeof(struct heap_block) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (length < HEAP_GROW_MIN)
        length = HEAP_GROW_MIN;
    if (length > HEAP_END - heap_end || !has_memory_for(length / PAGE_SIZE))
        return NULL;

    for (uint64_t offset = 0; offset < length; offset += PAGE_SIZE)
        map_heap_page(heap_end + offset);

    struct heap_block *block = last_block;

    if (block != NULL && block->magic == MAGIC_FREE) {
        block->size += length;
    } else {
        block = (struct heap_block *)heap_end;
        block->magic = MAGIC_FREE;
        block->size = length - sizeof(struct heap_block);
        block->prev = last_block;
        block->next = NULL;

        if (last_block != NULL)
            last_block->next = block;
        else
            first_block = block;
        last_block = block;
    }

    heap_end += length;
    return block;
}

/* Gives what block does not need back as a free block, if it fits one. */
static void split_block(struct heap_block *block, size_t size)
{
    if (block->size < size + sizeof(struct heap_block) + HEAP_ALIGN)
        return;

    struct heap_block *rest = (struct heap_block *)((char *)(block + 1) + size);

    rest->magic = MAGIC_FREE;
    rest->size = block->size - size - sizeof(struct heap_block);
    rest->prev = block;
    rest->next = block->next;

    if (block->next != NULL)
        block->next->prev = rest;
    else
        last_block = rest;

    block->next = rest;
    block->size = size;
}

/* Absorbs the block right after this one. */
static void merge_with_next(struct heap_block *block)
{
    struct heap_block *next = block->next;

    block->size += sizeof(struct heap_block) + next->size;
    block->next = next->next;

    if (next->next != NULL)
        next->next->prev = block;
    else
        last_block = block;

    next->magic = 0;
}

void *kmalloc(size_t size)
{
    if (size == 0 || size > HEAP_END - HEAP_START)
        return NULL;

    size = (size + HEAP_ALIGN - 1) & ~(HEAP_ALIGN - 1);

    struct heap_block *block;

    for (block = first_block; block != NULL; block = block->next) {
        check_block(block);
        if (block->magic == MAGIC_FREE && block->size >= size)
            break;
    }

    if (block == NULL)
        block = grow_heap(size);
    if (block == NULL)
        return NULL;

    split_block(block, size);
    block->magic = MAGIC_USED;
    return block + 1;
}

void kfree(void *pointer)
{
    if (pointer == NULL)
        return;

    uint64_t address = (uint64_t)pointer;

    if (address < HEAP_START || address >= heap_end || address % HEAP_ALIGN != 0)
        panic("bad pointer freed");

    struct heap_block *block = (struct heap_block *)pointer - 1;

    if (block->magic == MAGIC_FREE)
        panic("heap block freed twice");
    if (block->magic != MAGIC_USED)
        panic("bad pointer freed");

    /* A write past the end of this block lands on the next header. */
    if (block->next != NULL)
        check_block(block->next);

    block->magic = MAGIC_FREE;

    if (block->next != NULL && block->next->magic == MAGIC_FREE)
        merge_with_next(block);
    if (block->prev != NULL && block->prev->magic == MAGIC_FREE)
        merge_with_next(block->prev);
}
