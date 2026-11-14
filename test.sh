#!/bin/sh
# Boots on a basic and a feature-rich CPU and checks the serial log.
set -e
export CPPFLAGS=-DSELFTEST
. ./iso.sh

for CPU in qemu64 max; do
  echo "=== -cpu $CPU"
  LOG=$(timeout 10 qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom sol.iso \
    -cpu $CPU -display none -serial stdio -no-reboot "$@" || true)
  echo "$LOG"

  if echo "$LOG" | grep -q 'KERNEL PANIC'; then
    echo "FAIL: kernel panic on -cpu $CPU" >&2
    exit 1
  fi
  for EXPECTED in 'Kernel initialized' 'Memory self-test passed' 'Hello from user mode' 'argv reached echo' 'fault_code' 'Scheduler self-test passed' 'Welcome to the Sol shell'; do
    if ! echo "$LOG" | grep -q "$EXPECTED"; then
      echo "FAIL: missing \"$EXPECTED\" on -cpu $CPU" >&2
      exit 1
    fi
  done
done
echo PASS
