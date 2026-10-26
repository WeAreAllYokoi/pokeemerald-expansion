"""Generate passive retention roots from existing pinned regional identities.

Run from the reconstructed source root. Outputs are derived build inputs, never
authoritative copies of map IDs, layouts, game state, or regional dispatch.
"""
import json
import re
from pathlib import Path


def emit(path, text):
    target = Path(path)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text, encoding="utf-8", newline="\n")


def generate():
    catalog = json.loads(Path("tools/admg101/required-resources.json").read_text(encoding="utf-8"))
    groups = json.loads(Path("data/maps/map_groups.json").read_text(encoding="utf-8"))
    layouts = json.loads(Path("data/layouts/layouts.json").read_text(encoding="utf-8"))
    maps = {data["name"]: data for path in sorted(Path("data/maps").glob("*/map.json"))
            for data in [json.loads(path.read_text(encoding="utf-8"))]}
    registered = [name for group in groups["group_order"] for name in groups[group]]
    auxiliary = sorted(set(maps) - set(registered))
    checks = {
        "registered_maps": registered,
        "map_groups": groups["group_order"],
        "layouts": [layout["name"] for layout in layouts["layouts"]],
        "auxiliary_maps": auxiliary,
        "layout_table": layouts["layouts_table_label"],
        "tilesets": re.findall(r"const struct Tileset (\w+)\s*=", Path("src/data/tilesets/headers.h").read_text(encoding="utf-8")),
    }
    for key, actual in checks.items():
        if actual != catalog[key]:
            raise ValueError(f"Pinned resource catalog mismatch: {key}")
    for layout in layouts["layouts"]:
        for key in ("border_filepath", "blockdata_filepath"):
            if not Path(layout[key]).is_file():
                raise ValueError(f"Required layout input missing: {layout[key]}")
    notice = "// Derived ADMG-101 passive roots; do not edit.\n"
    emit("src/data/admg_tileset_roots.h", notice + "".join(
        f"ADMG_RETAIN_DATA({name});\n" for name in catalog["tilesets"]))
    emit("src/data/admg_object_roots.h", notice + "".join(
        f"ADMG_RETAIN_DATA({name});\n" for name in catalog["object_roots"]))
    roots = ["gMapGroups", layouts["layouts_table_label"]] + auxiliary
    emit("data/maps/admg_roots.inc",
         '@ Derived passive guest-pointer roots; no new map registrations.\n'
         '\t.pushsection .admg.exports, "a", %progbits\n\t.balign 4\n' +
         "".join(f"\t.4byte {name}\n" for name in roots) + '\t.popsection\n')
    for output, filename in (("headers", "header.inc"), ("connections", "connections.inc"),
                             ("events", "events.inc")):
        emit(f"data/maps/admg_aux_{output}.inc", '@ Derived auxiliary resources.\n' + "".join(
            f'\t.include "data/maps/{name}/{filename}"\n' for name in auxiliary))
    # Existing aggregation may already provide an auxiliary script. Retain its
    # provider once rather than emitting a second definition of the same label.
    existing_scripts = set(re.findall(r'\.include\s+"([^"]+)"', Path("data/event_scripts.s").read_text(encoding="utf-8")))
    # mapjson always emits the header's <map>_MapScripts pointer (never NULL).
    # An auxiliary map without an upstream scripts.inc receives upstream's empty
    # map-script table, so every retained header resolves at the full link.
    # The previous output of this generator is excluded so regeneration is stable.
    defined = set()
    for include in existing_scripts - {"data/maps/admg_aux_scripts.inc"}:
        if Path(include).is_file():
            defined.update(re.findall(r"^(\w+_MapScripts)::", Path(include).read_text(encoding="utf-8"), re.M))
    providers = []
    for name in auxiliary:
        path = f"data/maps/{name}/scripts.inc"
        if Path(path).is_file():
            if path not in existing_scripts:
                providers.append(f'\t.include "{path}"\n')
        elif "shared_scripts_map" not in maps[name]:
            if f"{name}_MapScripts" in defined:
                raise ValueError(f"Auxiliary map scripts already defined: {name}")
            providers.append(f"{name}_MapScripts::\n\t.byte 0\n")
    emit("data/maps/admg_aux_scripts.inc", '@ Derived auxiliary script providers.\n' + "".join(providers))
    return {key: len(value) if isinstance(value, list) else value for key, value in checks.items()}


if __name__ == "__main__":
    print(json.dumps(generate(), sort_keys=True))
