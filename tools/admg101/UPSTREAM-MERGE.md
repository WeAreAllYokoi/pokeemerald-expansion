# Merging upstream Emerald Expansion

How to bring a new upstream release into this fork. It was first done for 1.17.2 (recipe 40, November 2026); the
examples below are from that merge.

## What makes this fork different

Upstream makes features compile-time choices for ROM hackers: edit `include/config/*.h`, rebuild. This fork makes every
one of them a run-time selection in one built game, so ADMG-101's mods can choose them. So a merge is never "take
upstream's side": whatever upstream adds has to keep every option selectable at run time. The rules are below.

## 1. Set up

- Work in a clone of `WeAreAllYokoi/pokeemerald-expansion`. Branches: `admg-101` is the build line (what the current
  build is made from), `master` mirrors upstream's `master`, and each new build gets its own branch (`recipe-40`).
- Commit as the fork's owner, set in the clone only, never globally:
  `git config user.name WeAreAllYokoi` and
  `git config user.email 349864965+WeAreAllYokoi@users.noreply.github.com`. Check with `git config --show-origin user.name`.
- `.gitattributes` keeps sources LF; a `core.autocrlf=true` checkout is fine.
- Build in WSL or Linux (see [BUILDING.md](BUILDING.md)).

## 2. Prove the current build first

Build `admg-101` unchanged and check the ROM's SHA-1 against the current build's (build 39:
`cb305c76bd7337d02ef55c491c2cb1b2e9a10fe3`). If it doesn't match, something the build needs is missing: find it before
merging anything.

## 3. Review upstream before merging

- Update `master` from upstream, and find the base: `git merge-base admg-101 master`.
- List upstream's commits and files: `gh api repos/rh-hideout/pokeemerald-expansion/compare/<base>...master`.
- Overlap: the files both sides changed, `git diff --name-only <base> admg-101` against upstream's list. Heavy overlap
  is where conflicts and re-routing will be.
- For each commit, a table: what kind (bug fix, feature, graphics, tests, docs, release bookkeeping), what it fixes in
  one sentence, which of our files it touches, and the risk (none, low, medium, high, with the reason).
- Look for anything that changes a struct, a bitfield, a function signature, a constant, or a save format: those change
  ADMG-101's API descriptor (list them for the ADMG-101 side).
- Check our own unmerged fix branches against upstream's files (for 1.17.2: `admg101/newgame-clear-extended`).

## 4. The merge rules

No conflict is resolved by taking upstream's compile-time form:

- **Route every new compile-time read of an option** through the run-time selector, as the rest of the source does:
  `GetConfig(NAME)` for the configuration table, `ADMG_GetFeaturePolicy` for policies. In 1.17.2:
  `B_RESTORE_HELD_BATTLE_ITEMS`, `B_TRAINERS_KNOCK_OFF_ITEMS`, `B_RETURN_STOLEN_NPC_ITEMS`, and `B_HP_PERCENTAGE_DISPLAY`
  (which is the policy `ADMG_POLICY_ENEMY_HP_PERCENTAGE`).
- **Where we choose at run time and upstream at compile time, ours stays** and only upstream's fix comes in. In 1.17.2:
  the Pokédex fix tested `POKEDEX_PLUS_HGSS`; the dex is `ADMG_POLICY_HGSS_POKEDEX`, so the null test stayed.
