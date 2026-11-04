#!/usr/bin/env python3
"""F3.3a map identity audit: every place a build keeps, passes or encodes a map identity, and what limits map content.

  map_identity.py <DINFO ELF> <source tree> <out.json> <out.md>

Map identities in the decomp are an 8-bit group and an 8-bit number (MAP_* constants are group << 8 | num). This lists:
  - DWARF: struct/union members, globals, function parameters and locals whose name is a map group, number or ID, every
    member or global of a type that holds one (struct WarpData), and which of them the native save reaches (the durable
    roots: SaveBlock1-3, PokemonStorage, HallofFameTeam, TrainerHillChallengeSave, RecordedBattleSave);
  - source: MAP_GROUP()/MAP_NUM() uses, the calls that turn a group and number into a map (gMapGroups,
    Overworld_GetMapHeaderByGroupAndId, SetWarpDestination*), the script macros whose bytecode carries a map and how often
    the build's scripts use each, the map identity fields tools/mapjson generates, and the capacity constants of map content.
Nothing is changed. Run it in WSL with the audit's Python (pyelftools): see run_map_identity.sh.
"""
import collections
import json
import os
import re
import sys

from elftools.elf.elffile import ELFFile

elf_path, tree, out_json, out_md = sys.argv[1:5]

# Names of a map group, number or whole ID, as the decomp spells them (mapGroup, destMapNum, targetMapGroup, locationMapNum...)
MAP_NAME = re.compile(r"^(?:\w*[Mm]ap(?:Group|Num|Id|ID)\w*|mapGroup|mapNum|mapId)$")
DURABLE_ROOTS = ["SaveBlock1", "SaveBlock2", "SaveBlock3", "PokemonStorage", "HallofFameTeam", "TrainerHillChallengeSave", "RecordedBattleSave"]
CARRIER_STRUCTS = {"WarpData"}  # whole-type map carriers: every member or global of this type keeps a map identity


def attr(die, name, default=None):
    a = die.attributes.get(name)
    return a.value if a is not None else default


def name_of(die):
    n = attr(die, "DW_AT_name")
    return n.decode() if isinstance(n, bytes) else n


def strip(die):
    while die is not None and die.tag in ("DW_TAG_typedef", "DW_TAG_const_type", "DW_TAG_volatile_type"):
        die = die.get_DIE_from_attribute("DW_AT_type") if "DW_AT_type" in die.attributes else None
    return die


def describe(die, depth=0):
    """(text, kind, byte size, struct name or None)"""
    if die is None:
        return ("void", "void", 0, None)
    tag = die.tag
    if tag in ("DW_TAG_typedef", "DW_TAG_const_type", "DW_TAG_volatile_type"):
        inner = die.get_DIE_from_attribute("DW_AT_type") if "DW_AT_type" in die.attributes else None
        t, k, s, st = describe(inner, depth + 1)
        return (name_of(die) if tag == "DW_TAG_typedef" else t, k, s, st)
    if tag == "DW_TAG_base_type":
        return (name_of(die), "int", attr(die, "DW_AT_byte_size", 0), None)
    if tag == "DW_TAG_enumeration_type":
        return (f"enum {name_of(die)}", "enum", attr(die, "DW_AT_byte_size", 0), None)
    if tag == "DW_TAG_array_type":
        t, k, s, st = describe(die.get_DIE_from_attribute("DW_AT_type"), depth + 1)
        dims = [attr(c, "DW_AT_upper_bound") for c in die.iter_children() if c.tag == "DW_TAG_subrange_type"]
        return (t + "".join(f"[{d + 1 if isinstance(d, int) else ''}]" for d in dims), "array:" + k, s, st)
    if tag == "DW_TAG_pointer_type":
        if "DW_AT_type" in die.attributes and depth < 3:
            t, k, s, st = describe(die.get_DIE_from_attribute("DW_AT_type"), depth + 1)
            return (t + " *", "ptr:" + k, 4, None)
        return ("ptr", "ptr", 4, None)
    if tag in ("DW_TAG_structure_type", "DW_TAG_union_type"):
        return (f"struct {name_of(die)}", "struct", attr(die, "DW_AT_byte_size", 0), name_of(die))
    return (tag, tag, 0, None)


def array_element_struct(die):
    """The struct an array or plain struct member holds, through typedefs and arrays."""
    die = strip(die)
    while die is not None and die.tag == "DW_TAG_array_type":
        die = strip(die.get_DIE_from_attribute("DW_AT_type"))
    if die is not None and die.tag in ("DW_TAG_structure_type", "DW_TAG_union_type"):
        return name_of(die)
    return None


