#!/bin/bash
# Baut, installiert und macht einen Screenshot mit fester Zeit.
# Aufruf: bash tools/shot.sh <plattform> <ausgabe.png> [hh:mm:ss]
set -e
cd /mnt/c/dev/ClaudeCode/pebble/yourstyle_light
pebble install --emulator "$1" >/dev/null 2>&1
sleep 5
pebble emu-set-time "${3:-10:09:30}" --emulator "$1" >/dev/null 2>&1
sleep 1
pebble screenshot --no-open --emulator "$1" "$2" 2>&1 | grep Saved
