"""Derive bounded facility mon-set extents from the unchanged C declarations.

ARRAY_COUNT is evaluated by the original C compiler. No Python allocation,
terminator scanning, trainer-layout change or duplicated mon data is needed.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

INPUTS = ('src/data/battle_frontier/battle_frontier_trainers.h',
          'src/data/battle_frontier/battle_tent.h')
TABLES = ('gBattleFrontierTrainers', 'gSlateportBattleTentTrainers',
          'gVerdanturfBattleTentTrainers', 'gFallarborBattleTentTrainers')


def lexical(text):
    # Mask comments and quoted text while retaining original character offsets.
    pattern = r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text)


def close(code, first):
    depth = 0
    for i in range(first, len(code)):
        if code[i] == '{': depth += 1
        elif code[i] == '}':
            depth -= 1
            if not depth: return i
    raise ValueError('unterminated C initializer')


def derive(source):
    tables = {}
    hashes = {}
    for name in INPUTS:
        raw = (source / name).read_bytes()
        text = raw.decode('utf-8'); code = lexical(text)
        hashes[name] = hashlib.sha256(raw).hexdigest()
        for table in re.finditer(r'\bconst\s+struct\s+BattleFrontierTrainer\s+(\w+)\s*\[[^]]+\]\s*=\s*\{', code):
            name = table[1]
            if name not in TABLES or name in tables:
                raise ValueError('unregistered or duplicate facility table: ' + name)
            first = code.index('{', table.start()); last = close(code, first)
            position = first + 1; rows = []
            while position < last:
                skip = re.match(r'[\s,]*', code[position:last]); position += skip.end()
                if position == last: break
                row = re.match(r'\[([A-Za-z_]\w*)\]\s*=\s*\{', code[position:last])
                if row is None: raise ValueError('designated facility row required: ' + name)
                row_first = code.index('{', position); row_last = close(code, row_first)
                if row_last >= last: raise ValueError('row exceeds facility initializer')
                fields = list(re.finditer(r'\.monSet\s*=\s*', code[row_first:row_last]))
                if len(fields) != 1: raise ValueError('one original monSet required: ' + row[1])
                value = row_first + fields[0].end()
                expression = text[value:row_last].strip().rstrip(',').strip()
                if re.fullmatch(r'[A-Za-z_]\w*', expression):
                    declaration = re.search(r'\bconst\s+u16\s+' + re.escape(expression) + r'\s*\[\s*\]\s*=\s*\{', code)
                    if declaration is None: raise ValueError('defined original mon array required: ' + expression)
                elif not re.fullmatch(r'\(\s*const\s+u16\s*\[\s*\]\s*\)\s*\{[^{}]+\}', expression):
                    raise ValueError('unsupported original monSet expression: ' + row[1])
                if any(x['designator'] == row[1] for x in rows): raise ValueError('duplicate trainer designator')
                rows.append({'designator': row[1], 'expression': expression})
                position = row_last + 1
            if not rows: raise ValueError('nonempty facility table required')
            tables[name] = rows
    if set(tables) != set(TABLES): raise ValueError('complete four-table category required')
    lines = ['/* Derived original mon-set extents, including the terminator. */']
    for name in TABLES:
        lines.append(f'static const u32 sADMG_{name}MonSetExtents[ARRAY_COUNT({name})] = {{')
        for row in tables[name]:
            # Parentheses protect compound-literal commas from macro splitting.
            lines.append(f"    [{row['designator']}] = ARRAY_COUNT(({row['expression']})),")
        lines.append('};')
    header = ('\n'.join(lines) + '\n').encode('utf-8')
    record = {'format': 'admg101-facility-mon-set-extents', 'format_version': 1,
              'source_sha256': hashes, 'tables': tables,
              'header_sha256': hashlib.sha256(header).hexdigest(),
              'extent_includes_terminator': True, 'source_data_modified': False}
    return header, (json.dumps(record, sort_keys=True, indent=2) + '\n').encode('utf-8')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, default=Path('.'))
    parser.add_argument('--emit-directory', type=Path, default=Path('.'))
    args = parser.parse_args()
    header, record = derive(args.source)
    for name, raw in [('src/data/admg_facility_mon_extents.h', header),
                      ('tools/admg101/facility-mon-extents.json', record)]:
        target = args.emit_directory / name; target.parent.mkdir(parents=True, exist_ok=True)
        if not target.is_file() or target.read_bytes() != raw: target.write_bytes(raw)
    print(json.dumps({'status': 'GENERATED_DECLARATION_EXTENTS',
                      'rows': sum(len(x) for x in json.loads(record)['tables'].values())}))


if __name__ == '__main__': main()
