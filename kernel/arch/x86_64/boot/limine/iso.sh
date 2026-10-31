# Sourced by iso.sh from the top of the tree.
LIMINE_VERSION=12.9.6
LIMINE_DIR=kernel/arch/x86_64/boot/limine

if [ ! -f limine-binary/limine.c ]; then
  curl -fL "https://github.com/Limine-Bootloader/Limine/releases/download/v$LIMINE_VERSION/limine-binary.tar.gz" \
    | tar -xzf -
fi
CC=cc CFLAGS='-O2' CPPFLAGS='' $MAKE -C limine-binary

rm -rf isodir
mkdir -p isodir/boot/limine
mkdir -p isodir/EFI/BOOT

cp sysroot/boot/sol.kernel isodir/boot/sol.kernel
cp "$LIMINE_DIR/limine.conf" isodir/boot/limine/
cp limine-binary/limine-bios.sys \
   limine-binary/limine-bios-cd.bin \
   limine-binary/limine-uefi-cd.bin \
   isodir/boot/limine/
cp limine-binary/BOOTX64.EFI isodir/EFI/BOOT/

xorriso -as mkisofs -R -r -J \
  -b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
  -hfsplus -apm-block-size 2048 \
  --efi-boot boot/limine/limine-uefi-cd.bin \
  -efi-boot-part --efi-boot-image --protective-msdos-label \
  isodir -o sol.iso
limine-binary/limine bios-install sol.iso
