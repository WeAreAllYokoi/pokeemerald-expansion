#!/usr/bin/env python3

"""
Usage: python3 make_teachable.py SOURCE_DIR

Build a C-header defining the set of teachable moves for each configured-on
species-family based on the learnable moves defined in SOURCE_DIR/all_learnables.json.

A move is "teachable" if it is:
    1. Can be taught by some Move Tutor in the overworld, which is identified by
       using the ChooseMonForMoveTutor special in a script and setting VAR_0x8005
       to the offered MOVE constant. (e.g., MOVE_SWAGGER)
    2. Assigned to some TM or HM in include/constants/tms_hms.h using the
       FOREACH_TM macro.
    3. A universal move, as defined by universalMoves in
       src/data/pokemon/special_movesets.json

For a given species, a move is considered teachable to that species if:
    1. The species can learn the move via *any* method within any Expansion-
       supported game.
    2. The species rule defined by their teachingType in the species_info folder
"""

from itertools import chain
from textwrap import dedent

import json
import pathlib
import re
import sys
import typing


CONFIG_ENABLED_PAT = re.compile(r"^#define P_LEARNSET_HELPER_TEACHABLE\s+(?P<cfg_val>[^ ]*)", flags=re.MULTILINE)
# Read the build defaults for options that can change at runtime
ALPHABETICAL_ORDER_ENABLED_PAT = re.compile(r"^#define (?:ADMG_DEFAULT_)?HGSS_SORT_TMS_BY_NUM\s+(?P<cfg_val>[^ ]*)", flags=re.MULTILINE)
TM_LITERACY_PAT = re.compile(r"^#define (?:ADMG_DEFAULT_)?P_TM_LITERACY\s+GEN_(?P<cfg_val>[^ ]*)", flags=re.MULTILINE)
TMHM_MACRO_PAT = re.compile(r"F\((\w+)\)")
SNAKIFY_PAT = re.compile(r"(?!^)([A-Z]+)")

def enabled() -> bool:
    """
    Check if the user has explicitly enabled this opt-in helper.
    """
    with open("./include/config/pokemon.h", "r") as cfg_pokemon_fp:
        cfg_pokemon = cfg_pokemon_fp.read()
        cfg_defined = CONFIG_ENABLED_PAT.search(cfg_pokemon)
        return cfg_defined is not None and cfg_defined.group("cfg_val") in ("TRUE", "1")

def extract_repo_tms() -> typing.Generator[str, None, None]:
    """
    Yield MOVE constants assigned to a TM or HM in the user's repo.
    """
    with open("./include/constants/tms_hms.h", "r") as tmshms_fp:
        tmshms = tmshms_fp.read()
        match_it = TMHM_MACRO_PAT.finditer(tmshms)
        if not match_it:
            return

        for match in match_it:
            yield f"MOVE_{match.group(1)}"

def extract_tm_litteracy_config() -> bool:
    config = False
    with open("./include/config/pokemon.h", "r") as cfg_pokemon_fp:
        cfg_pokemon = cfg_pokemon_fp.read()
        cfg_defined = TM_LITERACY_PAT.search(cfg_pokemon)
        if cfg_defined:
            cfg_val = cfg_defined.group("cfg_val")
            if ((cfg_val == "LATEST") or (int(cfg_val) > 6)):
                config = True
    return config

def species_teachables(species_data, tms: list[str], tutors: list[str], special_movesets, all_learnables, tm_litteracy_config: bool) -> list[str]:
    """
    One species-family's teachable moves for a TM order and a P_TM_LITERACY rule.
    """
    species = species_data["name"]
    teaching_type = species_data["teaching_type"]
    species_upper =  SNAKIFY_PAT.sub(r"_\1", species).upper()
    if teaching_type == "ALL_TEACHABLES":
        part1 = list(filter(lambda m: m not in special_movesets["signatureTeachables"], tms))
        part2 = list(filter(lambda m: m not in special_movesets["signatureTeachables"], tutors))
    else:
        if teaching_type == "TM_ILLITERATE":
            learnables = all_learnables[species_upper]
            if not tm_litteracy_config:
                learnables = filter(lambda m: m not in special_movesets["universalMoves"], learnables)
        else:
            learnables = all_learnables[species_upper] + special_movesets["universalMoves"]
        part1 = list(filter(lambda m: m in learnables, tms))
        part2 = list(filter(lambda m: m in learnables, tutors))

    repo_species_teachables = part1 + part2
    if species_upper == "TERAPAGOS":
         repo_species_teachables = filter(lambda m: m != "MOVE_TERA_BLAST", repo_species_teachables)

    return list(dict.fromkeys(repo_species_teachables))

