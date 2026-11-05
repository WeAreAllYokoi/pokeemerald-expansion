#!/bin/bash
# Makes the BPS patch that turns a retail Pokémon Emerald (USA, Europe) ROM into this build, for its GitHub release.
#
#   bash tools/admg101/make_bps.sh <retail Emerald ROM> <this build's pokeemerald.gba> <out.bps>
#
# Run it in WSL or Linux after building (make). It refuses a retail ROM or a build whose SHA-1 is not the expected one
# (tools/admg101/rom-sha1.txt for the build), builds Floating IPS from its source at a pinned commit (no published
# checksums exist for its release binaries), creates the patch, and checks that applying it to the retail ROM gives
# this build exactly. Neither ROM is copied anywhere; only the .bps is written. Upload it to the build's release by hand.
set -euo pipefail
RETAIL_SHA1=f3ae088181bf583e55daf962a92bb46f4f1d07b7   # Pokémon Emerald (USA, Europe) Rev 0
FLIPS_REPOSITORY=https://github.com/Sir-Walrus/Flips.git  # Floating IPS by its author (renamed from Alcaro)
FLIPS_COMMIT=359d414cff73b3e0400871d3c288a59b36564834    # tag v198
HERE="$(cd "$(dirname "$0")" && pwd)"

if [ $# -ne 3 ]; then sed -n '2,9p' "$0"; exit 2; fi
RETAIL="$1"; BUILT="$2"; OUT="$3"
BUILD_SHA1="$(tr -d '[:space:]' < "$HERE/rom-sha1.txt")"

check() { # file, expected SHA-1, what
  local got; got="$(sha1sum "$1" | cut -d' ' -f1)"
  if [ "$got" != "$2" ]; then echo "make_bps.sh: $3 $1 has SHA-1 $got, not $2" >&2; exit 1; fi
}
check "$RETAIL" "$RETAIL_SHA1" "the retail ROM"
check "$BUILT" "$BUILD_SHA1" "the build"

FLIPS_DIR="${XDG_CACHE_HOME:-$HOME/.cache}/admg101/tools/flips-$FLIPS_COMMIT"
if [ ! -x "$FLIPS_DIR/flips" ]; then
  rm -rf "$FLIPS_DIR" && mkdir -p "$FLIPS_DIR"
  git -C "$FLIPS_DIR" init -q
  git -C "$FLIPS_DIR" fetch -q --depth 1 "$FLIPS_REPOSITORY" "$FLIPS_COMMIT"
  git -C "$FLIPS_DIR" checkout -q FETCH_HEAD
  test "$(git -C "$FLIPS_DIR" rev-parse HEAD)" = "$FLIPS_COMMIT"
  make -C "$FLIPS_DIR" TARGET=cli >"$FLIPS_DIR/build.log" 2>&1 || { tail -20 "$FLIPS_DIR/build.log"; exit 1; }
fi
FLIPS="$FLIPS_DIR/flips"

"$FLIPS" --create --bps-delta "$RETAIL" "$BUILT" "$OUT" >/dev/null
CHECK="$(mktemp)"
trap 'rm -f "$CHECK"' EXIT
"$FLIPS" --apply "$OUT" "$RETAIL" "$CHECK" >/dev/null
check "$CHECK" "$BUILD_SHA1" "the retail ROM patched with"
echo "wrote $OUT: $(stat -c %s "$OUT") bytes, SHA-1 $(sha1sum "$OUT" | cut -d' ' -f1), SHA-256 $(sha256sum "$OUT" | cut -d' ' -f1)"
echo "applied to the retail ROM it gives this build ($BUILD_SHA1)"
