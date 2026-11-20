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

This writes `pokeemerald.gba` and `pokeemerald.elf` at the root. A build reproduces exactly: build 40 (branch `admg-101`)
gives ROM SHA-1 `bc99c00d9bd16d29572ee9ad19761b5c7b4aa4cc`. Check that before changing anything.

## The symbol table in the ROM

`make ADMG_SYMBOLS=1` ends the build by running `tools/admgsym` on the ROM. It writes the ELF's ROM functions and
data objects (names, addresses, sizes, ARM or Thumb) into the 0xFF padding after the game as an ADMGSYM1 table, with a
footer in the ROM's last 16 bytes. ADMG-101's Ghidra loader (gba-ghidra-loader) reads it, so a build of this fork
names its own functions without its ELF. The format is described at the top of `tools/admgsym/admgsym.c`.

The table also names the code the ELF calls but has no function symbol for: GCC's `bx Rm` call thunks (as
`_call_via_<register>_<address>`) and crt0's code labels. And it records coverage ranges: the parts of the ROM where its
functions are every function there is, worked out from the ELF's `$a`, `$t` and `$d` mapping symbols. The loader then
lets no guess start a function inside them. Multiboot images with a cartridge header and anything given with
`--not-covered <symbol>` stay outside coverage, as does any code the tool finds in no function (other than padding),
of which it prints the first ten stretches when it runs.

It is off by default until the next build, because it changes the ROM's bytes: with it on, the ROM isn't build 40.
Changing the value rebuilds the ROM. The build fails, and the ROM is deleted, if the table doesn't fit in the padding
or the padding isn't all 0xFF. `admgsym sidecar <elf> <rom> <out.admgsym>` writes the same entries and names to their own
file instead, with the ROM CRC-32 taken over the whole ROM, leaving the ROM as it is.

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

## When you change a script macro or a script command's arguments

Content IDs (abilities, species, moves, items, map IDs) are 32 bits here. A macro that writes one at another width than
its command reads makes every command after it read the wrong bytes. `make check` runs this first; by itself:

```bash
make check-script-widths
```

A case it can't decide, like a command whose reads sit behind flags, goes in `tools/admg101/script-widths-reviewed.json`
with the reason it is correct (see [UPSTREAM-MERGE.md](UPSTREAM-MERGE.md), section 6).

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

## The release patch

A build is released as a BPS patch from the retail Pokémon Emerald (USA, Europe) ROM, attached to the build's GitHub
release (tag `build-N`). Never the ROM or the ELF. Make it after building, from your own retail ROM:

```bash
bash tools/admg101/make_bps.sh <retail Emerald ROM> pokeemerald.gba emerald-expansion-build-N.bps
```

It refuses a retail ROM or build with the wrong SHA-1, builds Floating IPS from its source at a pinned commit, and checks
that the patch applied to the retail ROM gives the build exactly. `tools/admg101/rom-sha1.txt` is the build's SHA-1; the
ADMG-101 build workflow (`.github/workflows/admg101-build.yml`) checks every push still builds it, in the Ubuntu 26.04 image
with the toolchain packages pinned. Update `rom-sha1.txt` in the same commit as any change that alters the ROM.

## Merging upstream

See [UPSTREAM-MERGE.md](UPSTREAM-MERGE.md).
