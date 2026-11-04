# Capacity and narrowing audit (F7.1)

One command audits a registry-capable Emerald Expansion source tree (recipe 35 and its successors) for content IDs that
are narrowed or stored narrower than 32 bits, and writes one report of every finding nobody has reviewed yet. It is the
repeatable form of the audits that found patch 0023 (16-bit task slots holding species and a narrowed send-out cry) and
the 28-byte map headers, and it is required for every new content category (F7).

```bash
# in WSL, over a built tree (tools/preproc and build/assets exist); the baseline is the tree it derives from
PYTHON=<python with pyelftools> \
  tools/admg101/audit/run_audit.sh <built tree> <baseline tree> --out <run directory>
```

Exit status 0 means every finding is reviewed; 1 means `report.md` lists unreviewed findings; 2 is a tool failure. The run
directory keeps the clang logs (`clang/`), the DINFO build (`dinfo/`), each filter's findings (`findings/*.txt`),
`dwarf.json` and `findings.sha256`.

## What runs

| Step | Script | Finds |
| --- | --- | --- |
| clang narrowing audit | `clang_audit.sh` | Every implicit conversion that loses integer precision (`-Wimplicit-int-conversion`, `-Wconstant-conversion`), reported by clang over the exact preprocessed sources GCC compiles (the Makefile's `CPPFLAGS`, the `preproc` pass). A source clang cannot compile is a finding (`clang-failed`). |
| `enum` | `enum_narrow.py` | A content enum (`enum Species`, `Move`, `Item`, `Ability`, `Type`, `TrainerId`, `NationalDexOrder`, …) or content typedef (`nature_t`, `mapsec_*`) converted to a narrower type. |
| `mondata` | `mondata_narrow.py` | A content value read through the `GetMonData` family (`MON_DATA_SPECIES`, `HELD_ITEM`, `MOVE1`–`4`, `HIDDEN_NATURE`, `TERA_TYPE`, `MET_LOCATION`, `POKEBALL`) narrowed: those getters return plain `u32`. |
| `cid` | `cid_narrow.py` | A task's `contentIds[]` or a sprite's `contentId` narrowed. |
| `dest` | `dest_narrow.py` | A narrowing whose destination (variable, member, slot, declared local) is named like a content ID. |
| `data` | `data_narrow.py` | A 32-bit store into a 16-bit task or sprite `data[]` slot on a line that mentions a content kind (`--all` lists every 32-bit `data[]` store). |
| `gfx` | `gfx_narrow.py` | F1.6: a narrowing into a graphics identity (a sprite or palette tag, a graphics ID, a picture ID, an icon palette index) whose value comes from a content ID (species, item, trainer picture). Sprite indices, OAM palette slots and templates are slots the game allocates, not identities, and are not reported. |
| `dwarf` | `dinfo_build.sh`, `dwarf_audit.py` | From a copy of the tree built with `DINFO=1` (`.rel.debug*` stripped for pyelftools): every struct member or global holding a content enum narrower than 32 bits or in a bit field, and every 8/16-bit integer member, array or pointer named like a content ID. The DINFO build is a copy of the same tree; its ROM differs from the release ROM (with this toolchain `-g` changes code placement), so its SHA-1 is recorded to show two runs built the same thing. |
| `layout` | `../asm_layout_probe.sh` | With a baseline tree: every `sizeof`/`offsetof` of a struct whose instances are emitted as assembly that differs from the baseline. |
| report | `report.py` | Applies `allowlist.txt` and writes `report.md`. |

## Map identity pass (F3.3a)

`run_map_identity.sh <tree> <audit dir> <report prefix>` lists, from the DINFO ELF `run_audit.sh` built and from the tree,
every place a build keeps, passes or encodes a map identity: members, globals, parameters and locals, which of them the
native save reaches, map lookups and warp setters, `MAP_GROUP()`/`MAP_NUM()` uses, script macros whose bytecode carries a
map, and the capacity constants of map content. It is an inventory for the map identity width design
(ADMG-101's `docs/admg101-f3.3a-map-identity.md`), not a pass/fail gate.

## The allowlist

`allowlist.txt` holds the reviewed findings that are accepted, each with a reason id defined in its `[reasons]` section.
An entry accepts exactly one finding: its filter, its location without the line number and the detail text exactly as the
report prints it. Nothing is suppressed by pattern: a second site with the same text, a changed line, a new member or a new
file is reported as unreviewed. Entries that match nothing any more are listed so they can be removed.

To review a new finding: fix it in the source series, or, if it is not a content capacity problem, add an entry with a
reason (an existing one, or a new reason that says why). Never add entries to make a run pass without that review.

## Requirements and reproducibility

WSL or Linux with `arm-none-eabi-gcc`/binutils, `clang`, `make` and Python 3 with `pyelftools` (`PYTHON` names the
interpreter; a venv with pyelftools 0.33 is enough). Jobs are capped at `AUDIT_JOBS` (default 8). Two runs' results can be compared by their `findings.sha256` (the clang log
and every filter's findings).
