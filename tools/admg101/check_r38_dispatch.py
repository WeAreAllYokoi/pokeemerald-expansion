"""Fail a recipe-38 preparation whose recompiled code has a computed jump GBARecomp could not resolve.

A Thumb `mov pc, rN` is how GCC dispatches a switch through a jump table. GBARecomp follows the tables it can trace
(coverage manifest `jump_tables`); one it cannot trace (for example a table address GCC spilled to the stack) compiles to
code that throws the first time the switch runs ("the recompiled code can't follow it there"). Such a jump is listed in
`indirect_jumps` but not in `jump_tables`. Found by the bucket-B smoke runs in MonGainEVs (patch 0080): every won battle hit it.

The upstream script engine has two deliberate computed jumps, __builtin_longjmp out of a script run (Script_GotoBreak_Internal,
Script_RequestEffects_Internal);
they are allowed by name. Anything else fails.
Usage: check_r38_dispatch.py <coverage-r1.json> <pokeemerald.gba>
"""
import json
import sys

ALLOWED = {'Script_GotoBreak_Internal', 'Script_RequestEffects_Internal'}
ROM_BASE = 0x08000000


def unresolved_dispatches(coverage, rom):
    found = []
    for function in coverage['functions']:
        if function['mode'] != 'thumb':
            continue
        tables = {table['at'] for table in function['jump_tables']}
        for address in function['indirect_jumps']:
            # Functions copied to RAM run at `address` but are stored at `rom_address`
            stored = address - function['address'] + function['rom_address'] if function['rom_address'] else address
            offset = stored - ROM_BASE
            if not 0 <= offset < len(rom) - 1:
                continue
            half = rom[offset] | rom[offset + 1] << 8
            # Thumb high-register MOV (0100 0110 H1 H2 Rm Rd) whose destination (H1:Rd) is PC
            if half & 0xFF00 == 0x4600 and half >> 7 & 1 and half & 7 == 7 and address not in tables:
                found.append((function['name'], address))
    return found


def main():
    coverage = json.load(open(sys.argv[1], encoding='utf-8'))
    rom = open(sys.argv[2], 'rb').read()
    found = unresolved_dispatches(coverage, rom)
    allowed = [f for f in found if f[0] in ALLOWED]
    failed = [f for f in found if f[0] not in ALLOWED]
    for name, address in failed:
        print(f'UNRESOLVED computed jump in {name} at 0x{address:08X}: the recompiled code throws when it runs')
    print(f'computed jumps: {len(found)} unresolved, {len(allowed)} allowed (upstream script engine), {len(failed)} failing')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
