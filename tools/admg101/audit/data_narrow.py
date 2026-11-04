#!/usr/bin/env python3
"""32-bit stores into a 16-bit task or sprite data[] slot (clang all.log) whose line mentions a content kind: a content
ID kept in data[] is cut to 16 bits (the send-out cry of patch 0023). Content IDs a task keeps belong in contentIds[].
   data_narrow.py <all.log> [--all]      --all: every 32-bit data[] store, without the content-kind stage"""
import re, sys
log = open(sys.argv[1], errors="replace").read().splitlines()
warn = re.compile(r"^(\S+?):(\d+):(\d+): warning: implicit conversion loses integer precision: '([^']+)'.* to '(s16|u16|short)'")
code = re.compile(r"^\s*(\d+) \| (.*)$")
# the content-kind stage (run_filters.sh's grep in the original audit)
content = re.compile(r"species|move|item|abilit|nature|MON_DATA_(SPECIES|HELD|MOVE|HIDDEN)|gSpecialVar|VarGet|enum (Species|Move|Item|Ability|Type|Nature)", re.I)
out = set()
for i, line in enumerate(log):
    m = warn.match(line)
    if not m:
        continue
    for j in range(i + 1, min(i + 3, len(log))):
        c = code.match(log[j])
        if c:
            text = c.group(2); col = int(m.group(3)) - 1
            lhs = text[:col]
            # a store into data[...] (task or sprite), or an argument into a data slot
            if re.search(r"data\[[^\]]*\]\s*(=|\+=|\|=)\s*[^=]*$", lhs):
                row = f"{m.group(1)}:{m.group(2)}: {m.group(4)} | {text.strip()[:170]}"
                # the content-kind stage looks at the source type and the code, not the file name (battle_anim_mon_movement.c)
                if "--all" in sys.argv or content.search(f"{m.group(4)} | {text}"):
                    out.add(row)
            break
print("\n".join(sorted(out)))
