#!/bin/bash
# F3.3a: the map identity audit of one build, in WSL.
#
#   run_map_identity.sh <build source tree> <audit output dir of run_audit.sh> <report prefix>
#
# Uses the DINFO ELF run_audit.sh built (<audit dir>/dinfo/pokeemerald.dinfo.elf) and the audit's Python (PYTHON, default
# a Python with pyelftools, which has pyelftools). Writes <report prefix>.md and <report prefix>.json.
set -euo pipefail
SRC="$(cd "$1" && pwd)"
AUDIT="$(cd "$2" && pwd)"
OUT="$3"
HERE="$(cd "$(dirname "$0")" && pwd)"
PYTHON="${PYTHON:-$HOME/.venvs/admg101-audit/bin/python}"
ELF="$AUDIT/dinfo/pokeemerald.dinfo.elf"
[ -f "$ELF" ] || { echo "no DINFO ELF at $ELF: run run_audit.sh first" >&2; exit 1; }
"$PYTHON" "$HERE/map_identity.py" "$ELF" "$SRC" "$OUT.json" "$OUT.md"
