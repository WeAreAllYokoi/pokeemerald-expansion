#!/bin/bash
# The dynamic-content capacity and narrowing audit (F7.1): runs every check over a registry-capable Emerald Expansion
# source tree and writes one report of the findings no allowlist entry has reviewed.
#
#   run_audit.sh <source tree> [<baseline tree>] [--out <dir>] [--elf <DINFO ELF>] [--no-dwarf]
#
#   <source tree>    a built tree (tools/preproc, build/assets): the clang audit and its filters run over it, and a copy
#                    of it is built with DWARF for the member audit unless --elf gives a stripped DINFO ELF of it.
#   <baseline tree>  optional, the tree it derives from (recipe 34): the layouts of assembly-emitted structs are
#                    compared with it (asm_layout_probe.sh).
#   --out            the run directory (default ./admg101-audit-<date>); report.md is written there.
#   --no-dwarf       skip the DINFO build and the member audit (they are reported as not run).
#
# Exit status: 0 when every finding is reviewed (allowlist.txt), 1 when any is not, 2 on a tool failure.
# Needs, in WSL or Linux: arm-none-eabi-gcc/binutils, clang, python3 with pyelftools (PYTHON may name another
# interpreter). Jobs run AUDIT_JOBS at a time (default 8). <out>/findings.sha256 records the results' checksums so two
# runs can be compared byte for byte.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
PYTHON="${PYTHON:-python3}"
SRC= BASE= OUT= ELF= DWARF=1
while [ $# -gt 0 ]; do
  case "$1" in
    --out) OUT="$2"; shift 2;;
    --elf) ELF="$2"; shift 2;;
    --no-dwarf) DWARF=0; shift;;
    -*) echo "unknown option $1" >&2; exit 2;;
    *) if [ -z "$SRC" ]; then SRC="$1"; elif [ -z "$BASE" ]; then BASE="$1"; else echo "too many trees" >&2; exit 2; fi; shift;;
  esac
done
[ -n "$SRC" ] || { sed -n '2,20p' "$0" >&2; exit 2; }
SRC="$(cd "$SRC" && pwd)"
OUT="${OUT:-$PWD/admg101-audit-$(date +%Y%m%d-%H%M%S)}"
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"
F="$OUT/findings"; rm -rf "$F"; mkdir -p "$F"

{
  echo "Source tree: $SRC"
  echo "Source commit: $(git -C "$SRC" rev-parse HEAD 2>/dev/null || echo unknown)"
  [ -f "$SRC/pokeemerald.gba" ] && echo "Source ROM SHA-1: $(sha1sum "$SRC/pokeemerald.gba" | cut -d' ' -f1)"
  [ -n "$BASE" ] && echo "Baseline tree: $BASE (commit $(git -C "$BASE" rev-parse HEAD 2>/dev/null || echo unknown))"
  echo "Toolchain: $(clang --version | head -1); $(arm-none-eabi-gcc --version | head -1)"
  echo "Allowlist: tools/admg101/audit/allowlist.txt ($(sha256sum "$HERE/allowlist.txt" | cut -c1-12))"
} > "$F/meta.txt"

echo "== clang narrowing audit"
"$HERE/clang_audit.sh" "$SRC" "$OUT/clang"
grep -v '^0 ' "$OUT/clang/status.txt" | awk '{ print $2 ": clang exit " $1 }' > "$F/clang-failed.txt" || true
for f in enum mondata cid dest data gfx; do
  "$PYTHON" "$HERE/${f}_narrow.py" "$OUT/clang/all.log" > "$F/$f.txt"
done

if [ "$DWARF" = 1 ]; then
  echo "== DWARF member audit"
  if [ -z "$ELF" ]; then
    "$HERE/dinfo_build.sh" "$SRC" "$OUT/dinfo"
    ELF="$OUT/dinfo/pokeemerald.dinfo.elf"
    DSHA="$(cut -d' ' -f1 "$OUT/dinfo/rom.sha1")"
    # The DWARF types come from the same source; the ROM itself differs (with this toolchain -g changes code placement),
    # so its SHA-1 is recorded to show the DINFO build is reproducible between runs, not compared with the source ROM.
    echo "DINFO build ROM SHA-1: $DSHA (built from a copy of the source tree with DINFO=1)" >> "$F/meta.txt"
    echo "$DSHA  dinfo/pokeemerald.gba" > "$OUT/dinfo.sha1"
  fi
  "$PYTHON" -c 'import elftools' || { echo "pyelftools is missing for $PYTHON" >&2; exit 2; }
  "$PYTHON" "$HERE/dwarf_audit.py" "$ELF" "$OUT/dwarf.json" "$F/dwarf.txt"
fi

if [ -n "$BASE" ]; then
  echo "== assembly-emitted struct layouts against the baseline"
  # A probe that does not compile against either tree is a tool failure, not "no layout differs"
  "$HERE/../asm_layout_probe.sh" "$BASE" "$SRC" > "$OUT/layout-probe.txt" || { echo "asm_layout_probe.sh failed" >&2; exit 2; }
  grep -v '^no layout differs$' "$OUT/layout-probe.txt" | sed 's/^\([^:]*\): /layout \1: /' > "$F/layout.txt" || true
fi

(cd "$F" && sha256sum *.txt | grep -v meta.txt) > "$OUT/findings.sha256"
sha256sum "$OUT/clang/all.log" | sed "s|$OUT/||" >> "$OUT/findings.sha256"
[ -f "$OUT/dinfo.sha1" ] && cat "$OUT/dinfo.sha1" >> "$OUT/findings.sha256"
echo "== report"
rc=0; "$PYTHON" "$HERE/report.py" "$F" "$HERE/allowlist.txt" "$OUT/report.md" || rc=$?
exit $rc
