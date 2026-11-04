#!/bin/bash
# Compares the layouts of the structs whose instances are emitted as assembly between two Emerald Expansion source trees.
#   asm_layout_probe.sh <before tree> <after tree>
# Prints every sizeof/offsetof that differs; nothing is written into either tree.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
probe() {
  (cd "$1" && arm-none-eabi-gcc -mthumb -mthumb-interwork -O2 -mabi=apcs-gnu -mtune=arm7tdmi -march=armv4t -std=gnu17 \
    -DMODERN=1 -DTESTING=0 -DEMERALD -iquote include -c "$HERE/asm_layout_probe.c" -o "$2.o")
  arm-none-eabi-objcopy -O binary -j .rodata "$2.o" "$2.bin"
  od -An -tu4 -w4 -v "$2.bin" | tr -d ' '
}
T="$(mktemp -d)"
trap 'rm -rf "$T"' EXIT
grep -o 'S([A-Za-z]*)\|O([A-Za-z]*,[A-Za-z]*)' "$HERE/asm_layout_probe.c" | grep -v '^S(T)\|^O(T,m)' > "$T/names"
probe "$1" "$T/a" > "$T/av"
probe "$2" "$T/b" > "$T/bv"
paste "$T/names" "$T/av" "$T/bv" | awk '$2 != $3 { print $1 ": " $2 " -> " $3; n++ } END { if (!n) print "no layout differs"; }'
