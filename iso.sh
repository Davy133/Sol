#!/bin/sh
set -e
. ./build.sh

. "kernel/arch/$(./target-triplet-to-arch.sh $HOST)/boot/$BOOTLOADER/iso.sh"
