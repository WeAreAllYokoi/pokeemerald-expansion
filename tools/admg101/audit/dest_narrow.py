#!/usr/bin/env python3
"""Narrowings whose destination (the assigned variable, member or slot, or the declared local) is named like a content ID."""
import re, sys

log = open(sys.argv[1], errors="replace").read().splitlines()
warn = re.compile(r"^(\S+?):(\d+):(\d+): warning: implicit conversion loses integer precision: '([^']+)'.* to '([^']+)'")
code = re.compile(r"^\s*(\d+) \| (.*)$")
content = re.compile(r"(species|move|item|abilit|nature|trainerid|opponent|heldItem|dexNum|nationalNum|typeId|^type\d?$|mapsec|metLocation)", re.I)
notcontent = re.compile(r"(count|num|index|idx|slot|pos|cursor|state|timer|speed|step|dir|delay|amount|menu|window|sprite|anim|offset|x$|y$|width|height|pp|power|accuracy|priority|target|flags?$|quantity|price|pocket|category|effect$|movement|moving|moved|remove|pal|tile)", re.I)
out = set()
for i, line in enumerate(log):
    m = warn.match(line)
    if not m:
        continue
    for j in range(i + 1, min(i + 3, len(log))):
        c = code.match(log[j])
        if not c:
            continue
        text = c.group(2)
        lhs = text[:int(m.group(3)) - 1].rstrip()
        d = re.search(r"([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*(=|\+=|\|=|-=)\s*(\(\s*[\w ]+\)\s*)?$", lhs)
        if d:
            name = d.group(1)
            if content.search(name) and not notcontent.search(name):
                out.add(f"{m.group(1)}:{m.group(2)}: {m.group(4)} -> {m.group(5)}: {text.strip()[:150]}")
        break
print("\n".join(sorted(out)))
