#!/bin/bash
# Builds a copy of a source tree with DWARF (make DINFO=1) for dwarf_audit.py and strips the ELF's .rel.debug* sections
# (the --emit-relocs relocations of the debug sections, which pyelftools would otherwise apply, very slowly).
#   dinfo_build.sh <source tree> <work directory>      -> <work>/pokeemerald.dinfo.elf
# The tree itself is not touched; the copy lives in <work>/tree.
set -euo pipefail
SRC="$(cd "$1" && pwd)"; WORK="$(mkdir -p "$2" && cd "$2" && pwd)"
JOBS="${AUDIT_JOBS:-8}"
rm -rf "$WORK/tree"; cp -a "$SRC" "$WORK/tree"; cd "$WORK/tree"
# DINFO only adds -g to CFLAGS: rebuild the C objects, keep the generated assets
rm -rf build/*/src build/*/data pokeemerald.elf pokeemerald.gba
make -j"$JOBS" DINFO=1 > "$WORK/dinfo.log" 2>&1 || { tail -20 "$WORK/dinfo.log" >&2; exit 1; }
arm-none-eabi-objcopy --remove-section='.rel.debug*' pokeemerald.elf "$WORK/pokeemerald.dinfo.elf"
sha1sum pokeemerald.gba | tee "$WORK/rom.sha1"
