# Building ADMG-101's Emerald Expansion

This fork is Emerald Expansion with ADMG-101's changes. Upstream makes features compile-time choices: you edit
`include/config/*.h` and rebuild. This fork makes them run-time choices in one built game, so that ADMG-101's mods can
select them. Every option is read through a run-time selector (`GetConfig`, `ADMG_GetFeaturePolicy`, the feature settings
and bindings), and data an option changes stays built, with its per-value differences measured. The tools here keep it
that way.

## Requirements

WSL or Linux with `make`, `gcc`, `arm-none-eabi-gcc` and binutils, and Python 3. The capacity audit also needs `clang` and
a Python with `pyelftools`.

## Build

```bash
make -j$(nproc)
```

This writes `pokeemerald.gba` and `pokeemerald.elf` at the root. A build reproduces exactly: build 39 (branch `admg-101`)
gives ROM SHA-1 `cb305c76bd7337d02ef55c491c2cb1b2e9a10fe3`. Check that before changing anything.

## When you change code that reads a configuration option

A read outside the selector keeps the compiled value, so selecting the option would apply only partly. After any change:

```bash
python3 -B tools/admg101/audit_r38_config_reads.py . --check
python3 -B tools/admg101/route_r38_config_extra.py --check .
```

Route a read the audit lists through the selector, as the rest of the source does. If the set of options read outside
`GetConfig` changed, regenerate the two committed headers, `include/constants/admg_config_catalog.h` and
`include/constants/admg_config_extra.h`, and commit them:

```bash
python3 -B tools/admg101/audit_r38_config_reads.py . --emit
python3 -B tools/admg101/route_r38_config_extra.py .
```

The checks' own tests: `test_config_read_detection.py` and `test_config_variants_cache.py`.

## For a new build

ADMG-101 recompiles each build into a native game. It needs these from the build, made with the tools here:

| What | Tool |
|---|---|
| Symbols and relocations | `export_symbols_v2.py --elf pokeemerald.elf --output symbols.json` |
| Each fixed option's data per value | `extract_r38_config_variants.py <source> <API descriptor> <output>` (the descriptor comes from ADMG-101) |
| Layout changes of assembly-emitted structs | `asm_layout_probe.sh <previous build's tree> <new tree>` |
| Content IDs narrowed below 32 bits | `audit/run_audit.sh` (see `audit/README.md`), for a new content category |
| Computed jumps the recompiler couldn't follow | `check_r38_dispatch.py <coverage-r1.json> pokeemerald.gba`, after ADMG-101 recompiles |

ADMG-101 does the rest (the recipe, the API descriptor, the target profile) with its own tools.

## Merging upstream

See [UPSTREAM-MERGE.md](UPSTREAM-MERGE.md).
