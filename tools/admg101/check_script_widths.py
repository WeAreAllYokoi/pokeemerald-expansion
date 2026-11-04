"""Check that script macros write every argument at the width their command reads.

Battle scripts: each command reads its arguments through a packed struct over the script bytes (CMD_ARGS, NATIVE_ARGS),
so the field types decide the layout. The field types are measured with the build's own compiler (a probe of sizeof
for each type) and compared with the macro's .byte/.2byte/.4byte sequence.

Event scripts: each command reads its arguments one at a time (ScriptReadByte/Halfword/Word/MapId). The macro, with
nested macros expanded and every side of each conditional, is compared with the order and width of those reads.

A case the scan cannot decide (bitfields, conditionals, reads that branch or go through a helper) is accepted only if
tools/admg101/script-widths-reviewed.json lists it with the same signature, so a change to it is reviewed again.

Why: this fork widens content IDs (abilities, species, moves, items, map IDs) to 32 bits, and a macro left at the old
width makes the command after it read 2 bytes off (recipe 40: jumpifability, jumpifabilitypresent, jumpifspecies,
showplayer, hideplayer). Run by `make check`; by hand:

python3 -B tools/admg101/check_script_widths.py --cpp "<cpp and flags>" --preproc "<preproc and flags>" \
    --cc1 "<cc1 and flags>" --as "<as and flags>" --nm <nm> --work <folder> [--write-reviewed]
"""
import argparse
import json
import re
import shlex
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
REVIEWED = Path(__file__).with_name('script-widths-reviewed.json')
WIDTH = {'.byte': 1, '.2byte': 2, '.4byte': 4}
COND = re.compile(r'^\.(if\w*)\b')
READS = {'ScriptReadByte': 1, 'ScriptReadHalfword': 2, 'ScriptReadWord': 4, 'ScriptReadMapId': 4}


def read(path):
    return (ROOT / path).read_text(encoding='utf-8', errors='replace')


def load_macros(texts):
    macros = {}
    for text in texts:
        for m in re.finditer(r'^\s*\.macro\s+(\w+)([^\n]*)\n(.*?)^\s*\.endm', text, re.S | re.M):
            lines = [l.split('@')[0].strip() for l in m.group(3).splitlines()]
            macros[m.group(1)] = [l for l in lines if l]
    return macros


def sequences(macros, lines, depth=0, limit=64):
    """Every width sequence the lines can emit: nested macros expanded, each conditional both ways, .error sides dropped."""
    if depth > 12:
        return {('?',)}
    result = [()]
    i = 0
    while i < len(lines):
        line = lines[i]
        if COND.match(line):
            level, j, branches, cur = 1, i + 1, [], []
            while j < len(lines):
                l = lines[j]
                if COND.match(l):
                    level += 1
                elif l.startswith('.endif'):
                    level -= 1
                    if level == 0:
                        break
                elif level == 1 and l.startswith('.else'):
                    branches.append(cur)
                    cur = []
                    j += 1
                    continue
                cur.append(l)
                j += 1
            branches.append(cur)
            if len(branches) == 1:
                branches.append([])
            branches = [b for b in branches if not any(x.startswith('.error') for x in b)] or [[]]
            alts = set()
            for b in branches:
                alts |= sequences(macros, b, depth + 1, limit)
            result = [r + a for r in result for a in sorted(alts)][:limit]
            i = j + 1
            continue
        head = line.split()[0].rstrip(',')
        if head in WIDTH:
            count = len([x for x in re.split(r',(?![^(]*\))', line[len(head):]) if x.strip()]) or 1
            result = [r + (WIDTH[head],) * count for r in result]
        elif head in macros:
            alts = sequences(macros, macros[head], depth + 1, limit)
            result = [r + a for r in result for a in sorted(alts)][:limit]
        i += 1
    return set(result)


OPCODE = re.compile(r'^(\.byte\s+(B_SCR_OP_|SCR_OP_)\w+|callnative\b)')


def emits_command(macros, name, depth=0):
    """Whether a macro writes a command of its own (an opcode or a callnative), not just data for one."""
    if depth > 12 or name not in macros:
        return False
    for line in macros[name]:
        if OPCODE.match(line):
            return True
        head = line.split()[0].rstrip(',')
        if head in macros and emits_command(macros, head, depth + 1):
            return True
    return False


def arguments(macros, lines):
    """The lines after a command's opcode that are its arguments: up to the next command, or the end of the
    conditional the opcode is in."""
    out, level = [], 0
    for line in lines:
        if COND.match(line):
            level += 1
        elif line.startswith('.endif') or line.startswith('.else'):
            if level == 0:
                break
            if line.startswith('.endif'):
                level -= 1
        elif OPCODE.match(line):
            break
        else:
            head = line.split()[0].rstrip(',')
            if head in macros and emits_command(macros, head):
                break
        out.append(line)
    return out


