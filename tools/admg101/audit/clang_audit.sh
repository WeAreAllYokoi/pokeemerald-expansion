#!/bin/bash
# Narrowing audit: every implicit conversion that loses precision, reported by clang over the exact preprocessed sources
# GCC compiles (the Makefile's CPPFLAGS and preproc pass).
#   clang_audit.sh <source tree> <output directory>
# The tree must have been built once (tools/preproc and build/assets exist). Writes <out>/<file>.log per source,
# <out>/status.txt (clang's exit code per file) and <out>/all.log; nothing is written into the tree.
set -euo pipefail
SRC="$(cd "$1" && pwd)"; OUT="$(mkdir -p "$2" && cd "$2" && pwd)"
JOBS="${AUDIT_JOBS:-8}"
[ -x "$SRC/tools/preproc/preproc" ] && [ -d "$SRC/build/assets" ] || { echo "build $SRC once first (preproc, build/assets)" >&2; exit 2; }
rm -rf "$OUT"/logs; mkdir -p "$OUT/logs"; : > "$OUT/status.txt"
cd "$SRC"
ls src/*.c | grep -v '\.inc\.c$' > "$OUT/files.txt"
audit_one() {
  f=$1; b=$(basename "$f" .c); extra=
  # Makefile: ADMG_INTEGRATION_OBJS get the recipe version define
  case "$b" in admg_native|admg_exports) extra="-DADMG_INTEGRATION_RECIPE_VERSION=34";; esac
  arm-none-eabi-cpp -iquote include -Wno-trigraphs -DMODERN=1 -DTESTING=0 -DEMERALD -std=gnu17 $extra "$f" 2>/dev/null \
    | tools/preproc/preproc -i -g build/assets "$f" charmap.txt > "$OUT/logs/$b.i" 2>/dev/null
  rc=0
  clang --target=armv4t-none-eabi -mthumb -fsyntax-only -x cpp-output -std=gnu17 -Wno-everything -Wimplicit-int-conversion \
    -Wconstant-conversion -fno-builtin -ferror-limit=0 -fno-color-diagnostics "$OUT/logs/$b.i" > "$OUT/logs/$b.log" 2>&1 || rc=$?
  echo "$rc $f" >> "$OUT/status.txt"
  rm -f "$OUT/logs/$b.i"
}
export -f audit_one; export OUT
xargs -P "$JOBS" -I{} bash -c 'audit_one {}' < "$OUT/files.txt"
sort -k2 -o "$OUT/status.txt" "$OUT/status.txt"
# concatenate in file order so two runs compare byte for byte
while read -r f; do cat "$OUT/logs/$(basename "$f" .c).log"; done < "$OUT/files.txt" > "$OUT/all.log"
echo "clang audit: $(wc -l < "$OUT/status.txt") files, $(grep -vc '^0 ' "$OUT/status.txt" || true) failed, $(grep -c 'loses integer precision' "$OUT/all.log" || true) precision-losing conversions"