def prepare_output(all_learnables: dict[str, set[str]], tms_by_num: list[str], tutors: list[str], special_movesets, repo_teaching_types, header: str, sort_tms_by_num: bool) -> str:
    """
    Build teachable_learnsets.h for both TM orders and TM-literacy settings.

    Keep the original name for the configured defaults and reuse arrays when
    variants match. sTeachableLearnsetVariants maps the choices for
    GetSpeciesTeachableLearnset, indexed by [sort by TM number][TM literacy].
    """

    tm_litteracy_config = extract_tm_litteracy_config()
    tm_orders = (sorted(tms_by_num), list(tms_by_num))  # [FALSE: alphabetical, TRUE: TM number]
    source_key = (int(sort_tms_by_num), int(tm_litteracy_config))

    cursor = 0
    new = header + dedent("""
    static const enum Move sNoneTeachableLearnset[] = {
        MOVE_UNAVAILABLE,
    };
    """)

    def array(name, moves):
        joinpat = ",\n    "
        return f"static const enum Move {name}[] = " + "{\n" + "\n".join([
            f"    {joinpat.join(chain(moves, ('MOVE_UNAVAILABLE',)))},",
            "};\n",
        ])

    variant_rows = []
    for species_data in repo_teaching_types:
        if isinstance(species_data, str):
            new += (species_data)
            continue
        species = species_data["name"]
        source_name = f"s{species}TeachableLearnset"
        variants = {}
        for sort_key in (0, 1):
            for literate_key in (0, 1):
                variants[(sort_key, literate_key)] = species_teachables(species_data, tm_orders[sort_key], tutors, special_movesets, all_learnables, bool(literate_key))
        new += array(source_name, variants[source_key])
        names = {tuple(variants[source_key]): source_name}
        for key in sorted(variants):
            moves = tuple(variants[key])
            if moves not in names:
                names[moves] = f"{source_name}_Sort{key[0]}Literate{key[1]}"
                new += array(names[moves], moves)
        row = [names[tuple(variants[key])] for key in sorted(variants)]
        if any(name != source_name for name in row):
            variant_rows.append((source_name, row))

    new += dedent("""
    #define ADMG_TEACHABLE_LEARNSET_VARIANTS
    struct TeachableLearnsetVariants
    {
        const enum Move *source;
        const enum Move *variants[2][2]; // [HGSS_SORT_TMS_BY_NUM][P_TM_LITERACY > GEN_6]
    };

    static const struct TeachableLearnsetVariants sTeachableLearnsetVariants[] =
    {
    """)
    for source_name, row in variant_rows:
        new += f"    {{ {source_name}, {{ {{ {row[0]}, {row[1]} }}, {{ {row[2]}, {row[3]} }} }} }},\n"
    new += "};\n"

    return new

def prepare_header(h_align: int, tmshms: list[str], tutors: list[str], universals: list[str]) -> str:
    universals_title = "Near-universal moves found in data/special_movesets.json:"
    tmhm_title = "TM/HM moves found in \"include/constants/tms_hms.h\":"
    tutor_title = "Tutor moves found from map scripts:"
    h_align = max(h_align, len(universals_title), len(tmhm_title), len(tutor_title))

    lines = [
         "//",
         "// DO NOT MODIFY THIS FILE! It is auto-generated by tools/learnset_helpers/make_teachables.py",
         "//",
         "",
        f"// {'*' * h_align} //",
        f"// {tmhm_title: >{h_align}} //",
    ]
    lines.extend([f"// - {move: <{h_align - 2}} //" for move in tmshms])
    lines.extend([
        f"// {'*' * h_align} //",
        f"// {tutor_title: <{h_align}} //",
    ])
    lines.extend([f"// - {move: <{h_align - 2}} //" for move in sorted(tutors)])
    lines.extend([
        f"// {'*' * h_align} //",
        f"// {universals_title: <{h_align}} //",
    ])
    lines.extend([f"// - {move: <{h_align - 2}} //" for move in universals])
    lines.extend([
        f"// {'*' * h_align} //",
         "",
    ])

    return "\n".join(lines)

