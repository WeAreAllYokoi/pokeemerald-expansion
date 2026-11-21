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

`make ADMG_SYMBOLS=1` ends the build by running `tools/admgsym` on the ROM. It writes the ELF's named symbols
(functions, data objects and labels, with names, addresses, sizes and ARM or Thumb, in ROM and RAM) into the 0xFF padding
after the game as an ADMGSYM1 table, with a footer in the ROM's last 16 bytes. ADMG-101's Ghidra loader
(gba-ghidra-loader) and ADMG-101's preparation read it, so a build of this fork can be prepared without its ELF. The format is
described at the top of `tools/admgsym/admgsym.c`.

The table carries the tables mods can move too. `tools/admg101/relocatable-symbols.txt` lists them, in the order
ADMG-101 numbers them (ADMG-101's `tools/recipe/relocatable-symbols.txt` holds the same list); the Makefile passes it
as `--relocatable`. The table numbers each one from 1 and keeps the relocations ADMG-101's recompiler reads for them:
1,044 of the 218,526 R_ARM_ABS32 relocations the recompiler's symbol file carries for build 40. A name in the list has
to be exactly one data object with a size, and the list has to keep at least one relocation, or the build fails. Build 40's table takes 1.25 MiB of
its 2.52 MiB of padding.

The table also names the code the ELF calls but has no function symbol for: GCC's `bx Rm` call thunks (as
`_call_via_<register>_<address>`) and crt0's code labels. And it records coverage ranges: the parts of the ROM where its
functions are every function there is, worked out from the ELF's `$a`, `$t` and `$d` mapping symbols. The loader then
lets no guess start a function inside them. Multiboot images with a cartridge header and anything given with
`--not-covered <symbol>` stay outside coverage, as does any code the tool finds in no function (other than padding),
of which it prints the first ten stretches when it runs.

It is off by default until the next build, because it changes the ROM's bytes: with it on, the ROM isn't build 40.
Changing the value rebuilds the ROM. The build fails, and the ROM is deleted, if the table doesn't fit in the padding
or the padding isn't all 0xFF. `admgsym sidecar [--relocatable <list>] <elf> <rom> <out.admgsym>` writes the same
table to its own file instead, with the ROM CRC-32 taken over the whole ROM, leaving the ROM as it is.

## Going past 32 MiB (ADMG-101 only)

A GBA cartridge holds at most 32 MiB. On ADMG-101 a hack can have more: data the linker puts in the extended ROM, a
region at `0x40000000` of up to 256 MiB that ADMG-101 is to map when the game starts. The build then writes
`pokeemerald.admgrom` instead of `pokeemerald.gba`: the 32 MiB cartridge image, then the extended ROM's data. Only
ADMG-101 will run it, and ADMG-101 doesn't read `.admgrom` files yet. No GBA or emulator can run it, and the build's
tests, which run on mGBA, still link everything into the cartridge.

Move whole groups of data with `ADMG_ROMX`:

```bash
make -j$(nproc) ADMG_ROMX="sound pokemon graphics maps"
```

| Group | What moves | MiB in build 40 |
|---|---|---|
| `sound` | `data/sound_data.o` (voice groups, samples, the song and music player tables) and the songs | 10.64 |
| `pokemon` | `src/pokemon.o`'s data: the species pictures and palettes, and the species tables | 7.04 |
| `graphics` | `src/graphics.o`, the overworld object graphics (`src/event_object_movement.o`) and the fonts | 2.09 |
| `maps` | Map layouts and headers (`data/maps.o`), map events, tilesets and animated tiles | 2.52 |
| `extended` | Nothing more: only the objects marked `EXTENDED_DATA`, which every `ADMG_ROMX` build moves | 0 |

Each group is a file of linker input-section rules in `tools/admg101/romx-groups/`, which the Makefile hands to
`ld_script_modern.ld`'s `.romx` section. A group moves an object's `.rodata` whole, because the build doesn't compile
with `-fdata-sections`. That's also why `ADMG_ROMX` can't be used with LTO, which `RELEASE=1` turns on: LTO compiles
with `-fdata-sections` and links objects of its own, so the groups' rules would match nothing. String literals and
compound strings stay in the cartridge: GCC puts string literals in `.rodata.str*` when it merges constants, which
every build here does (the Makefile passes `-fmerge-constants`, so `-O0` builds do too). They have to stay, because
the heap records where a block was allocated as a 25-bit offset from the start of the cartridge (`include/malloc.h`),
and those locations are string literals.

`pokemon` moves `gSpeciesInfo`, one of ADMG-101's relocatable tables, with the pictures, so it is its own group.
Event scripts (`data/event_scripts.o`) and all code stay in the cartridge. For data a group doesn't cover, mark the
object `EXTENDED_DATA` (`include/gba/defines.h`), as `NATIVE_DATA` marks native memory. The extended ROM is read-only,
so the object must be `const`. A build whose ELF has extended ROM data can't make a `.gba`, so it fails unless
`ADMG_ROMX` is set: `ADMG_ROMX=extended` moves only the marked objects.

`tools/admgext` writes the `.admgrom` and refuses code in the extended ROM (an executable section, a function or an
`$a`/`$t` mapping symbol there) and writable data. Its refusals and the format are described at the top of
`tools/admgext/admgext.c`. With `ADMG_ROMX` empty and no `EXTENDED_DATA`, the default, the build writes the same `.gba`
as before, byte for byte. `ADMG_SYMBOLS=1` can't be combined with it yet.

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

For a build with `ADMG_ROMX`, give it `pokeemerald.admgrom` instead, and record the `.admgrom`'s SHA-1 in
`rom-sha1.txt`. The build workflow below then has to build with the same `ADMG_ROMX`.

It refuses a retail ROM or build with the wrong SHA-1, builds Floating IPS from its source at a pinned commit, and checks
that the patch applied to the retail ROM gives the build exactly. `tools/admg101/rom-sha1.txt` is the build's SHA-1; the
ADMG-101 build workflow (`.github/workflows/admg101-build.yml`) checks every push still builds it, in the Ubuntu 26.04 image
with the toolchain packages pinned. Update `rom-sha1.txt` in the same commit as any change that alters the ROM.

## Merging upstream

See [UPSTREAM-MERGE.md](UPSTREAM-MERGE.md).