- **Data stays built.** Upstream guards data with its option macros; this fork builds the data and chooses at run
  time. Translate upstream's guards to ours: `OW_POKEMON_OBJECT_EVENTS` is `ADMG_DEFAULT_OW_POKEMON_OBJECT_EVENTS`,
  `OW_PKMN_OBJECTS_SHARE_PALETTES` is `ADMG_DEFAULT_OW_PKMN_OBJECTS_SHARE_PALETTES`, `P_FOOTPRINTS` is
  `ADMG_DEFAULT_P_FOOTPRINTS`, and `OW_BATTLE_ONLY_FORMS` guards are removed (it is a run-time selection,
  `ADMG_CONFIG_EXTRA`, so it can't be a preprocessor guard). Search the whole merge for them, not only the conflicts:
  in 1.17.2, 18 `OW_BATTLE_ONLY_FORMS` guards arrived in files that merged without a conflict.
- **A new upstream option becomes a new run-time option** (a selector tag, policy or setting), never a baked-in value.
- **Re-express upstream's fix inside our structure** where our code was rewritten: in 1.17.2 the Factory rental fix went
  into our rental backing, and the approach-music table into our state-access check, keeping the old default for an
  out-of-range value.

## 5. Merge, don't cherry-pick

`git switch -c recipe-NN admg-101`, merge our pending fix branches, then `git merge --no-ff master`. A real merge moves
the merge base, so the next sync starts from here instead of meeting the same conflicts again, and the history says
which upstream the build is. Every conflict is resolved once either way.

## 6. The gates

All of these must pass before the merge is committed:

```bash
python3 -B tools/admg101/audit_r38_config_reads.py . --check     # no direct code reads of an option
python3 -B tools/admg101/route_r38_config_extra.py --check .     # the extra routes header is current
make -j$(nproc)                                                  # the game builds
make check -j$(nproc)                                            # script widths, then upstream's test suite
```

`make check` compiles and links from recipe 40 on, and the test ELF uses the game's own memory layout: the native region
at 0x10000000 (`.native_bss`: the 16 MB heap, the parties, the save blocks, the PC storage) is in `ld_script_test.ld`
as it is in `ld_script_modern.ld`. Test builds get no layout or data of their own; the only test branches are upstream's
harness (test flags and vars, the config test override, allocation counters, fixtures). Keep it that way: never shrink
memory or drop data to make a test build fit.

Stock mGBA has no memory at 0x10000000, so the suite does not run under `mgba-rom-test`: the first allocation fails
(`ASSERTION FAILED src/malloc.c:97`, then `Illegal opcode`) and the workers flood their output. It runs natively instead:
the test ELF is recompiled by GBARecomp and run in ADMG-101's runtime, which maps the native region (FU-6 part 2; the
runner is still a proof, not yet a tool here). Upstream's runner then reports through its own protocol (`:N`, `:P`, `:F`)
and exits with its own code.

### What the native run found

The first native run (recipe 40, November 2026) failed `test/battle/weather/rain.c`'s "Drizzle fails if Desolate Land
is active" with `wrong use of printselectionstringfromtable`. The cause was a bug in the shipped game, build 39
included, not in the runtime: this fork widened content IDs to 32 bits (`enum Ability`, `enum Species`, `enum Move`,
`enum Item`, `MapId`, ...), and some code still wrote or read them at the old width.

- Battle-script macros `jumpifability`, `jumpifabilitypresent` and `jumpifspecies` wrote the ID as `.2byte`, while
  their commands read a 4-byte field (`CMD_ARGS` and `NATIVE_ARGS` are packed structs over the script bytes). The jump
  target and the next command were read 2 bytes off: Groudon's Drought script ran garbage. `jumpifability` is used 36
  times in the battle scripts.
- Event-script macros `showplayer` and `hideplayer` wrote the map as two `.byte`s, while `ScrCmd_showobjectat` and
  `ScrCmd_hideobjectat` read a 4-byte `MapId`: the next command after any of their 17 uses was read 2 bytes off.
- `GetBoxMonData(..., MON_DATA_KNOWN_MOVES, ...)` walked its move list as `u16`, while every caller passes `enum Move`
  lists: the PC's "don't release the last Pokémon that knows a needed move" check read the wrong moves.
- The Battle Pyramid bag copied its 32-bit `itemId` array through a `u16` buffer of half the size (`src/item.c`,
  `src/battle_pyramid_bag.c`), scrambling the bag.
- Upstream's test helper `Moves_` passed a `u16` array to the 32-bit move field, so each move took the next one as its
  upper half (all of `strong_winds.c` was INVALID).

Running the whole suite (807 files) then found more of the same kinds, and some upstream bugs that only show natively:

- `gBattleScripting`'s hand-written script offsets (`sMOVE_EFFECT` ... `sSTICKY_WEB_STAT_DROP`) were stale after
  `moveEffect` and `abilityPopupOverwrite` became 32-bit: scripts wrote into the wrong fields. Move effects set by a
  script never applied (Flatter's and Swagger's confusion), Sticky Web set the next ability pop-up to ability 1, and
  ability replacement showed the wrong pop-ups. About 80 failing tests had this one cause.
- 16-bit sentinels left behind: Mimic set the chosen move to 0xFFFF and Beat Up tested for 0xFFFF, where moves and
  species are now 32-bit (`MOVE_UNAVAILABLE`, `SPECIES_ID_MAX`).
