#!/bin/sh
# Boots the ISO headless and checks the serial log. Arguments go to QEMU.
set -e
. ./iso.sh

LOG=$(timeout 10 qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom sol.iso \
  -display none -serial stdio -no-reboot "$@" || true)
echo "$LOG"

if echo "$LOG" | grep -q 'KERNEL PANIC'; then
  echo 'FAIL: kernel panic' >&2
  exit 1
fi
for EXPECTED in 'Kernel initialized'; do
  if ! echo "$LOG" | grep -q "$EXPECTED"; then
    echo "FAIL: missing \"$EXPECTED\"" >&2
    exit 1
  fi
done
echo PASS