def file_of(cu, die, cache):
    """Where a DIE is declared: its compilation unit and declaration line. (The unit's line program is not parsed: with
    pyelftools on CPython 3.14 that parse failed with a TypeError and then a segfault.)"""
    line = attr(die, "DW_AT_decl_line")
    if line is None:
        return None
    unit = cache.get(cu.cu_offset)
    if unit is None:
        unit = cache[cu.cu_offset] = name_of(cu.get_top_DIE())
    return f"{unit} (decl line {line})"


members, globals_, params, locals_ = {}, {}, {}, collections.Counter()
struct_members = collections.defaultdict(dict)  # struct -> member -> struct it holds (for save reachability)
fcache = {}
with open(elf_path, "rb") as f:
    dwarf = ELFFile(f).get_dwarf_info()
    for cu in dwarf.iter_CUs():
        top = cu.get_top_DIE()
        for die in cu.iter_DIEs():
            tag = die.tag
            if tag in ("DW_TAG_structure_type", "DW_TAG_union_type") and name_of(die) and not attr(die, "DW_AT_declaration"):
                sname = name_of(die)
                for m in die.iter_children():
                    if m.tag != "DW_TAG_member":
                        continue
                    mname = name_of(m) or "<anon>"
                    t = m.get_DIE_from_attribute("DW_AT_type") if "DW_AT_type" in m.attributes else None
                    held = array_element_struct(t)
                    if held:
                        struct_members[sname][mname] = held
                    text, kind, size, st = describe(t)
                    if MAP_NAME.match(mname) or held in CARRIER_STRUCTS:
                        key = (sname, mname)
                        if key not in members:
                            members[key] = {"struct": sname, "member": mname, "type": text, "size": size,
                                            "bits": attr(m, "DW_AT_bit_size"), "where": file_of(cu, m, fcache),
                                            "why": "carrier type" if held in CARRIER_STRUCTS else "name"}
            elif tag == "DW_TAG_variable" and die.get_parent() is top and name_of(die) and "DW_AT_type" in die.attributes:
                vname = name_of(die)
                t = die.get_DIE_from_attribute("DW_AT_type")
                held = array_element_struct(t)
                if (MAP_NAME.match(vname) or held in CARRIER_STRUCTS) and vname not in globals_:
                    text, kind, size, st = describe(t)
                    globals_[vname] = {"name": vname, "type": text, "size": size, "where": file_of(cu, die, fcache),
                                       "why": "carrier type" if held in CARRIER_STRUCTS else "name"}
            elif tag == "DW_TAG_subprogram" and name_of(die):
                fname = name_of(die)
                for c in die.iter_children():
                    if c.tag != "DW_TAG_formal_parameter" or not name_of(c) or "DW_AT_type" not in c.attributes:
                        continue
                    pname = name_of(c)
                    t = c.get_DIE_from_attribute("DW_AT_type")
                    held = array_element_struct(t)
                    if MAP_NAME.match(pname):
                        text, kind, size, st = describe(t)
                        params[(fname, pname)] = {"function": fname, "parameter": pname, "type": text, "size": size,
                                                  "where": file_of(cu, die, fcache)}
            elif tag == "DW_TAG_variable" and name_of(die) and MAP_NAME.match(name_of(die)) and die.get_parent() is not top:
                t = die.get_DIE_from_attribute("DW_AT_type") if "DW_AT_type" in die.attributes else None
                text, kind, size, st = describe(t)
                locals_[(text, size)] += 1

# Which structs the native save reaches, and through which path
reach = {}
queue = [(r, r) for r in DURABLE_ROOTS]
while queue:
    s, path = queue.pop()
    if s in reach:
        continue
    reach[s] = path
    for m, held in struct_members.get(s, {}).items():
        if held not in reach:
            queue.append((held, f"{path}.{m}"))
for m in members.values():
    m["saved"] = reach.get(m["struct"])

# Source pass
def read(path):
    try:
        return open(os.path.join(tree, path), encoding="utf-8", errors="replace").read()
    except OSError:
        return ""


def walk(dirs, exts):
    for d in dirs:
        for root, _, files in os.walk(os.path.join(tree, d)):
            for fn in files:
                if fn.endswith(exts):
                    p = os.path.join(root, fn)
                    yield os.path.relpath(p, tree), open(p, encoding="utf-8", errors="replace").read()


c_files = list(walk(["src", "include"], (".c", ".h")))
macro_uses = collections.Counter()
lookups = collections.defaultdict(list)
LOOKUP = re.compile(r"\b(Overworld_GetMapHeaderByGroupAndId|gMapGroups|SetWarpDestination\w*|LoadMapFromCameraTransition|SetDynamicWarp\w*|SetEscapeWarp|SetLastHealLocationWarp|SetContinueGameWarp\w*)\s*[\(\[]")
for path, text in c_files:
    if path.startswith("src/data/maps") or "/generated/" in path:
        continue
    for i, line in enumerate(text.splitlines(), 1):
        n = len(re.findall(r"\bMAP_(GROUP|NUM)\s*\(", line))
        if n:
            macro_uses[path] += n
        for m in LOOKUP.finditer(line):
            if not line.lstrip().startswith(("//", "*", "#define")):
                lookups[m.group(1)].append(f"{path}:{i}")

