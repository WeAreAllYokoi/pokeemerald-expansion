"""Deterministic S3-B proposals and generation from the separately approved lock.

The locked mode validates the entire approved allocation against pinned inputs;
it cannot accept a changed proposal or allocate replacement identifiers.
"""
import argparse
import hashlib
import json
import operator
import re
from pathlib import Path

PIN = "c5f026363afda6dd56225bebb2f2d2cdcd9d84f0"
OPS = {"+": operator.add, "-": operator.sub, "*": operator.mul,
       "/": lambda a, b: (abs(a) // abs(b)) * (-1 if (a < 0) != (b < 0) else 1),
       "%": operator.mod, "<<": operator.lshift, ">>": operator.rshift,
       "|": operator.or_, "&": operator.and_, "^": operator.xor}
PRIORITY = {"|": 1, "^": 2, "&": 3, "<<": 4, ">>": 4, "+": 5, "-": 5,
            "*": 6, "/": 6, "%": 6}


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def canonical(value):
    return (json.dumps(value, sort_keys=True, indent=2) + "\n").encode("utf-8")


def read_macros(path):
    result = {}
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        match = re.fullmatch(r"#define\s+(\w+)\s+(.+)", line)
        if match:
            result[match[1]] = match[2]
    return result


def evaluate(expression, macros, stack=()):
    """Evaluate a bounded C integer subset without feeding arbitrary C to Python."""
    token_pattern = re.compile(r"\s*(0[xX][0-9a-fA-F]+[uUlL]*|[0-9]+[uUlL]*|[A-Za-z_]\w*|<<|>>|[()+*/%&|^~\-])")
    tokens, cursor = [], 0
    expression = expression.strip()
    while cursor < len(expression):
        match = token_pattern.match(expression, cursor)
        if not match:
            raise ValueError("unsupported integer expression")
        tokens.append(match[1])
        cursor = match.end()
    position = 0
    def parse(minimum=0):
        nonlocal position
        if position >= len(tokens):
            raise ValueError("missing integer operand")
        token = tokens[position]
        position += 1
        if token == "(":
            left = parse()
            if position >= len(tokens) or tokens[position] != ")":
                raise ValueError("unbalanced integer expression")
            position += 1
        elif token in {"+", "-", "~"}:
            operand = parse(7)
            left = operand if token == "+" else (-operand if token == "-" else ~operand)
        elif token[0].isdigit():
            numeric = token.rstrip("uUlL")
            left = int(numeric, 16 if numeric.lower().startswith("0x") else 8 if len(numeric) > 1 and numeric.startswith("0") else 10)
        elif re.fullmatch(r"[A-Za-z_]\w*", token):
            if token in stack or token not in macros:
                raise ValueError("unresolved or recursive integer symbol: " + token)
            left = evaluate(macros[token], macros, stack + (token,))
        else:
            raise ValueError("invalid integer operand")
        while position < len(tokens) and tokens[position] in PRIORITY and PRIORITY[tokens[position]] >= minimum:
            symbol = tokens[position]
            position += 1
            right = parse(PRIORITY[symbol] + 1)
            if symbol in {"<<", ">>"} and not 0 <= right < 64:
                raise ValueError("out-of-range integer shift")
            left = OPS[symbol](left, right)
        return left
    value = parse()
    if position != len(tokens):
        raise ValueError("unsupported integer suffix")
    return value


def state_constants(macros):
    resolved, unresolved = {}, {}
    for name in sorted(macros):
        if name.startswith(("FLAG_", "VAR_", "TESTING_FLAG_", "TESTING_VAR_")):
            try:
                resolved[name] = evaluate(name, macros)
            except (ValueError, SyntaxError, ZeroDivisionError) as exc:
                unresolved[name] = str(exc)
    return resolved, unresolved


def reserve(kind, name, count, alignment, lower, upper, occupied):
    base = ((lower + alignment - 1) // alignment) * alignment
    while base + count - 1 <= upper:
        conflicts = [r for r in occupied if r["kind"] == kind
                     and not (base + count - 1 < r["first"] or base > r["last"])]
        if not conflicts:
            record = {"kind": kind, "name": name, "first": base,
                      "last": base + count - 1, "count": count, "alignment": alignment}
            occupied.append(record)
            return record
        base = ((max(r["last"] for r in conflicts) + 1 + alignment - 1) // alignment) * alignment
    raise ValueError("no complete legal reservation for " + name)


def generate(registry, macros):
    if registry["source_identity"]["commit"] != PIN:
        raise ValueError("wrong Expansion revision")
    values, unresolved = state_constants(macros)
    def val(name):
        return evaluate(name, macros)
    occupied = [
        {"kind": "flag", "name": "hoenn", "first": 0, "last": val("FLAGS_COUNT") - 1},
        {"kind": "flag", "name": "special", "first": val("SPECIAL_FLAGS_START"), "last": val("SPECIAL_FLAGS_END")},
        {"kind": "flag", "name": "testing", "first": val("TESTING_FLAGS_START"), "last": val("TESTING_FLAGS_START") + 7},
        {"kind": "var", "name": "hoenn", "first": val("VARS_START"), "last": val("VARS_END")},
        {"kind": "var", "name": "special", "first": val("SPECIAL_VARS_START"), "last": val("SPECIAL_VARS_END")},
        {"kind": "var", "name": "testing", "first": val("TESTING_VARS_START"), "last": val("TESTING_VARS_START") + 7},
    ]
    # Keep complete native ordinal order. Alignment retains id & 7.
    flag = reserve("flag", "kanto_native_flags", 2304, 8, val("FLAGS_COUNT"),
                   val("SPECIAL_FLAGS_START") - 1, occupied)
    var = reserve("var", "kanto_native_vars", 224, 1, val("VARS_END") + 1,
                  val("SPECIAL_VARS_START") - 1, occupied)
    text = reserve("var", "text_color_scratch", 2, 1, val("SPECIAL_VARS_END") + 1,
                   val("TESTING_VARS_START") - 1, occupied)
    # Native unused special positions must not alias Hoenn's active bits 2/4.
    scratch = reserve("flag", "reserved_native_special_scratch", 8, 8,
                      val("SPECIAL_FLAGS_END") + 1, val("TESTING_FLAGS_START") - 1, occupied)
    bindings = sorted(registry["unbound_feature_bindings"], key=lambda r: r["name"])
    if any(r["kind"] not in {"flag", "variable"} for r in bindings):
        raise ValueError("unknown binding kind")
    flag_bindings = [r for r in bindings if r["kind"] == "flag"]
    var_bindings = [r for r in bindings if r["kind"] == "variable"]
    if len(flag_bindings) != 38 or len(var_bindings) != 8:
        raise ValueError("binding kinds do not reconcile to accepted 38 flags / 8 vars")
    control_flags = reserve("flag", "feature_binding_flags", 40, 8, val("FLAGS_COUNT"),
                            val("SPECIAL_FLAGS_START") - 1, occupied)
    control_vars = reserve("var", "feature_binding_vars", 8, 1, val("VARS_END") + 1,
                           val("SPECIAL_VARS_START") - 1, occupied)
    entries, review_cases = [], []
    native_alias_targets = {}
    for row in sorted(registry["complete_native_flag_symbols"], key=lambda r: r["name"]):
        name, ordinal, policy = row["name"], row["native_ordinal"], row["policy"]
        target = row.get("canonical_target_name")
        if policy == "common_alias":
            if target not in values:
                raise ValueError("missing canonical flag target: " + str(target))
            assigned, route = values[target], "canonical_alias"
            native_alias_targets.setdefault(ordinal, set()).add(target)
        elif policy == "scratch":
            if target in values:
                assigned, route = values[target], "canonical_transient_alias"
            else:
                assigned, route = scratch["first"] + (ordinal & 7), "independent_reserved_transient"
                review_cases.append({"case": "native_unused_special_has_no_hoenn_same_name",
                                     "symbol": name, "native_ordinal": ordinal,
                                     "treatment": "independent reserved scratch; no Hoenn active-bit alias"})
        elif 0 <= ordinal <= 2303:
            assigned = flag["first"] + ordinal
            route = "service_transient" if policy == "union_visibility" else "kanto_bank"
        else:
            raise ValueError("unclassified native flag ordinal: " + name)
        entries.append({"kind": "flag", "name": name, "generated_name": "ADMG_KANTO_" + name,
                        "native_ordinal": ordinal, "policy": policy, "assigned_id": assigned,
                        "route": route, "canonical_target": target,
                        "native_mirror_reserved": route != "kanto_bank"})
    for ordinal, targets in sorted(native_alias_targets.items()):
        if len(targets) > 1:
            review_cases.append({"case": "native_ordinal_has_distinct_approved_canonical_targets",
                                 "native_ordinal": ordinal, "targets": sorted(targets),
                                 "treatment": "resolve named semantics; raw mirror is reserved and has no writable authority"})
    for row in sorted(registry["complete_native_persistent_variable_slots"], key=lambda r: r["native_ordinal"]):
        ordinal = row["native_ordinal"]
        targets = row["canonical_target_names"]
        if len(set(targets)) > 1:
            raise ValueError("ambiguous canonical variable slot")
        assigned = values[targets[0]] if targets else var["first"] + ordinal - 0x4020
        entries.append({"kind": "var", "names": row["native_names"], "native_ordinal": ordinal,
                        "assigned_id": assigned, "route": "canonical_alias" if targets else "kanto_bank",
                        "canonical_target": targets[0] if targets else None,
                        "native_mirror_reserved": bool(targets)})
    for row in sorted(registry["audited_variables"], key=lambda r: r["name"]):
        if row["semantic_class"] != "special_scratch":
            continue
        target = row.get("canonical_target_name")
        assigned = values[target] if target else text["first"] + (row["name"] == "VAR_PREV_TEXT_COLOR")
        entries.append({"kind": "var", "name": row["name"], "assigned_id": assigned,
                        "route": "canonical_transient_alias" if target else "independent_transient",
                        "canonical_target": target, "lifetime": "transient"})
    for kind, rows, bank in [("flag", flag_bindings, control_flags), ("var", var_bindings, control_vars)]:
        for index, row in enumerate(rows):
            entries.append({"kind": kind, "name": row["name"], "assigned_id": bank["first"] + index,
                            "route": "feature_binding", "source_binding_remains": 0,
                            "presence_default": "absent", "activation": "none",
                            "consumer_adaptation_gate": "source control semantics in owning S2/S3 package"})
    marker = next(r for r in registry["audited_flags"] if r["policy"] == "marker")
    entries.append({"kind": "flag", "name": marker["name"],
                    "assigned_id": flag["first"] + marker["native_definition"]["ordinal"],
                    "route": "marker_alias", "extra_storage": 0})
    hidden = registry["namespace_ranges"]["frlg_hidden_items"]
    base = val("FLAG_HIDDEN_ITEMS_START")
    offsets = [flag["first"] + ordinal - base for ordinal in range(hidden["native_first"], hidden["native_last"] + 1)]
    if min(offsets) < 0 or max(offsets) >= 1 << 13:
        raise ValueError("complete hidden-item reservation cannot be represented")
    if any(not 0 <= row["assigned_id"] <= 0xffff for row in entries):
        raise ValueError("u16 identifier overflow")
    for i, a in enumerate(occupied):
        for b in occupied[i + 1:]:
            if a["kind"] == b["kind"] and max(a["first"], b["first"]) <= min(a["last"], b["last"]):
                raise ValueError("reservation collision")
    return {"format": "admg101-s3-b-allocation-proposal", "format_version": 1,
            "status": "PROPOSED_NOT_ACCEPTED", "source_commit": PIN,
            "integrated_target_schema_version": None, "runtime_consumption": "none",
            "reservations": occupied, "entries": entries, "review_cases": review_cases,
            "legacy_constants": values, "unresolved_legacy_expressions": unresolved,
            "hidden_item_constraints": {"base": base, "named_events": 182, "native_positions": len(offsets),
                                        "offset_first": min(offsets), "offset_last": max(offsets)},
            "default_requirements": {"hoenn_ids": "unchanged", "feature_binding_presence": "absent",
                                     "automatic_kanto_initialization": False, "mod_save_capacity_allocation": 0},
            "coverage": registry["counts"], "consumer_safety_verified": False,
            "later_gates": ["allocation review", "source integration", "V3-B2..B5", "S3-C..F", "S2-F normal native diagnostics"]}


def verify_lock(lock, proposal):
    if lock.get("format") != "admg101-s3-b-namespace-lock" or lock.get("status") != "ACCEPTED_IMMUTABLE":
        raise ValueError("an explicitly accepted namespace lock is required")
    original = dict(lock)
    for key in ("approval", "allocation_proposal_sha256", "exact_assignment_review"):
        original.pop(key, None)
    original.update(format="admg101-s3-b-allocation-proposal", status="PROPOSED_NOT_ACCEPTED")
    if canonical(original) != canonical(proposal):
        raise ValueError("locked allocation differs from pinned proposal; never reallocate")
    if hashlib.sha256(canonical(proposal)).hexdigest() != lock["allocation_proposal_sha256"]:
        raise ValueError("immutable allocation digest mismatch")
    values = proposal["legacy_constants"]
    entries = {r["name"]: r for r in proposal["entries"] if "name" in r and r["kind"] == "flag"}
    expected = {"FLAG_SPECIAL_FLAG_UNUSED_0x4002": 0x4082,
                "FLAG_SPECIAL_FLAG_UNUSED_0x4004": 0x4084,
                "FLAG_SYS_MYSTERY_EVENT_ENABLE": 0x08ac,
                "FLAG_SYS_MYSTERY_GIFT_ENABLE": 0x08db}
    if any(entries[name]["assigned_id"] != value for name, value in expected.items()):
        raise ValueError("approved alias treatment changed")
    for name in ("FLAG_SYS_MYSTERY_EVENT_ENABLE", "FLAG_SYS_MYSTERY_GIFT_ENABLE"):
        if entries[name]["canonical_target"] != name or entries[name]["assigned_id"] != values[name]:
            raise ValueError("Mystery service must use its named canonical authority")
    return lock


def generated_identifier(prefix, name):
    # Registry array-element labels describe logical positions, not C macro
    # syntax. Preserve their IDs/extent and emit distinct legal identifiers for
    # the whole category; the base identifier retains its original spelling.
    element = re.fullmatch(r"([A-Za-z_]\w*)\[([0-9]+)\]", name)
    if element:
        name = element[1] + "_WORD_" + element[2]
    result = prefix + name
    if not re.fullmatch(r"[A-Za-z_]\w*", result):
        raise ValueError("unsupported generated state identifier: " + result)
    return result


def emit_locked(lock, registry, output, source):
    """Emit identifiers and routing facts only. No allocation or gameplay dispatch."""
    output.mkdir(parents=True, exist_ok=True)
    files = {"include/constants/admg_state_ids.h", "src/data/admg_state_routes.h",
             "tools/admg101/state-operands.json"}
    banks = {(r["kind"] + "_" + r["name"] if r["name"] in {"hoenn", "special", "testing"} else r["name"]): r
             for r in lock["reservations"]}
    flags, variables = {}, {}
    for entry in lock["entries"]:
        names = entry.get("names", [entry.get("name")])
        for name in names:
            if not name:
                continue
            target = flags if entry["kind"] == "flag" else variables
            prefix = "ADMG_BINDING_" if entry["route"] == "feature_binding" else "ADMG_KANTO_"
            generated = generated_identifier(prefix, name)
            if name in target and target[name]["id"] != entry["assigned_id"]:
                raise ValueError("conflicting named operand: " + name)
            target[name] = {"id": entry["assigned_id"], "generated": generated,
                            "route": entry["route"], "canonical_target": entry.get("canonical_target")}
    # Temporary state is intentionally shared; never install native mirror bits.
    for name, value in lock["legacy_constants"].items():
        if name.startswith("FLAG_TEMP_") and 0 <= value < 32:
            flags.setdefault(name, {"id": value, "generated": name, "route": "common_temporary"})
        if name.startswith("VAR_") and (0x4000 <= value < 0x4020 or 0x8000 <= value <= 0x8015):
            variables.setdefault(name, {"id": value, "generated": name, "route": "common_scratch"})
    # Some retained scripts use the original unsuffixed spelling while the
    # compatibility declaration was renamed *_FRLG. Resolve that entire category
    # by named declaration and source owner, never by colliding Hoenn ordinal.
    source_spellings = []
    for name in sorted(list(variables)):
        if not name.endswith("_FRLG"):
            continue
        spelling = name.removesuffix("_FRLG")
        if spelling not in lock["legacy_constants"]:
            continue
        row = variables[name]
        if spelling in variables and variables[spelling]["id"] != row["id"]:
            raise ValueError("ambiguous native source spelling: " + spelling)
        variables[spelling] = dict(row)
        source_spellings.append({"spelling": spelling, "native_declaration": name,
                                 "id": row["id"], "canonical_target": row.get("canonical_target"),
                                 "source_owner": "kanto", "new_writable_authority": False})
    header = ["/* Generated from the reviewed immutable S3-B lock. */", "#ifndef ADMG_STATE_IDS_H", "#define ADMG_STATE_IDS_H"]
    for name, bank in sorted(banks.items()):
        prefix = "ADMG_STATE_" + name.upper()
        header += [f"#define {prefix}_FIRST 0x{bank['first']:04X}",
                   f"#define {prefix}_LAST 0x{bank['last']:04X}",
                   f"#define {prefix}_COUNT {bank.get('count', bank['last'] - bank['first'] + 1)}"]
    definitions = {}
    for entry in [*flags.values(), *variables.values()]:
        name = entry["generated"]
        if name.startswith("ADMG_"):
            if name in definitions and definitions[name] != entry["id"]:
                raise ValueError("conflicting generated state authority: " + name)
            definitions[name] = entry["id"]
    for name, value in sorted(definitions.items()):
        header.append(f"#define {name} 0x{value:04X}")
    reserved_flags = sorted(set(range(32)) | {r["native_ordinal"] for r in lock["entries"]
        if r["kind"] == "flag" and r.get("native_mirror_reserved") and 0 <= r.get("native_ordinal", -1) < 2304
        and r["route"] != "service_transient"})
    reserved_vars = sorted({r["native_ordinal"] - 0x4020 for r in lock["entries"]
        if r["kind"] == "var" and r.get("native_mirror_reserved")})
    # Compiler operand classification uses the same approved backing domains
    # and mirror exclusions as runtime routing. It does not allocate identifiers.
    valid_variables = set()
    for name in ("var_hoenn", "var_special", "kanto_native_vars",
                 "text_color_scratch", "feature_binding_vars"):
        bank = banks[name]
        valid_variables.update(range(bank["first"], bank["last"] + 1))
    kanto_first = banks["kanto_native_vars"]["first"]
    valid_variables.difference_update(kanto_first + ordinal for ordinal in reserved_vars)
    def interval_expression(values):
        ranges = []
        for value in sorted(values):
            if ranges and value == ranges[-1][1] + 1:
                ranges[-1][1] = value
            else:
                ranges.append([value, value])
        return " || ".join(f"((value) == 0x{first:04X})" if first == last else
            f"((value) >= 0x{first:04X} && (value) <= 0x{last:04X})"
            for first, last in ranges)
    testing = banks["var_testing"]
    test_variables = range(testing["first"], testing["last"] + 1)
    header += ["/* Constant-expression operand kind; never evaluates guest state. */",
        "#if TESTING",
        "#define ADMG_STATE_IS_TEST_VAR_OPERAND(value) (" + interval_expression(test_variables) + ")",
        "#else", "#define ADMG_STATE_IS_TEST_VAR_OPERAND(value) 0", "#endif",
        "#define ADMG_STATE_IS_VAR_OPERAND(value) (" + interval_expression(valid_variables)
            + " || ADMG_STATE_IS_TEST_VAR_OPERAND(value))", "#endif", ""]
    transient = sorted(r["native_ordinal"] for r in lock["entries"] if r["route"] == "service_transient")
    if transient != list(range(99, 107)):
        raise ValueError("reviewed Union Room lifetime/bit positions changed")
    routes = ["/* Generated routing exceptions; raw alias mirrors have no writable authority. */",
              "static const u16 sADMG_ReservedFlagMirrors[] = {" + ", ".join(map(str, reserved_flags)) + "};",
              "static const u16 sADMG_ReservedVarMirrors[] = {" + ", ".join(map(str, reserved_vars)) + "};",
              f"#define ADMG_UNION_NATIVE_FIRST {transient[0]}", f"#define ADMG_UNION_NATIVE_LAST {transient[-1]}",
              "#define ADMG_UNION_STORAGE_BYTES 2", "#define ADMG_KANTO_DAILY_FIRST 912", "#define ADMG_KANTO_DAILY_COUNT 64", ""]
    catalog = {"format": "admg101-state-operands", "source_commit": PIN,
               "lock_sha256": hashlib.sha256(canonical(lock)).hexdigest(),
               "flags": flags, "vars": variables, "legacy_constants": lock["legacy_constants"],
               "source_spelling_aliases": source_spellings,
               "reserved_flag_mirrors": reserved_flags, "reserved_var_mirrors": reserved_vars,
               "map_fields": {"object_events.flag": "flag", "coord_events.var": "var", "bg_events.hidden_item.flag": "flag"},
               "runtime_map_inference": False, "gameplay_activation": "none"}
    catalog["map_owners"] = {}
    for path in sorted(source.glob("data/maps/*/map.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        region = data.get("region", "REGION_HOENN")
        if region not in {"REGION_HOENN", "REGION_KANTO"}:
            raise ValueError("unclassified map source owner: " + str(path))
        catalog["map_owners"][path.parent.name] = {
            "owner": "kanto" if region == "REGION_KANTO" else "hoenn",
            "source_sha256": digest(path), "shared_events_map": data.get("shared_events_map")}
    if len(catalog["map_owners"]) != 939:
        raise ValueError("complete source map ownership inventory required")
    for name, data in [("include/constants/admg_state_ids.h", "\n".join(header).encode()),
                       ("src/data/admg_state_routes.h", "\n".join(routes).encode()),
                       ("tools/admg101/state-operands.json", canonical(catalog))]:
        path = output / name
        path.parent.mkdir(parents=True, exist_ok=True)
        if path.exists():
            if path.read_bytes() != data:
                raise ValueError("derived namespace changed; use a fresh candidate: " + str(path))
        else:
            path.write_bytes(data)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--registry", required=True)
    parser.add_argument("--macros")
    parser.add_argument("--output")
    parser.add_argument("--reverse-inputs", action="store_true")
    parser.add_argument("--lock")
    parser.add_argument("--emit-directory")
    parser.add_argument("--source")
    args = parser.parse_args()
    output = Path(args.output) if args.output else None
    if output and output.exists():
        raise ValueError("refusing to overwrite allocation proposal")
    registry = json.loads(Path(args.registry).read_text(encoding="utf-8"))
    if args.reverse_inputs:
        for key in ("complete_native_flag_symbols", "complete_native_persistent_variable_slots",
                    "audited_variables", "unbound_feature_bindings"):
            registry[key].reverse()
    if args.macros:
        proposal = generate(registry, read_macros(args.macros))
        proposal["semantic_registry_sha256"] = digest(args.registry)
    elif args.lock:
        proposal = json.loads(Path(args.lock).read_text(encoding="utf-8"))
        for key in ("approval", "allocation_proposal_sha256", "exact_assignment_review"):
            proposal.pop(key, None)
        proposal.update(format="admg101-s3-b-allocation-proposal", status="PROPOSED_NOT_ACCEPTED")
        if hashlib.sha256(canonical(proposal)).hexdigest() != "66a251af00184942c9c5e303cd358034d5c9b5b1f1f1c096f136d87a8a24b1b7":
            raise ValueError("reviewed immutable proposal identity changed")
        if proposal["semantic_registry_sha256"] != digest(args.registry) or registry["source_identity"]["commit"] != PIN:
            raise ValueError("accepted semantic input changed")
    else:
        raise ValueError("proposal generation needs macros; locked generation needs the accepted lock")
    if args.lock:
        lock = verify_lock(json.loads(Path(args.lock).read_text(encoding="utf-8")), proposal)
        if args.emit_directory:
            if not args.source:
                raise ValueError("source ownership is required for generation")
            emit_locked(lock, registry, Path(args.emit_directory), Path(args.source))
    elif args.emit_directory:
        raise ValueError("source generation requires the reviewed immutable lock")
    if output:
        output.write_bytes(canonical(proposal))
    print(json.dumps({"status": "LOCKED_GENERATION" if args.lock else proposal["status"],
                      "sha256": digest(output) if output else digest(args.lock),
                      "entries": len(proposal["entries"]), "review_cases": len(proposal["review_cases"])}))


if __name__ == "__main__":
    main()
