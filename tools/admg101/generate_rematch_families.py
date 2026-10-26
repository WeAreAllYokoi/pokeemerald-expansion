"""Emit the complete reviewed FR/LG family provider using locked symbols."""
import argparse
import hashlib
import json
from pathlib import Path

PIN = 'c5f026363afda6dd56225bebb2f2d2cdcd9d84f0'
REFERENCE = '037335f4c725d7c9aecdac87066f2002b4bd7e14'
LOCK_SHA = '703ce3ae593e7e22704ba2ee38dc11d66c00391e0a2a454824f25c9c260229cb'
AUTHORITY_SHA = '3b36bdfa5f8f83506106058e97432260964b24ff1c7d98cb0bdef2429e8514aa'


def canonical(value):
    return (json.dumps(value, sort_keys=True, indent=2) + '\n').encode()


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def emit(authority, lock):
    if authority['source_commit'] != REFERENCE or authority['target_commit'] != PIN:
        raise ValueError('pinned relationship authority required')
    if lock['source_commit'] != PIN or lock['status'] != 'IMMUTABLE_REVIEWED':
        raise ValueError('reviewed trainer identity lock required')
    entries = {r['name']: r for r in lock['entries'] if r['content_namespace'] == 'kanto'}
    if len(entries) != 624:
        raise ValueError('complete retained native trainer table required')
    families = sorted(authority['families'], key=lambda r: r['reference_row'])
    if sha(canonical(lock)) != LOCK_SHA:
        raise ValueError('immutable reviewed trainer assignments changed')
    if sha(canonical(dict(authority, families=families))) != AUTHORITY_SHA:
        raise ValueError('accepted source-proven family data changed')
    if len(families) != 221 or [r['reference_row'] for r in families] != list(range(221)):
        raise ValueError('complete original family order required')
    lines = ['/* Generated explicit source-proven families. Do not compact sparse slots. */',
             '#ifndef ADMG_REMATCH_FAMILIES_H', '#define ADMG_REMATCH_FAMILIES_H',
             '#include "constants/admg_trainer_ids.h"',
             'const u16 gADMG_KantoRematchFamilies[221][6] = {']
    rows, members, bases, skips, repeats = [], set(), set(), 0, 0
    for row in families:
        slots = row['slots']
        if len(slots) != 6 or [r['slot'] for r in slots] != list(range(6)):
            raise ValueError('all six positions must be retained')
        values, operands, terminated = [], [], False
        seen = []
        for slot in slots:
            kind, symbol = slot['kind'], slot['symbol']
            if kind == 'trainer':
                entry = entries.get(symbol)
                if terminated or entry is None or entry['native_table_index'] != slot['target_native_ordinal']:
                    raise ValueError('unmapped or post-terminator trainer: ' + symbol)
                value = entry['proposed_canonical_id']
                if not 1025 <= value <= 1647:
                    raise ValueError('ordinary native identity required')
                operands.append(symbol); members.add(symbol); seen.append(symbol)
            elif kind == 'skip':
                if terminated or symbol != 'SKIP':
                    raise ValueError('invalid SKIP placement')
                value = 65535; operands.append('0xFFFF'); skips += 1
            elif kind == 'terminator':
                if symbol != 'TRAINER_NONE':
                    raise ValueError('original zero terminator required')
                terminated = True; value = 0; operands.append('0')
            else:
                raise ValueError('unknown slot semantics')
            values.append(value)
        if slots[0]['kind'] != 'trainer' or slots[0]['symbol'] != row['base_symbol'] or values[0] in bases:
            raise ValueError('unique source-declared family base required')
        bases.add(values[0]); repeats += len(seen) != len(set(seen))
        lines.append('    {' + ', '.join(operands) + '},')
        rows.append({'reference_row': row['reference_row'], 'base_symbol': row['base_symbol'],
                     'canonical_slots': values, 'symbols': [r['symbol'] for r in slots],
                     'initializer_sha256': row['initializer_sha256']})
    if (len(members), skips, repeats) != (351, 240, 136):
        raise ValueError('accepted complete capability inventory changed')
    lines += ['};', '#endif', '']
    manifest = {'format': 'admg101-native-rematch-operands', 'format_version': 1,
                'source_commit': PIN, 'relationship_commit': REFERENCE,
                'authority_sha256': sha(canonical(dict(authority, families=families))),
                'trainer_lock_sha256': sha(canonical(lock)),
                'family_count': 221, 'slot_count': 6, 'distinct_members': 351,
                'skip_positions': skips, 'families_with_repeated_identity': repeats,
                'other_retained_rows': 273, 'current_map_used_for_identity': False,
                'campaign_used_for_identity': False, 'rows': rows}
    return {'include/data/admg_rematch_families.h': '\n'.join(lines).encode(),
            'tools/admg101/rematch-operands.json': canonical(manifest)}


def main():
    parser = argparse.ArgumentParser()
    for name in ('authority', 'lock', 'emit-directory'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    outputs = emit(json.loads(Path(args.authority).read_text(encoding='utf-8')),
                   json.loads(Path(args.lock).read_text(encoding='utf-8')))
    for name, raw in outputs.items():
        path = Path(args.emit_directory) / name
        path.parent.mkdir(parents=True, exist_ok=True)
        if path.exists():
            if path.read_bytes() != raw:
                raise ValueError('preserve prior output; use a fresh candidate')
        else:
            path.write_bytes(raw)
    print(json.dumps({'status': 'PASS_COMPLETE_NATIVE_REMATCH_GENERATION',
                      'outputs': {name: sha(raw) for name, raw in outputs.items()}}, sort_keys=True))


if __name__ == '__main__':
    main()
