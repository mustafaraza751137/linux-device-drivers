#!/bin/bash
# Setup a simulated GPIO chip using gpio-sim (for driver development on a VM).
# Usage:   sudo ./gpio-sim-setup.sh
# Cleanup: sudo ./gpio-sim-setup.sh --cleanup

DEV=mydev
BANK=bank0
NLINES=8
BASE=/sys/kernel/config/gpio-sim

if [ "$1" == "--cleanup" ]; then
    if [ -d "$BASE/$DEV" ]; then
        echo 0 > "$BASE/$DEV/live" 2>/dev/null
        rmdir "$BASE/$DEV/$BANK"/* 2>/dev/null
        rmdir "$BASE/$DEV/$BANK" 2>/dev/null
        rmdir "$BASE/$DEV" 2>/dev/null
        echo "chip destroyed"
    else
        echo "nothing to clean up"
    fi
    modprobe -r gpio-sim 2>/dev/null
    exit 0
fi

# load the module (creates $BASE)
modprobe gpio-sim || { echo "failed to load gpio-sim"; exit 1; }

if [ -e /dev/gpiochip0 ]; then
    echo "gpiochip0 already exists - nothing to do"
    exit 0
fi

mkdir "$BASE/$DEV" 2>/dev/null
mkdir "$BASE/$DEV/$BANK" 2>/dev/null
echo "$NLINES" > "$BASE/$DEV/$BANK/num_lines"

# instantiate the chip
echo 1 > "$BASE/$DEV/live"
if [ $? -ne 0 ]; then
    echo "failed to instantiate chip"
    exit 1
fi

echo "done:"
gpiodetect