# Script macros whose bytecode carries a map: those that call `map` or a macro that does (formatwarp...)
inc = "\n".join(read(os.path.join("asm/macros", f)) for f in sorted(os.listdir(os.path.join(tree, "asm/macros"))) if f.endswith(".inc"))
bodies = {}
for m in re.finditer(r"^\s*\.macro\s+(\w+)[^\n]*\n(.*?)^\s*\.endm", inc, re.M | re.S):
    bodies[m.group(1)] = m.group(2)
carrying = {"map"}
changed = True
while changed:
    changed = False
    for name, body in bodies.items():
        if name in carrying:
            continue
        calls = set(re.findall(r"^\s*(\w+)\b", body, re.M))
        if calls & carrying:
            carrying.add(name)
            changed = True
carrying.discard("map")
script_uses = collections.Counter()
for path, text in walk(["data"], (".inc", ".s", ".pory")):
    for line in text.splitlines():
        w = line.split("@")[0].strip().split(None, 1)
        if w and w[0] in carrying:
            script_uses[w[0]] += 1

# Map identity fields tools/mapjson generates
mapjson = read("tools/mapjson/mapjson.cpp")
generated = sorted(set(re.findall(r'\["(dest_map|map|connections|heal_locations|warp_events|object_events|map_group\w*|layout\w*)"\]', mapjson)))

# Capacity constants of map content
consts = {}
for name in ["MAP_GROUPS_COUNT", "MAX_MAP_DATA_SIZE", "NUM_TILES_IN_PRIMARY", "NUM_TILES_IN_PRIMARY_FRLG", "NUM_METATILES_IN_PRIMARY",
             "NUM_METATILES_IN_PRIMARY_FRLG", "NUM_PALS_IN_PRIMARY", "NUM_PALS_IN_PRIMARY_FRLG", "NUM_TILES_TOTAL", "NUM_METATILES_TOTAL",
             "NUM_PALS_TOTAL", "OBJECT_EVENT_TEMPLATES_COUNT", "OBJECT_EVENTS_COUNT", "MAPSEC_COUNT", "KANTO_MAPSEC_START",
             "MAP_OFFSET", "MAX_MAP_WIDTH", "MAX_MAP_HEIGHT"]:
    for path, text in c_files:
        m = re.search(rf"#define\s+{name}\s+(.+)", text) or re.search(rf"\b{name}\s*=\s*([^,\n]+)", text)
        if m:
            consts[name] = {"value": m.group(1).strip().split("//")[0].strip(), "where": path}
            break
layouts = read("include/constants/layouts.h")
layout_ids = [int(v) for v in re.findall(r"#define\s+LAYOUT_\w+\s+(\d+)", layouts)]
groups = read("include/constants/map_groups.h")
map_count = len(re.findall(r"^\s*MAP_\w+\s*=\s*\(", groups, re.M))
group_sizes = collections.Counter(int(g) for g in re.findall(r"^\s*MAP_\w+\s*=\s*\(\s*\d+\s*\|\s*\((\d+)\s*<<\s*8\)\)", groups, re.M))

# Where each struct and function is defined in the tree (DWARF cannot say: every unit is "<stdin>")
struct_at, function_at = {}, {}
FUNC = re.compile(r"^[A-Za-z_][\w \*]*?\b(\w+)\s*\([^;]*$")
for path, text in c_files:
    lines = text.splitlines()
    for i, line in enumerate(lines, 1):
        if line.startswith("struct ") and (line.rstrip().endswith("{") or (i < len(lines) and lines[i].strip() == "{")):
            name = line[7:].split()[0].rstrip("{")
            struct_at.setdefault(name, f"{path}:{i}")
        elif path.endswith(".c") and not line.startswith((" ", "\t", "#", "/", "}")) and "(" in line and not line.rstrip().endswith(";"):
            m = FUNC.match(line)
            if m and m.group(1) not in ("if", "for", "while", "switch", "return"):
                function_at.setdefault(m.group(1), f"{path}:{i}")
for m in members.values():
    m["where"] = struct_at.get(m["struct"], m["where"])
for prm in params.values():
    base = prm["function"].split(".")[0]
    prm["where"] = function_at.get(base, prm["where"])

