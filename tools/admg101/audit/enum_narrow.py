#!/usr/bin/env python3
"""Narrowings (clang all.log) whose source type is a content enum or content typedef: a 32-bit content ID cut to a narrower
type. Every content enum ends in <KIND>_ID_MAX = 0xFFFFFFFF, so the compiler sees each of these.   enum_narrow.py <all.log>"""
import re, sys
CONTENT = r"enum (Species|Move|Item|Ability|Type|TrainerPicID|TrainerClassID|TrainerId|NationalDexOrder|MoveEffect|BattleMoveEffects)|nature_t|mapsec_u8_t|mapsec_u16_t|mapsec_s16_t|metloc_u8_t"
log = open(sys.argv[1], errors="replace").read().splitlines()
warn = re.compile(r"^(\S+?):(\d+):(\d+): warning: implicit conversion loses integer precision: '(" + CONTENT + r")'.* to '([^']+)'")
code = re.compile(r"^\s*(\d+) \| (.*)$")
out = set()
for i, line in enumerate(log):
    m = warn.match(line)
    if not m:
        continue
    text = ""
    for j in range(i + 1, min(i + 3, len(log))):
        c = code.match(log[j])
        if c:
            text = c.group(2)
            break
    out.add(f"{m.group(1)}:{m.group(2)}: {m.group(4)} -> {m.groups()[-1]}: {text.strip()[:160]}")
print("\n".join(sorted(out)))
