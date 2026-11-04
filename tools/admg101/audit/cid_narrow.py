#!/usr/bin/env python3
"""Narrowings (clang all.log) of a task or sprite contentIds[]/contentId slot, which hold 32-bit content IDs.
   cid_narrow.py <all.log>"""
import re, sys
log = open(sys.argv[1], errors="replace").read().splitlines()
warn = re.compile(r"^(\S+?):(\d+):(\d+): warning: implicit conversion loses integer precision: '([^']+)'.* to '([^']+)'")
code = re.compile(r"^\s*(\d+) \| (.*)$")
out = set()
for i, line in enumerate(log):
    m = warn.match(line)
    if not m: continue
    for j in range(i + 1, min(i + 3, len(log))):
        c = code.match(log[j])
        if c:
            expr = c.group(2)[int(m.group(3)) - 1:]
            if re.match(r"[\w\[\]\.\->()]*contentIds?\b", expr):
                out.add(f"{m.group(1)}:{m.group(2)}: {m.group(4)} -> {m.group(5)}: {c.group(2).strip()[:160]}")
            break
print("\n".join(sorted(out)))
