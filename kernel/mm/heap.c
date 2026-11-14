#include <asm/irq.h>
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

/* Header before each block; the list is in address order, with no gaps. */
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

static bool map_heap_page(uint64_t virt)
{
    uint64_t phys = pmm_alloc_page();

    if (phys == 0)
        return false;
    if (paging_map_page(virt, phys, PAGE_WRITE | PAGE_NX))
        return true;

    pmm_free_page(phys);
    return false;
}

static struct heap_block *grow_heap(size_t size)
{
    uint64_t length = (size + sizeof(struct heap_block) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (length < HEAP_GROW_MIN)
        length = HEAP_GROW_MIN;
    if (length > HEAP_END - heap_end || length / PAGE_SIZE > pmm_free_page_count())
        return NULL;

    for (uint64_t offset = 0; offset < length; offset += PAGE_SIZE) {
        if (!map_heap_page(heap_end + offset)) {
            while (offset > 0) {
                offset -= PAGE_SIZE;
                pmm_free_page(paging_unmap_page(heap_end + offset));
            }
            return NULL;
        }
    }

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

static void *heap_alloc(size_t size)
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

/* Keeps some free tail mapped, to avoid map/unmap churn. */
static void shrink_heap(void)
{
    struct heap_block *block = last_block;
    uint64_t keep = ((uint64_t)(block + 1) + HEAP_GROW_MIN + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (block->magic != MAGIC_FREE || heap_end < keep + HEAP_GROW_MIN)
        return;

    while (heap_end > keep) {
        heap_end -= PAGE_SIZE;
        pmm_free_page(paging_unmap_page(heap_end));
    }
    block->size = keep - (uint64_t)(block + 1);
}

void *kmalloc(size_t size)
{
    uint64_t flags = irq_save();
    void *pointer = heap_alloc(size);
    irq_restore(flags);
    return pointer;
}

void kfree(void *pointer)
{
    if (pointer == NULL)
        return;

    uint64_t flags = irq_save();
    uint64_t address = (uint64_t)pointer;

    if (address < HEAP_START || address >= heap_end || address % HEAP_ALIGN != 0)
        panic("bad pointer freed");

    struct heap_block *block = (struct heap_block *)pointer - 1;

    if (block->magic == MAGIC_FREE)
        panic("heap block freed twice");
    if (block->magic != MAGIC_USED)
        panic("bad pointer freed");

    /* Catches writes past the end of this block. */
    if (block->next != NULL)
        check_block(block->next);

    block->magic = MAGIC_FREE;

    if (block->next != NULL && block->next->magic == MAGIC_FREE)
        merge_with_next(block);
    if (block->prev != NULL && block->prev->magic == MAGIC_FREE)
        merge_with_next(block->prev);

    shrink_heap();
    irq_restore(flags);
}

void heap_selftest(void)
{
    void *a = kmalloc(100);
    void *b = kmalloc(200);
    void *c = kmalloc(300);

    if (a == NULL || b == NULL || c == NULL || a == b || b == c || (uint64_t)a % HEAP_ALIGN != 0)
        panic("heap self-test: bad allocation");

    kfree(b);
    kfree(a);
    if (kmalloc(300) != a)
        panic("heap self-test: freed blocks were not merged");

    kfree(a);
    kfree(c);
    if (first_block != last_block || first_block->magic != MAGIC_FREE)
        panic("heap self-test: heap did not return to one free block");

    uint64_t pages = pmm_free_page_count();
    uint64_t end = heap_end;

    if (kmalloc((pages + 1) * PAGE_SIZE) != NULL || pmm_free_page_count() != pages)
        panic("heap self-test: allocated more memory than there is");

    /* Data pages fit but their page tables don't, so growth fails halfway. */
    if (kmalloc(pages * PAGE_SIZE - sizeof(struct heap_block)) != NULL)
        panic("heap self-test: allocated the memory page tables needed");
    if (heap_end != end || pages - pmm_free_page_count() > pages / 256 + 4)
        panic("heap self-test: failed growth leaked pages");
    if (first_block != last_block || first_block->magic != MAGIC_FREE)
        panic("heap self-test: failed growth changed the heap");

    pages = pmm_free_page_count();
    void *big = kmalloc(1024 * 1024);

    if (big == NULL)
        panic("heap self-test: could not grow the heap");
    kfree(big);
    if (heap_end - HEAP_START > 2 * HEAP_GROW_MIN
            || pmm_free_page_count() + heap_end / PAGE_SIZE != pages + end / PAGE_SIZE)
        panic("heap self-test: freed memory was not returned");

    pages = pmm_free_page_count();
    uint64_t page = pmm_alloc_page();

    if (page == 0 || pmm_free_page_count() != pages - 1)
        panic("heap self-test: bad page allocation");
    pmm_free_page(page);
    if (pmm_free_page_count() != pages)
        panic("heap self-test: page was not freed");
}