- Recorded battles: the fork's moveset check refused every turn of a played-back battle (646 timeouts); the recorded
  controllers indexed the moves with the gimmick flag still set (upstream too).
- A Soundproof check on the absent partner in singles, and Shadow Force's and Phantom Force's animations playing sound
  `SOUND_PAN_ATTACKER` (upstream typos; natively the bad song pointer faults).
- Test harness: the leak check assumed the heap is in EWRAM; text printers outlived a failed test across the per-test
  heap reset; tests pinned upstream's save sizes, BoxPokemon bytes and AI frame counts.
- Gooey Mulch's mulch-usage description, which upstream's default build never tests, was too wide for the bag.

All are fixed in recipe 40. The fixes change the ROM. With them, the suite runs natively with every test passing,
except the one below.

### Known limitation: crash recovery

`test/test_runner.c` and upstream's runner recover from a crash by restarting the game: a test that crashes (for
example by calling a NULL function, as `test_test_runner.c`'s "Tests resume after CRASH" does on purpose) jumps to
0x00000000, the GBA's reset vector, and the runner resumes after the crashed test from `.persistent` IWRAM. ADMG-101's
runtime treats a jump to 0x00000000 as an error, not a reset, so the run stops there and the rest of that file doesn't
run. This is not a game bug: it needs the runtime to reset on a jump to address 0 (GBAModernRuntime). Until then that
file stops at its crash test.

The checks after a merge or any ID widening:

- `make check-script-widths` (also run by `make check`; `tools/admg101/check_script_widths.py`): every battle-script
  macro against the packed struct its command reads, with each `CMD_ARGS`/`NATIVE_ARGS` field type measured by the
  build's own compiler, and every event-script macro, nested macros expanded, against the order and width of its
  command's `ScriptRead*` calls. A case it cannot decide (bitfields, reads in loops or behind flags, a helper that reads
  the bytes) passes only as listed in `script-widths-reviewed.json` with the same signature and a reason; a changed one
  fails until it is reviewed again (`--write-reviewed` records the current cases, keeping their reasons). Recipe 40:
  377 battle-script and 313 event-script macros, 12 reviewed by hand.
- `SetMonData`/`SetBoxMonData`: `CHECK_MON_DATA_ARG` stops the build for a narrow pointer to an ID field, but only when
  the field is a constant. A field like `MON_DATA_MOVE1 + i` needs the guard extended temporarily to computed fields,
  and its hits read by hand.
- `gBattleScripting` offsets: `STATIC_ASSERT`s in `src/battle_script_commands.c` check every hand-written offset in
  `include/constants/battle_script_commands.h` against the struct, and the multi-byte fields' sizes, so a field that is
  widened or moved fails the build until the offsets and the scripts' write widths follow.
- Animation scripts carry no content IDs; their commands read explicit widths.

This is why the suite runs natively: it ran the game's own battle code and found bugs in the shipped game that no
compile, link or static gate had caught.

History, so it isn't rediscovered: build 39 didn't compile `make check`. Commit `05853bf370` moved the enlarged data to
native memory and updated only `ld_script_modern.ld`, an oversight; test builds also left out the run-time selector.
The old workspace's test checks were bounded ARM compiles of single source units, with no link or run, so upstream's
runner had not run since. Recipe 40 fixed the compile (a test `ADMG_GetConfig` that returns each option's default) and
the link.

Then on the ADMG-101 side, its configuration inventory with `ADMG_GAME_TOOLS` set to this folder:
`tools/recipe/inventory_r38_config.py <this clone> --check`. Its status counts should match the previous build's,
with nothing half-applied. Also search the merged diff for upstream compile-time guards that don't use our names (rule 3).

If the audit's catalog changed, regenerate the two committed headers (`--emit`, and the router without `--check`) and
commit them with the merge.

## 7. The merge commit

List every conflict and every change made to auto-merged code: the file, what was done, and which rule it follows.
Upstream bugs found during the review that we fix go in commits of their own after the merge (1.17.2: Cud Chew's 2-bit
timer wrapped from 0 to 3).

## 8. Push

The fork is public. Push only after the owner has seen the diff and the review.

## 9. ADMG-101's side

The new build then becomes a recipe in ADMG-101: symbols, measured config variants, the API descriptor, regenerated SDK
views, the target profile and ROM hash. See ADMG-101's `docs/admg101-new-build.md`.
