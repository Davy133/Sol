#include <kernel/boot.h>
#include <kernel/serial.h>

#include "limine.h"

/* Limine finds these by scanning the image, before any kernel code runs. */
__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_address_request executable_address_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

static struct boot_info boot_info;

__attribute__((__noreturn__))
static void boot_fail(const char* reason) {
    serial_init();
    serial_write_string("limine: ");
    serial_write_string(reason);
    serial_write_string("\n");
    arch_halt();
}

static enum boot_memory_type convert_memory_type(uint64_t type) {
    switch (type) {
        case LIMINE_MEMMAP_USABLE:
            return BOOT_MEMORY_USABLE;
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            return BOOT_MEMORY_ACPI_RECLAIMABLE;
        case LIMINE_MEMMAP_ACPI_NVS:
            return BOOT_MEMORY_ACPI_NVS;
        case LIMINE_MEMMAP_BAD_MEMORY:
            return BOOT_MEMORY_BAD;
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            return BOOT_MEMORY_BOOTLOADER_RECLAIMABLE;
        case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
            return BOOT_MEMORY_KERNEL;
        case LIMINE_MEMMAP_FRAMEBUFFER:
            return BOOT_MEMORY_FRAMEBUFFER;
        case LIMINE_MEMMAP_RESERVED_MAPPED:
            return BOOT_MEMORY_RESERVED_MAPPED;
        default:
            return BOOT_MEMORY_RESERVED;
    }
}

/* Copied because Limine's response lives in bootloader-reclaimable memory. */
static void read_memory_map(void) {
    struct limine_memmap_response* memmap = memmap_request.response;

    if (memmap == NULL)
        boot_fail("no memory map");
    if (memmap->entry_count > BOOT_MAX_MEMORY_REGIONS)
        boot_fail("too many memory map entries");

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct boot_memory_region* region = &boot_info.memory_regions[i];
        region->base = memmap->entries[i]->base;
        region->length = memmap->entries[i]->length;
        region->type = convert_memory_type(memmap->entries[i]->type);
    }
    boot_info.memory_region_count = memmap->entry_count;
}

static void read_framebuffer(void) {
    struct limine_framebuffer_response* response = framebuffer_request.response;

    if (response == NULL || response->framebuffer_count < 1)
        return;

    struct limine_framebuffer* limine_fb = response->framebuffers[0];
    if (limine_fb->memory_model != LIMINE_FRAMEBUFFER_RGB)
        return;

    boot_info.framebuffer = (struct boot_framebuffer) {
        .address = limine_fb->address,
        .width = limine_fb->width,
        .height = limine_fb->height,
        .pitch = limine_fb->pitch,
        .bpp = limine_fb->bpp,
        .red_size = limine_fb->red_mask_size,
        .red_shift = limine_fb->red_mask_shift,
        .green_size = limine_fb->green_mask_size,
        .green_shift = limine_fb->green_mask_shift,
        .blue_size = limine_fb->blue_mask_size,
        .blue_shift = limine_fb->blue_mask_shift,
    };
}

void _start(void) {
    if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision))
        boot_fail("base revision not supported");
    if (hhdm_request.response == NULL)
        boot_fail("no higher half direct map");
    if (executable_address_request.response == NULL)
        boot_fail("no kernel address");

    boot_info.physmap_offset = hhdm_request.response->offset;
    boot_info.kernel_physical_base = executable_address_request.response->physical_base;
    read_memory_map();
    read_framebuffer();

    arch_start(&boot_info);
}
