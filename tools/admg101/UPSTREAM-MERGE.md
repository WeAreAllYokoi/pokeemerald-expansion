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
make check -j$(nproc)                                            # upstream's test suite, compared with admg-101 unmerged
```

`make check` does not compile on build 39 itself: test builds (`TESTING`) leave out the run-time selector, so
`ADMG_CONFIG_EXTRA` and `ADMG_GetConfig` are undeclared (`src/battle_ai_util.c`, `src/battle_anim.c`). Until that is
fixed it can't gate a merge; check that the merge fails the same way as `admg-101` unmerged, no worse.

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
