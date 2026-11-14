SYSTEM_HEADER_PROJECTS="libc kernel"
PROJECTS="libc kernel user"

export MAKE=${MAKE:-make}
export HOST=${HOST:-$(./default-host.sh)}
export BOOTLOADER=${BOOTLOADER:-limine}

export AR=${HOST}-ar
export AS=${HOST}-as
export CC=${HOST}-gcc

export PREFIX=/usr
export EXEC_PREFIX=$PREFIX
export BOOTDIR=/boot
export LIBDIR=$EXEC_PREFIX/lib
export INCLUDEDIR=$PREFIX/include

export CFLAGS='-O2 -g'
export CPPFLAGS=${CPPFLAGS:-}

export SYSROOT="$(pwd)/sysroot"
export CC="$CC --sysroot=$SYSROOT"

# -elf targets have no system include directory.
if echo "$HOST" | grep -Eq -- '-elf($|-)'; then
  export CC="$CC -isystem=$INCLUDEDIR"
fi
