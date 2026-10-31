OUTPUT_FORMAT(elf64-x86-64)

/* Limine jumps to the symbol designated at the entry point. */
ENTRY(_start)

/* Each segment gets its own permissions in the page tables Limine builds. */
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

	.limine_requests :
	{
		KEEP(*(.limine_requests_start))
		KEEP(*(.limine_requests))
		KEEP(*(.limine_requests_end))
	} :limine_requests

	. = ALIGN(CONSTANT(MAXPAGESIZE));

	.text :
	{
		*(.text .text.*)
	} :text

	. = ALIGN(CONSTANT(MAXPAGESIZE));

	/* Read-only data. */
	.rodata :
	{
		*(.rodata .rodata.*)
	} :rodata

	. = ALIGN(CONSTANT(MAXPAGESIZE));

	/* Read-write data (initialized) */
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

	/DISCARD/ :
	{
		*(.eh_frame*)
		*(.note .note.*)
		*(.got .got.*)
	}
}
