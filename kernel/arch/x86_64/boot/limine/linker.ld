OUTPUT_FORMAT(elf64-x86-64)

/* Limine jumps to the symbol designated at the entry point. */
ENTRY(_start)

/* Each segment gets its own permissions; paging_init() needs the symbols below. */
PHDRS
{
	limine_requests PT_LOAD FLAGS(0x06); /* Read, write */
	text PT_LOAD FLAGS(0x05); /* Read, execute */
	rodata PT_LOAD FLAGS(0x04); /* Read */
	data PT_LOAD FLAGS(0x06); /* Read, write */
}

SECTIONS
{
	/* The Limine protocol loads the kernel in the topmost 2 GiB of the address
	   space, which is also what -mcmodel=kernel expects. */
	. = 0xffffffff80000000;
	__kernel_start = .;

	.limine_requests :
	{
		KEEP(*(.limine_requests_start))
		KEEP(*(.limine_requests))
		KEEP(*(.limine_requests_end))
	} :limine_requests

	. = ALIGN(CONSTANT(MAXPAGESIZE));

	__text_start = .;
	.text :
	{
		*(.text .text.*)
	} :text

	. = ALIGN(CONSTANT(MAXPAGESIZE));

	/* Read-only data. */
	__rodata_start = .;
	.rodata :
	{
		*(.rodata .rodata.*)
	} :rodata

	. = ALIGN(CONSTANT(MAXPAGESIZE));

	/* Read-write data (initialized) */
	__data_start = .;
	.data :
	{
		*(.data .data.*)
	} :data

	/* Read-write data (uninitialized). Must stay last in the data segment, or
	   its zeros are written to the file. */
	.bss :
	{
		*(COMMON)
		*(.bss .bss.*)
	} :data

	__kernel_end = .;

	/DISCARD/ :
	{
		*(.eh_frame*)
		*(.note .note.*)
		*(.got .got.*)
	}
}
