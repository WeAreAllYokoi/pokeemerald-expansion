#!/usr/bin/env python3
"""List every struct/union member and global whose type stores a content ID, from DWARF."""
import json, re, sys, collections
from elftools.elf.elffile import ELFFile

# dwarf_audit.py <DINFO ELF, .rel.debug* stripped> <report.json> [<findings.txt>]
elf_path, out_path = sys.argv[1], sys.argv[2]
CONTENT_ENUMS = {"Species", "Move", "Item", "Ability", "Type", "TrainerPicID", "MoveEffect", "TrainerClassID", "TrainerId", "NationalDexOrder",
                 "Nature", "MapSec", "EggGroup", "GrowthRate", "Stat", "BattleMoveEffects", "Weather", "BattleEnvironment"}
NAME_RE = re.compile(r"(species|move(?!ment)|item|abilit|type(?!s?Info)|mapsec|metLocation|trainer(Id|Num|Class|Pic)?|nature|ball|berry|decor|easyChat|ecWord|words?$|mapGroup|mapNum|location|heldItem|egg)", re.I)

def attr(die, name, default=None):
    a = die.attributes.get(name)
    return a.value if a is not None else default

def name_of(die):
    n = attr(die, "DW_AT_name")
    return n.decode() if isinstance(n, bytes) else n

def resolve(die, depth=0):
    """Returns (description, base kind, byte size, enum name)."""
    if die is None:
        return ("void", "void", 0, None)
    tag = die.tag
    if tag in ("DW_TAG_typedef", "DW_TAG_const_type", "DW_TAG_volatile_type"):
        inner = die.get_DIE_from_attribute("DW_AT_type") if "DW_AT_type" in die.attributes else None
        d, k, s, e = resolve(inner, depth + 1)
        if tag == "DW_TAG_typedef":
            d = f"{name_of(die)}"
        return (d, k, s, e)
    if tag == "DW_TAG_enumeration_type":
        return (f"enum {name_of(die)}", "enum", attr(die, "DW_AT_byte_size", 0), name_of(die))
    if tag == "DW_TAG_base_type":
        return (name_of(die), "int", attr(die, "DW_AT_byte_size", 0), None)
    if tag == "DW_TAG_array_type":
        inner = die.get_DIE_from_attribute("DW_AT_type")
        d, k, s, e = resolve(inner, depth + 1)
        count = []
        for c in die.iter_children():
            if c.tag == "DW_TAG_subrange_type":
                ub = attr(c, "DW_AT_upper_bound")
                count.append(ub + 1 if isinstance(ub, int) else None)
        return (f"{d}{''.join(f'[{n}]' for n in count)}", "array:" + k, s, e)
    if tag == "DW_TAG_pointer_type":
        if "DW_AT_type" in die.attributes and depth < 4:
            d, k, s, e = resolve(die.get_DIE_from_attribute("DW_AT_type"), depth + 1)
            return (f"{d} *", "ptr:" + k, s, e)
        return ("ptr", "ptr", 4, None)
    if tag in ("DW_TAG_structure_type", "DW_TAG_union_type"):
        return (f"struct {name_of(die)}", "struct", attr(die, "DW_AT_byte_size", 0), None)
    return (tag, tag, 0, None)

with open(elf_path, "rb") as f:
    elf = ELFFile(f)
    dwarf = elf.get_dwarf_info()
    members = {}
    globals_ = {}
    for cu in dwarf.iter_CUs():
        top = cu.get_top_DIE()
        cu_name = name_of(top)
        files = None
        for die in cu.iter_DIEs():
            if die.tag in ("DW_TAG_structure_type", "DW_TAG_union_type") and name_of(die) and not die.attributes.get("DW_AT_declaration"):
                sname = name_of(die)
                for m in die.iter_children():
                    if m.tag != "DW_TAG_member":
                        continue
                    mname = name_of(m) or "<anon>"
                    t = m.get_DIE_from_attribute("DW_AT_type") if "DW_AT_type" in m.attributes else None
                    desc, kind, size, enum = resolve(t)
                    bits = attr(m, "DW_AT_bit_size")
                    key = (sname, mname)
                    if key in members:
                        continue
                    members[key] = dict(struct=sname, member=mname, type=desc, kind=kind, size=size, bits=bits, enum=enum, cu=cu_name)
            elif die.tag == "DW_TAG_variable" and die.get_parent() is top and name_of(die) and "DW_AT_type" in die.attributes:
                vname = name_of(die)
                t = die.get_DIE_from_attribute("DW_AT_type")
                desc, kind, size, enum = resolve(t)
                if vname not in globals_:
                    globals_[vname] = dict(name=vname, type=desc, kind=kind, size=size, enum=enum, cu=cu_name)

def interesting(e, label):
    if e["enum"] in CONTENT_ENUMS:
        return "content-enum"
    if e["kind"] in ("int", "array:int", "ptr:int", "ptr:array:int") and e["size"] in (1, 2) and NAME_RE.search(label):
        return "narrow-named"
    return None

report = {"members": [], "globals": []}
for (s, m), e in sorted(members.items()):
    why = interesting(e, m)
    if why:
        e["why"] = why
        report["members"].append(e)
for n, e in sorted(globals_.items()):
    why = interesting(e, n)
    if why:
        e["why"] = why
        report["globals"].append(e)
json.dump(report, open(out_path, "w"), indent=1)
c = collections.Counter((e["why"], e["enum"]) for e in report["members"])
print("members", len(report["members"]), "globals", len(report["globals"]))
for k, v in sorted(c.items(), key=lambda kv: -kv[1]):
    print(k, v)

# Findings for the report, one line each ("<location>: <detail>"):
#  - every member or global holding a content enum narrower than 32 bits or in a bit field;
#  - every struct member named like a content ID (GATE_NAME) stored in an 8/16-bit integer, array of or pointer to one,
#    except one-bit flags, booleans, and text (pointers and byte arrays longer than 8). Globals named like content are not
#    gated: they are ROM tables and script text (dwarf.json still lists every one).
GATE_NAME = re.compile(r"(species|move(?!ment)|item|abilit|nature|trainerIds?$|trainerNum|dexNum|nationalNum|mapsec|metLocation)", re.I)


def text_like(e):
    m = re.search(r"\[(\d+|None)\]$", e["type"])
    return e["type"].endswith("*") or (e["size"] == 1 and m is not None and (m.group(1) == "None" or int(m.group(1)) > 8))


findings_path = sys.argv[3] if len(sys.argv) > 3 else None
if findings_path:
    rows = []
    for e in report["members"]:
        if e["why"] == "content-enum":
            if (e["size"] or 4) >= 4 and not e["bits"]:
                continue
        elif not GATE_NAME.search(e["member"]) or e["bits"] == 1 or e["type"].startswith("bool") or text_like(e):
            continue
        bits = f" bits={e['bits']}" if e["bits"] else ""
        rows.append(f"struct {e['struct']}.{e['member']}: {e['why']} {e['type']} size={e['size']}{bits}")
    for e in report["globals"]:
        if e["why"] == "content-enum" and (e["size"] or 4) < 4:
            rows.append(f"global {e['name']}: {e['why']} {e['type']} size={e['size']}")
    open(findings_path, "w").write("\n".join(sorted(set(rows))) + ("\n" if rows else ""))
    print("findings", len(set(rows)))
