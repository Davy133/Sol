CFLAGS?=-O2 -g
LDFLAGS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot

# shortcut: general registers only, as task switches do not save SSE state yet.
CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra -mgeneral-regs-only
LDFLAGS:=$(LDFLAGS) -nostdlib -static -Wl,-z,noexecstack -Wl,-Ttext-segment=0x400000 \
-Wl,-z,max-page-size=0x1000 -Wl,-z,separate-code

# The initrd's layout: root/bin/sh is the archive's bin/sh.
PROGRAMS=\
root/bin/sh \
root/bin/ls \
root/bin/cat \
root/bin/echo \
root/bin/uname \
root/test/hello \
root/test/fault_kernel \
root/test/fault_code \

.PHONY: all clean install install-headers install-initrd

all: initrd.tar

# Fixed owner and order, so the archive does not depend on who built it.
initrd.tar: $(PROGRAMS) root/etc/motd
	tar --format=ustar --owner=0 --group=0 --numeric-owner --sort=name -cf $@ -C root bin etc test

crt0.o: crt0.S
	$(CC) -c $< -o $@

root/bin/%: %.c crt0.o sol.h
	mkdir -p $(@D)
	$(CC) -o $@ crt0.o $< -std=gnu11 $(CFLAGS) $(LDFLAGS)

root/test/%: test/%.c crt0.o sol.h
	mkdir -p $(@D)
	$(CC) -o $@ crt0.o $< -std=gnu11 $(CFLAGS) $(LDFLAGS)

root/etc/motd: etc/motd
	mkdir -p $(@D)
	cp $< $@

clean:
	rm -rf root initrd.tar crt0.o

install: install-initrd

install-headers:

install-initrd: initrd.tar
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp initrd.tar $(DESTDIR)$(BOOTDIR)