def split_args(text):
    parts, depth, cur = [], 0, ''
    for ch in text:
        depth += ch in '([{'
        depth -= ch in ')]}'
        if ch == ',' and depth == 0:
            parts.append(cur.strip())
            cur = ''
        else:
            cur += ch
    if cur.strip():
        parts.append(cur.strip())
    return parts


def battle_commands():
    """name -> list of (type, count) for each CMD_ARGS/NATIVE_ARGS command; None for bitfield commands."""
    commands = {}
    for path in sorted((ROOT / 'src').glob('*.c')):
        text = path.read_text(encoding='utf-8', errors='replace')
        for m in re.finditer(r'^(?:static\s+)?void\s+(\w+)\s*\(void\)\s*\{(.*?)^\}', text, re.S | re.M):
            a = re.search(r'\b(CMD_ARGS|NATIVE_ARGS)\((.*?)\);', m.group(2), re.S)
            if not a:
                continue
            decls = split_args(a.group(2)) if a.group(2).strip() else []
            fields = []
            for d in decls:
                if '(*' in d:
                    fields.append(('void *', 1))
                    continue
                f = re.match(r'^(.*?)(\w+)\s*(?:\[(\w+)\])?\s*(?::\s*\d+)?$', d.strip())
                fields.append((f.group(1).strip(), f.group(3) or '1', ':' in d))
            if any(len(f) == 3 and not f[0] for f in fields):
                continue  # battle animation CMD_ARGS: untyped s16 fields over gBattleAnimArgs, not script bytes
            if any(len(f) == 3 and f[2] for f in fields):
                commands[m.group(1)] = None
                continue
            fields = [(f[0], f[1]) for f in fields]
            if a.group(1) == 'NATIVE_ARGS':
                fields = [('void *', 1)] + fields
            commands[m.group(1)] = fields
    return commands


def measure(types, tools, work):
    """sizeof each type, compiled with the build's own pipeline."""
    work.mkdir(parents=True, exist_ok=True)
    includes = re.findall(r'^#include .*$', read('src/battle_script_commands.c'), re.M)
    probe = work / 'script_widths_probe.c'
    probe.write_text('\n'.join(includes + [''] + [f'const u8 gScriptWidthsProbe{i}[sizeof({t})] = {{0}};'
                                                  for i, t in enumerate(types)]) + '\n', encoding='utf-8', newline='\n')
    obj = work / 'script_widths_probe.o'
    q = shlex.quote
    # -iquote src: the probe carries battle_script_commands.c's includes, some relative to src/
    command = (f'{tools.cpp} -iquote src {q(str(probe))} |{tools.preproc} {q(str(probe))} charmap.txt | {tools.cc1} -o - - '
               f'| {tools.as_} -o {q(str(obj))} -')
    subprocess.run(['bash', '-c', 'set -o pipefail; ' + command], cwd=ROOT, check=True)
    nm = subprocess.run([tools.nm, '-S', str(obj)], capture_output=True, text=True, check=True).stdout
    sizes = {}
    for line in nm.splitlines():
        m = re.match(r'^[0-9a-f]+ ([0-9a-f]+) \w gScriptWidthsProbe(\d+)$', line.strip())
        if m:
            sizes[types[int(m.group(2))]] = int(m.group(1), 16)
    missing = [t for t in types if t not in sizes]
    if missing:
        raise SystemExit(f'the probe measured no size for {missing}')
    return sizes


def check_battle(tools, work):
    commands = battle_commands()
    types = sorted({t for c in commands.values() if c for t, _ in c})
    size = measure(types, tools, work)
    op_to_cmd = dict(re.findall(r'\[(B_SCR_OP_\w+)\]\s*=\s*(\w+),', read('src/battle_script_commands.c')))
    macros = load_macros([read(p.relative_to(ROOT)) for p in sorted((ROOT / 'asm/macros').glob('*.inc'))])
    own = load_macros([read('asm/macros/battle_script.inc')])
    checked, cases = 0, {}
    for name, lines in own.items():
        if not lines:
            continue
        m = re.match(r'^\.byte\s+(B_SCR_OP_\w+)$', lines[0])
        n = re.match(r'^callnative\s+(\w+)$', lines[0])
        if m and m.group(1) in op_to_cmd:
            command, prefix = op_to_cmd[m.group(1)], ()
        elif n:
            command, prefix = n.group(1), (4,)
        else:
            continue
        if command not in commands:
            continue
        emitted = sorted(prefix + s for s in sequences(macros, arguments(macros, lines[1:])))
        fields = commands[command]
        reads = None if fields is None else [size[t] * (int(c) if str(c).isdigit() else 1) for t, c in fields]
        checked += 1
        if reads is not None and len(emitted) == 1 and list(emitted[0]) == reads:
            continue
        cases[f'battle:{name}'] = {'command': command, 'emitted': [list(e) for e in emitted], 'reads': reads}
    return checked, cases


