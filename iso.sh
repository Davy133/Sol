#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/sol.kernel isodir/boot/sol.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "sol" {
	multiboot /boot/sol.kernel
}
EOF
grub-mkrescue -o sol.iso isodir