result = {
    "members": sorted(members.values(), key=lambda m: (m["struct"], m["member"])),
    "globals": sorted(globals_.values(), key=lambda g: g["name"]),
    "parameters": sorted(params.values(), key=lambda p: (p["function"], p["parameter"])),
    "locals": [{"type": t, "size": s, "count": n} for (t, s), n in locals_.most_common()],
    "macroUses": dict(macro_uses.most_common()),
    "lookups": {k: v for k, v in sorted(lookups.items())},
    "scriptMacros": {k: script_uses.get(k, 0) for k in sorted(carrying)},
    "mapjsonFields": generated,
    "capacity": consts,
    "maps": map_count,
    "groups": len(group_sizes),
    "largestGroup": max(group_sizes.values()) if group_sizes else 0,
    "layouts": max(layout_ids) if layout_ids else 0,
}
json.dump(result, open(out_json, "w"), indent=1)

# Report
L = []
w = L.append
w("# Map identity audit (F3.3a)\n")
w(f"Source tree `{tree}`, DINFO ELF `{os.path.basename(elf_path)}`. Generated by `tools/admg101/audit/map_identity.py`; nothing in the tree was changed.\n")
w("## Summary\n")
saved = [m for m in result["members"] if m["saved"]]
by_size = collections.Counter(p["size"] for p in result["parameters"])
w(f"- **Maps:** {map_count} in {len(group_sizes)} groups (largest group {result['largestGroup']} maps); layouts: {result['layouts']} (IDs are 1-based `u16`).")
w(f"- **Struct and union members** keeping a map identity: {len(result['members'])} ({len(saved)} reached by the native save).")
w(f"- **Globals:** {len(result['globals'])}. **Function parameters:** {len(result['parameters'])} "
  f"({', '.join(f'{n} of {s} byte(s)' for s, n in sorted(by_size.items()))}). **Locals:** {sum(l['count'] for l in result['locals'])}.")
w(f"- **`MAP_GROUP()`/`MAP_NUM()` uses:** {sum(macro_uses.values())} in {len(macro_uses)} files.")
w(f"- **Script macros carrying a map in bytecode:** {len(carrying)}, used {sum(script_uses.values())} times in the build's scripts.\n")
w("## Members\n")
w("| Struct | Member | Type | Size | Bits | Saved via | Source |")
w("| --- | --- | --- | --- | --- | --- | --- |")
for m in result["members"]:
    w(f"| `{m['struct']}` | `{m['member']}` | `{m['type']}` | {m['size']} | {m['bits'] or ''} | {('`' + m['saved'] + '`') if m['saved'] else ''} | {m['where'] or ''} |")
w("\n## Globals\n")
w("| Global | Type | Size | Source |")
w("| --- | --- | --- | --- |")
for g in result["globals"]:
    w(f"| `{g['name']}` | `{g['type']}` | {g['size']} | {g['where'] or ''} |")
w("\n## Function parameters\n")
w("| Function | Parameter | Type | Size | Source |")
w("| --- | --- | --- | --- | --- |")
for p in result["parameters"]:
    w(f"| `{p['function']}` | `{p['parameter']}` | `{p['type']}` | {p['size']} | {p['where'] or ''} |")
w("\n## Locals (by type)\n")
w("| Type | Size | Count |")
w("| --- | --- | --- |")
for l in result["locals"]:
    w(f"| `{l['type']}` | {l['size']} | {l['count']} |")
w("\n## Map lookups and warp setters (call sites)\n")
for k, v in result["lookups"].items():
    w(f"- `{k}`: {len(v)} — " + ", ".join(v[:40]) + (" …" if len(v) > 40 else ""))
w("\n## `MAP_GROUP()` / `MAP_NUM()` uses by file\n")
for k, v in result["macroUses"].items():
    w(f"- `{k}`: {v}")
w("\n## Script macros whose bytecode carries a map\n")
w("| Macro | Uses in the build's scripts |")
w("| --- | --- |")
for k, v in result["scriptMacros"].items():
    w(f"| `{k}` | {v} |")
w("\n## Map identity fields `tools/mapjson` reads\n")
w(", ".join(f"`{g}`" for g in generated))
w("\n\n## Capacity constants\n")
w("| Constant | Value | Source |")
w("| --- | --- | --- |")
for k, v in result["capacity"].items():
    w(f"| `{k}` | `{v['value']}` | {v['where']} |")
open(out_md, "w").write("\n".join(L) + "\n")
print(f"members {len(result['members'])} (saved {len(saved)}), globals {len(result['globals'])}, parameters {len(result['parameters'])}, "
      f"locals {sum(l['count'] for l in result['locals'])}, macro uses {sum(macro_uses.values())}, script macros {len(carrying)} / uses {sum(script_uses.values())}")