def script_functions():
    functions = {}
    for path in sorted((ROOT / 'src').glob('*.c')):
        text = path.read_text(encoding='utf-8', errors='replace')
        for m in re.finditer(r'^\w[\w\s\*]*?\b(\w+)\s*\(struct ScriptContext\s*\*\s*ctx\)[^{;]*\{(.*?)^\}', text, re.S | re.M):
            functions[m.group(1)] = m.group(2)
    return functions


def check_event():
    table = read('data/script_cmd_table.inc')
    op_to_cmd = dict(re.findall(r'script_cmd_table_entry\s+(SCR_OP_\w+)\s+(\w+)', table))
    functions = script_functions()
    sources = '\n'.join(p.read_text(encoding='utf-8', errors='replace') for p in sorted((ROOT / 'src').glob('*.c')))
    # natives that take no arguments: `void Name(void)` definitions, brace on the same or the next line
    void_natives = set(re.findall(r'^\w[\w\s\*]*?\b(\w+)\s*\(void\)\s*(?:\{|$)', sources, re.M))
    macros = load_macros([read(p.relative_to(ROOT)) for p in sorted((ROOT / 'asm/macros').glob('*.inc'))])
    own = load_macros([read('asm/macros/event.inc')])
    checked, cases = 0, {}
    for name, lines in own.items():
        for i, line in enumerate(lines):
            m = re.match(r'^\.byte\s+(SCR_OP_\w+)$', line)
            n = re.match(r'^callnative\s+(\w+)', line)
            if not (m or n):
                continue
            if m and m.group(1) == 'SCR_OP_CALLNATIVE':
                break
            command = op_to_cmd.get(m.group(1)) if m else n.group(1)
            body = functions.get(command)
            emitted = sorted(sequences(macros, arguments(macros, lines[i + 1:])))
            if body is None and command in void_natives:
                body = ''  # a native that takes no arguments reads none
            reads = None if body is None else [READS[c] for c in re.findall(
                r'\b(ScriptRead(?:Byte|Halfword|Word|MapId))\s*\(ctx\)', body)]
            checked += 1
            if reads is not None and tuple(reads) in set(emitted) and len(emitted) == 1:
                break
            cases[f'event:{name}'] = {'command': command, 'emitted': [list(e) for e in emitted], 'reads': reads}
            break
    return checked, cases


def main():
    parser = argparse.ArgumentParser()
    for option in ('cpp', 'preproc', 'cc1', 'as', 'nm', 'work'):
        parser.add_argument('--' + option, required=True, dest=option.rstrip('_') + ('_' if option == 'as' else ''))
    parser.add_argument('--write-reviewed', action='store_true', help='record the current undecided cases (after review)')
    tools = parser.parse_args()
    battle_checked, battle = check_battle(tools, Path(tools.work))
    event_checked, event = check_event()
    cases = {**battle, **event}
    reviewed = json.loads(REVIEWED.read_text(encoding='utf-8')) if REVIEWED.exists() else {}
    if tools.write_reviewed:
        out = {k: {**v, 'why': reviewed.get(k, {}).get('why', 'TODO: say why this is correct')} for k, v in sorted(cases.items())}
        REVIEWED.write_text(json.dumps(out, indent=1) + '\n', encoding='utf-8', newline='\n')
        print(f'wrote {len(out)} cases to {REVIEWED.name}; give each its reason')
        return 0
    problems = []
    for key, case in sorted(cases.items()):
        known = reviewed.get(key)
        if known is None:
            problems.append(f'{key} -> {case["command"]}: writes {case["emitted"]}, reads {case["reads"]}')
        elif {k: known.get(k) for k in case} != case:
            problems.append(f'{key} -> {case["command"]} changed since its review: writes {case["emitted"]}, reads {case["reads"]}')
    for key in sorted(set(reviewed) - set(cases)):
        problems.append(f'{key}: reviewed, but no longer undecided; remove it from {REVIEWED.name}')
    print(f'script widths: {battle_checked} battle-script and {event_checked} event-script macros, '
          f'{len(cases)} reviewed by hand')
    if problems:
        print('\n'.join(problems))
        print(f'{len(problems)} macros write a width their command does not read, or changed since review')
        return 1
    print('PASS')
    return 0


if __name__ == '__main__':
    sys.exit(main())
