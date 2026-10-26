"""Qualify typed state operands after preproc -s and before C preprocessing.

Source-owner rules use the emitted map catalogue and reviewed mixed labels.
The tool never rewrites weather, trainer IDs, text, comparison constants or C.
Unknown named state operands in owned code fail, rather than silently staying zero.
"""
import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

# Parameter positions in the pinned event macro ABI. 'state' permits a named
# flag ID passed as a literal to a special, while numeric immediates stay intact.
OPERANDS = {
    "object_event": {11: "flag"}, "coord_event": {3: "state"}, "bg_hidden_item_event": {4: "flag"},
    "setflag": {0: "flag"}, "clearflag": {0: "flag"}, "checkflag": {0: "flag"},
    "goto_if_set": {0: "flag"}, "goto_if_unset": {0: "flag"},
    "call_if_set": {0: "flag"}, "call_if_unset": {0: "flag"},
    "vgoto_if_set": {0: "flag"}, "vgoto_if_unset": {0: "flag"},
    "setworldmapflag": {0: "flag"}, "cant_see_if_set": {0: "flag"}, "cant_see_if_unset": {0: "flag"},
    "setvar": {0: "var", 1: "state"}, "addvar": {0: "var"}, "subvar": {0: "var"},
    "copyvar": {0: "var", 1: "var"}, "setorcopyvar": {0: "var", 1: "state"},
    "compare": {0: "var", 1: "state"}, "compare_var_to_value": {0: "var"},
    "compare_var_to_var": {0: "var", 1: "var"}, "switch": {0: "var"},
    "specialvar": {0: "var"}, "map_script_2": {0: "var"},
    "getplayerxy": {0: "var", 1: "var"}, "checkcoins": {0: "var"},
    "bufferspeciesname": {1: "var"}, "bufferitemname": {1: "var"}, "buffernumberstring": {1: "var"},
    "givemon": {0: "var"}, "removeobject": {0: "var"}, "applymovement": {0: "var"},
    "checkitemspace": {0: "var", 1: "var"}, "removecoins": {0: "var"},
    "giveitem": {0: "var", 1: "var"}, "additem": {0: "var", 1: "var"},
    "removeitem": {0: "var", 1: "var"}, "putitemaway": {0: "var", 1: "var"},
    "playmoncry": {0: "var"}, "playslotmachine": {0: "var"},
}
for prefix in ("goto_if_", "call_if_", "vgoto_if_", "vcall_if_"):
    for comparison in ("eq", "ne", "gt", "ge", "lt", "le"):
        OPERANDS[prefix + comparison] = {0: "var", 1: "state"}
SYMBOL = re.compile(r"\b(?:FLAG_|VAR_)\w+")
LINE = re.compile(r'^\s*#\s*(?:line\s+)?(\d+)\s+"([^"]+)"')


def split_operands(text):
    parts, start, depth = [], 0, 0
    for i, char in enumerate(text):
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
        elif char == "," and depth == 0:
            parts.append(text[start:i])
            start = i + 1
    parts.append(text[start:])
    return parts


def rewrite(text, catalog, rules, initial_source):
    source, number = initial_source, 1
    labels, records, output = {}, [], []
    for line in text.splitlines(keepends=True):
        directive = LINE.match(line)
        if directive:
            number, source = int(directive[1]), directive[2].replace("\\", "/")
            if source.startswith("./"):
                source = source[2:]
            output.append(line)
            continue
        owner = "hoenn"
        if source.startswith("data/maps/") and len(source.split("/")) >= 4:
            name = source.split("/")[2]
            if name not in catalog["map_owners"]:
                raise ValueError("unknown map source boundary: " + source)
            owner = catalog["map_owners"][name]["owner"]
        code, separator, comment = line.partition("@")
        label = re.match(r"\s*(\w+)::?\s*$", code.rstrip())
        if label:
            labels[source] = label[1]
        for rule in rules["mixed_script_rules"]:
            if source != rule["file"]:
                continue
            if label and label[1] == rule["from_label"]:
                labels[source + ":owner"] = rule["owner"]
            if label and label[1] == rule.get("until_label"):
                labels[source + ":owner"] = "hoenn"
            owner = labels.get(source + ":owner", "hoenn")
        if owner in {"kanto", "engine"} and not code.lstrip().startswith(("#", ".", '"')) and not label:
            match = re.match(r"(\s*)(\w+)(\s+)(.*?)(\r?\n)?$", code)
            symbols = SYMBOL.findall(code)
            if symbols:
                if not match or match[2] not in OPERANDS:
                    raise ValueError(f"unclassified typed state operation at {source}:{number}")
                parts = split_operands(match[4])
                recognized = []
                typed = dict(OPERANDS[match[2]])
                # This field is a reveal flag only for the documented boulder
                # template ABI, never an ordinary trainer namespace operand.
                if match[2] == "object_event" and len(parts) > 8 and parts[1].strip() == "OBJ_EVENT_GFX_PUSHABLE_BOULDER":
                    typed[8] = "flag"
                for index, kind in typed.items():
                    if index >= len(parts):
                        continue
                    def qualify(found):
                        name = found[0]
                        actual = "flag" if name.startswith("FLAG_") else "var"
                        if kind != "state" and kind != actual:
                            raise ValueError(f"wrong state operand kind at {source}:{number}")
                        table = catalog["flags" if actual == "flag" else "vars"]
                        if name not in table:
                            raise ValueError(f"unresolved native state symbol {name} at {source}:{number}")
                        row = table[name]
                        recognized.append(name)
                        records.append({"file": source, "line": number, "command": match[2], "operand": index,
                                        "owner": owner, "name": name, "generated": row["generated"], "id": row["id"]})
                        return row["generated"]
                    parts[index] = SYMBOL.sub(qualify, parts[index])
                if sorted(recognized) != sorted(symbols):
                    raise ValueError(f"state symbol outside classified operands at {source}:{number}")
                code = match[1] + match[2] + match[3] + ",".join(parts) + (match[5] or "")
        output.append(code + separator + comment)
        number += 1
    return "".join(output), records


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--catalog", default="tools/admg101/state-operands.json")
    parser.add_argument("--ownership", default="tools/admg101/source-ownership.json")
    parser.add_argument("--source", required=True)
    parser.add_argument("--input")
    parser.add_argument("--report")
    args = parser.parse_args()
    catalog = json.loads(Path(args.catalog).read_text(encoding="utf-8"))
    rules = json.loads(Path(args.ownership).read_text(encoding="utf-8"))
    text = Path(args.input).read_text(encoding="utf-8") if args.input else sys.stdin.read()
    result, records = rewrite(text, catalog, rules, args.source)
    if args.report:
        path = Path(args.report)
        if path.exists():
            raise ValueError("refusing to overwrite operand reconciliation")
        path.write_text(json.dumps({"operands": records, "source_sha256": hashlib.sha256(text.encode()).hexdigest(),
                                   "output_sha256": hashlib.sha256(result.encode()).hexdigest(),
                                   "lock_sha256": catalog["lock_sha256"]}, sort_keys=True, indent=2) + "\n", encoding="utf-8", newline="\n")
    sys.stdout.write(result)


if __name__ == "__main__":
    main()
