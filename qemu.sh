#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom sol.iso -serial stdio \
  -audiodev pa,id=speaker -machine pcspk-audiodev=speaker
