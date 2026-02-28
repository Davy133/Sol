DEFAULT_HOST!=../default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)

CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=
NASM?=nasm
NASMFLAGS?=-f elf32

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc

ARCHDIR=arch/$(HOSTARCH)

include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
core/kernel.o \

OBJS=\
$(ARCHDIR)/cpu/crti.o \
$(ARCHDIR)/cpu/crtbegin.o \
$(KERNEL_OBJS) \
$(ARCHDIR)/cpu/crtend.o \
$(ARCHDIR)/cpu/crtn.o \

LINK_LIST=\
$(LDFLAGS) \
$(ARCHDIR)/cpu/crti.o \
$(ARCHDIR)/cpu/crtbegin.o \
$(KERNEL_OBJS) \
$(LIBS) \
$(ARCHDIR)/cpu/crtend.o \
$(ARCHDIR)/cpu/crtn.o \

.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .asm

all: sol.kernel

sol.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot sol.kernel

$(ARCHDIR)/cpu/crtbegin.o $(ARCHDIR)/cpu/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.asm.o:
	$(NASM) $(NASMFLAGS) -MD $(@:.o=.d) $< -o $@

clean:
	rm -f sol.kernel
	rm -f $(OBJS) *.o */*.o */*/*.o */*/*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d */*/*/*.d */*/*/*.asm.d

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: sol.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp sol.kernel $(DESTDIR)$(BOOTDIR)

-include $(OBJS:.o=.d)
