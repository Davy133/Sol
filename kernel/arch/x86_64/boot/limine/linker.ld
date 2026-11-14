OUTPUT_FORMAT(elf64-x86-64)

ENTRY(_start)

PHDRS
{
	limine_requests PT_LOAD FLAGS(0x06); /* Read, write */
	text PT_LOAD FLAGS(0x05); /* Read, execute */
	rodata PT_LOAD FLAGS(0x04); /* Read */
	data PT_LOAD FLAGS(0x06); /* Read, write */
}

SECTIONS
{
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

	__rodata_start = .;
	.rodata :
	{
		*(.rodata .rodata.*)
	} :rodata

	. = ALIGN(CONSTANT(MAXPAGESIZE));

	__data_start = .;
	.data :
	{
		*(.data .data.*)
	} :data

	/* Keep last, or its zeros end up in the file. */
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
