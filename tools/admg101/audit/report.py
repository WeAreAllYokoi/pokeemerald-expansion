#!/usr/bin/env python3
"""Applies the allowlist of reviewed findings to the audit's filter outputs and writes one report.

  report.py <run directory> <allowlist.txt> <report.md>

The run directory holds one <filter>.txt per filter (one finding per line, "<location>[:<line>]: <detail>") and
meta.txt (key: value lines for the report's header). An allowlist entry names a filter, a location without its line
number, the exact detail text and the reason it was accepted; it accepts exactly one finding, so a second identical
site, or any changed text, is reported as unreviewed. Exit status: 0 when every finding is reviewed, 1 otherwise,
2 when the allowlist is malformed."""
import collections, pathlib, re, sys

FILTERS = [
    ("clang-failed", "Sources clang could not compile (their conversions are not audited)"),
    ("enum", "Content enum or content typedef narrowed (enum_narrow.py)"),
    ("mondata", "Content value read through the GetMonData family narrowed (mondata_narrow.py)"),
    ("cid", "Task or sprite contentIds[] slot narrowed (cid_narrow.py)"),
    ("dest", "Narrowing into a destination named like a content ID (dest_narrow.py)"),
    ("data", "32-bit content-kind store into a 16-bit task/sprite data[] slot (data_narrow.py)"),
    ("gfx", "Graphics identity (tag, graphics ID, picture or icon palette index) narrowed from a content ID (gfx_narrow.py, F1.6)"),
    ("dwarf", "Struct member or global narrower than 32 bits that holds or is named like a content ID (dwarf_audit.py)"),
    ("layout", "Layout of an assembly-emitted struct differs from the baseline tree (asm_layout_probe.sh)"),
]
LOC = re.compile(r"^(?P<loc>.+?)(?::(?P<line>\d+))?: (?P<detail>.*)$")


def parse_allowlist(path):
    reasons, entries, errors = {}, [], []
    section = None
    for n, raw in enumerate(pathlib.Path(path).read_text(encoding="utf-8").splitlines(), 1):
        line = raw.rstrip()
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        if line in ("[reasons]", "[entries]"):
            section = line
            continue
        if section == "[reasons]":
            m = re.match(r"^([a-z0-9-]+):\s+(\S.*)$", line)
            if not m:
                errors.append(f"line {n}: reason must be '<id>: <text>'")
            else:
                reasons[m.group(1)] = m.group(2)
        elif section == "[entries]":
            parts = [p.strip() for p in line.split(" | ", 3)]
            if len(parts) != 4 or not all(parts):
                errors.append(f"line {n}: entry must be '<reason> | <filter> | <location> | <detail>'")
                continue
            entries.append((n, *parts))
        else:
            errors.append(f"line {n}: outside [reasons]/[entries]")
    for n, reason, filt, _, _ in entries:
        if reason not in reasons:
            errors.append(f"line {n}: reason '{reason}' is not defined")
        if filt not in dict(FILTERS):
            errors.append(f"line {n}: unknown filter '{filt}'")
    return reasons, entries, errors


def main():
    run, allow_path, out_path = pathlib.Path(sys.argv[1]), sys.argv[2], pathlib.Path(sys.argv[3])
    reasons, entries, errors = parse_allowlist(allow_path)
    if errors:
        print("allowlist errors:\n  " + "\n  ".join(errors), file=sys.stderr)
        return 2
    allowed = collections.defaultdict(list)  # (filter, location, detail) -> entry line numbers, one per accepted finding
    for n, reason, filt, loc, detail in entries:
        allowed[(filt, loc, detail)].append((n, reason))
    meta = {}
    if (run / "meta.txt").exists():
        for line in (run / "meta.txt").read_text(encoding="utf-8").splitlines():
            if ": " in line:
                k, v = line.split(": ", 1)
                meta[k] = v

    unreviewed, accepted_by_reason, totals = {}, collections.Counter(), {}
    used = collections.Counter()
    for filt, _ in FILTERS:
        p = run / f"{filt}.txt"
        if not p.exists():
            totals[filt] = None
            continue
        rows = [r for r in p.read_text(encoding="utf-8", errors="replace").splitlines() if r.strip()]
        totals[filt] = len(rows)
        new = []
        for r in rows:
            m = LOC.match(r)
            key = (filt, m.group("loc"), m.group("detail").rstrip()) if m else (filt, r.rstrip(), "")
            if used[key] < len(allowed.get(key, ())):
                accepted_by_reason[allowed[key][used[key]][1]] += 1
                used[key] += 1
            else:
                new.append(r)
        unreviewed[filt] = new
    stale = []
    for key, lst in allowed.items():
        stale.extend((n, *key) for n, _ in lst[used[key]:])
    stale.sort()
    ran = {f for f, t in totals.items() if t is not None}
    stale = [s for s in stale if s[1] in ran]  # entries of a filter that did not run are not stale

    total_new = sum(len(v) for v in unreviewed.values())
    w = []
    w.append("# Dynamic-content capacity and narrowing audit\n")
    for k, v in meta.items():
        w.append(f"- **{k}**: {v}")
    w.append("")
    w.append(f"**Result: {total_new} unreviewed finding(s)**, {sum(accepted_by_reason.values())} reviewed and accepted "
             f"(allowlist), {len(stale)} allowlist entr{'y' if len(stale) == 1 else 'ies'} matching nothing.\n")
    w.append("| Filter | What it finds | Findings | Accepted | Unreviewed |")
    w.append("| --- | --- | ---: | ---: | ---: |")
    for filt, desc in FILTERS:
        t = totals[filt]
        if t is None:
            w.append(f"| `{filt}` | {desc} | not run | | |")
        else:
            w.append(f"| `{filt}` | {desc} | {t} | {t - len(unreviewed[filt])} | {len(unreviewed[filt])} |")
    w.append("")
    if total_new:
        w.append("## Unreviewed findings\n")
        w.append("Each is a possible capacity or narrowing defect: fix it in the source series, or review it and add an "
                 "allowlist entry with its reason. Nothing is accepted automatically.\n")
        for filt, _ in FILTERS:
            if unreviewed.get(filt):
                w.append(f"### `{filt}`\n")
                w.append("```text")
                w.extend(unreviewed[filt])
                w.append("```\n")
    if stale:
        w.append("## Allowlist entries matching nothing\n")
        w.append("The code they accepted changed or is gone; remove them or review the new text.\n")
        w.append("```text")
        w.extend(f"allowlist line {n}: {filt} | {loc} | {detail}" for n, filt, loc, detail in stale)
        w.append("```\n")
    w.append("## Accepted findings by reason\n")
    w.append("| Reason | Accepted | Why |")
    w.append("| --- | ---: | --- |")
    for rid, text in reasons.items():
        w.append(f"| `{rid}` | {accepted_by_reason[rid]} | {text} |")
    out_path.write_text("\n".join(w) + "\n", encoding="utf-8")
    print(f"audit: {total_new} unreviewed, {sum(accepted_by_reason.values())} accepted, {len(stale)} stale allowlist entries -> {out_path}")
    return 1 if total_new else 0


if __name__ == "__main__":
    sys.exit(main())