def create_tutor_moves_array(tutors):
    """
    Generate gTutorMoves[].
    """
    header = dedent("""\
        // DO NOT MODIFY THIS FILE! It is auto-generated by tools/learnset_helpers/make_teachables.py
        // Needed by tutor moves relearner!

        const enum Move gTutorMoves[] = {
    """)

    lines = [f"    {move}," for move in tutors]
    lines.append("    MOVE_UNAVAILABLE\n};\n")
    with open("./src/data/tutor_moves.h", "w") as f:
        f.write(header + "\n".join(lines))

def make_move_tutors(build_dir, special_movesets):
    SOURCE_TUTORS_JSON = build_dir / "all_tutors.json"

    assert SOURCE_TUTORS_JSON.exists(), f"{SOURCE_TUTORS_JSON=} does not exist"
    assert SOURCE_TUTORS_JSON.is_file(), f"{SOURCE_TUTORS_JSON=} is not a file"

    with open(SOURCE_TUTORS_JSON, "r") as fp:
        repo_tutors = json.load(fp)

    repo_tutors = sorted(list(set(repo_tutors + special_movesets["extraTutors"])))
    create_tutor_moves_array(repo_tutors)

    return repo_tutors

def main():
    if not enabled():
        quit()

    tutor_mode = False
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        print("Invalid number of arguments", file=sys.stderr)
        print(__doc__, file=sys.stderr)
        quit(1)

    if len(sys.argv) == 3:
        if sys.argv[1] != "--tutors":
            print("Unknown make_teachables mode", file=sys.stderr)
            quit(1)
        tutor_mode = True

    SOURCE_DIR = pathlib.Path(sys.argv[-1])

    with open("src/data/pokemon/special_movesets.json", "r") as file:
        special_movesets = json.load(file)

    repo_tutors = make_move_tutors(SOURCE_DIR, special_movesets)
    if tutor_mode:
        quit(0)

    SOURCE_LEARNSETS_JSON = pathlib.Path("./src/data/pokemon/all_learnables.json")
    SOURCE_TEACHING_TYPES_JSON = SOURCE_DIR / "all_teaching_types.json"

    assert SOURCE_LEARNSETS_JSON.exists(), f"{SOURCE_LEARNSETS_JSON=} does not exist"
    assert SOURCE_LEARNSETS_JSON.is_file(), f"{SOURCE_LEARNSETS_JSON=} is not a file"

    assert SOURCE_TEACHING_TYPES_JSON.exists(), f"{SOURCE_TEACHING_TYPES_JSON=} does not exist"
    assert SOURCE_TEACHING_TYPES_JSON.is_file(), f"{SOURCE_TEACHING_TYPES_JSON=} is not a file"

    repo_tms = list(extract_repo_tms())
    repo_tms_by_num = list(repo_tms)

    with open("./include/config/pokedex_plus_hgss.h", "r") as cfg_pokemon_fp:
        cfg_pokemon = cfg_pokemon_fp.read()
        cfg_defined = ALPHABETICAL_ORDER_ENABLED_PAT.search(cfg_pokemon)
        sort_tms_by_num = not (cfg_defined is None or cfg_defined.group("cfg_val") in ("FALSE", "0"))
        if not sort_tms_by_num:
            repo_tms = sorted(repo_tms)

    h_align = max(map(lambda move: len(move), chain(special_movesets["universalMoves"], repo_tms, repo_tutors))) + 2
    header = prepare_header(h_align, repo_tms, repo_tutors, special_movesets["universalMoves"])

    with open(SOURCE_LEARNSETS_JSON, "r") as source_fp:
        all_learnables = json.load(source_fp)

    with open(SOURCE_TEACHING_TYPES_JSON, "r") as source_fp:
        repo_teaching_types = json.load(source_fp)

    content = prepare_output(all_learnables, repo_tms_by_num, repo_tutors, special_movesets, repo_teaching_types, header, sort_tms_by_num)
    with open("./src/data/pokemon/teachable_learnsets.h", "w") as teachables_fp:
        teachables_fp.write(content)

if __name__ == "__main__":
    main()
