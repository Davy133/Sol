#ifndef _KERNEL_BOOT_H
#define _KERNEL_BOOT_H

#include <stddef.h>
#include <stdint.h>

/*
 * What a boot adapter (arch/<arch>/boot/<bootloader>/) owes the kernel. Only the
 * adapter knows which bootloader ran; everything past arch_start() uses this.
 *
 * Before calling arch_start() the adapter must leave the CPU in 64-bit mode with
 * interrupts off, a valid stack, the kernel mapped at its link address and all
 * physical memory mapped at physmap_offset.
 */

#define BOOT_MAX_MEMORY_REGIONS 256

enum boot_memory_type {
	BOOT_MEMORY_USABLE,
	BOOT_MEMORY_RESERVED,
	BOOT_MEMORY_ACPI_RECLAIMABLE,
	BOOT_MEMORY_ACPI_NVS,
	BOOT_MEMORY_BAD,
	BOOT_MEMORY_BOOTLOADER_RECLAIMABLE,
	BOOT_MEMORY_KERNEL,
	BOOT_MEMORY_FRAMEBUFFER,
	BOOT_MEMORY_RESERVED_MAPPED,
};

struct boot_memory_region {
	uint64_t base;
	uint64_t length;
	enum boot_memory_type type;
};

/* address is NULL when there is no usable framebuffer. */
struct boot_framebuffer {
	void* address;
	uint64_t width;
	uint64_t height;
	uint64_t pitch;
	uint16_t bpp;
	uint8_t red_size;
	uint8_t red_shift;
	uint8_t green_size;
	uint8_t green_shift;
	uint8_t blue_size;
	uint8_t blue_shift;
};

struct boot_info {
	/* Virtual address of physical address 0. */
	uint64_t physmap_offset;
	/* Physical address of __kernel_start; the image is physically contiguous. */
	uint64_t kernel_physical_base;
	struct boot_memory_region memory_regions[BOOT_MAX_MEMORY_REGIONS];
	size_t memory_region_count;
	struct boot_framebuffer framebuffer;
};

__attribute__((__noreturn__))
void arch_start(struct boot_info* info);
__attribute__((__noreturn__))
void arch_halt(void);

void kernel_main(const struct boot_info* info);

#endif
