#!/usr/bin/env python3
"""F1.6: narrowings into a graphics identity (a sprite or palette tag, a graphics ID, a picture or icon palette index)
whose value comes from a content ID (a species, item, trainer picture or other registry ID). A 16-bit tag computed from a
32-bit species is where an added species' picture would collide with another's or with a fixed tag."""
import re, sys

log = open(sys.argv[1], errors="replace").read().splitlines()
warn = re.compile(r"^(\S+?):(\d+):(\d+): warning: implicit conversion loses integer precision: '([^']+)'.* to '([^']+)'")
code = re.compile(r"^\s*(\d+) \| (.*)$")
graphics = re.compile(r"(tag|graphicsId|gfxId|picId|iconPal)", re.I)
# Sprite indices, OAM palette slots and templates are small slots the game allocates, not identities derived from content
slot = re.compile(r"(spriteId|SpriteId|paletteNum|palIndex|palSlot|Slot|template|Template)$")
content = re.compile(r"(species|item|trainerPic|picId|SPECIES_|ITEM_|TRAINER_PIC_|heldItem|natDex)", re.I)
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
        column = int(m.group(3)) - 1
        lhs, rhs = text[:column].rstrip(), text[column:]
        d = re.search(r"([A-Za-z_]\w*)\s*(\[[^\]]*\])?\s*(=|\+=|\|=|-=)\s*(\(\s*[\w ]+\)\s*)?$", lhs)
        destination = d.group(1) if d else None
        # A call argument (no assignment): the parameter is unknown here, so the call's name stands for the destination
        call = re.search(r"([A-Za-z_]\w*)\s*\([^()]*$", lhs) if not d else None
        name = destination or (call.group(1) if call else "")
        if name and graphics.search(name) and not slot.search(name) and content.search(rhs):
            out.add(f"{m.group(1)}:{m.group(2)}: {m.group(4)} -> {m.group(5)}: {text.strip()[:150]}")
        break
print("\n".join(sorted(out)))
