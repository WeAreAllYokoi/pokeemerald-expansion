"""Emit the reviewed trainer namespace. No allocation or campaign inference.

Array designators and trainer reference operands are distinct roles. The lock
is an approved source adaptation input, suitable for migration into the engine
fork; no project paths, build configurations or runtime region are inputs.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

PIN='c5f026363afda6dd56225bebb2f2d2cdcd9d84f0'
PROPOSAL_HASH='c186ab7fe8d84b63c376c6eb318f29889a8f6cd0509a8804d6641d0292b4360a'
ASSIGNMENTS_HASH='6561edb5b6e482aca6d86c75efafec3a65e2c5b29f132509986e5e1ffd1b257d'
STATE_LOCK_HASH='3e97899592e769294cba7239975dd4a339e277eb411eef5c569a606ebb63565e'


def canonical(value):return (json.dumps(value,sort_keys=True,indent=2)+'\n').encode()
def digest(raw):return hashlib.sha256(raw).hexdigest()


def reviewed(lock):
    if lock['status']!='IMMUTABLE_REVIEWED' or lock['source_commit']!=PIN:
        raise ValueError('accepted source and allocation lock required')
    if lock['accepted_proposal_sha256']!=PROPOSAL_HASH or lock['state_namespace_lock_sha256']!=STATE_LOCK_HASH:
        raise ValueError('accepted allocation/state foundation changed')
    if lock['reservation']!={'first':1024,'last':1791,'positions':768,
            'position_zero':'reserved; native placeholder accessible through owner/table lookup'}:
        raise ValueError('reviewed reservation changed')
    entries=sorted(lock['entries'],key=lambda r:(r['content_namespace'],r['native_table_index'],r['name']))
    if digest(canonical(entries))!=ASSIGNMENTS_HASH:
        raise ValueError('immutable trainer assignment drift')
    return entries


def emit(lock,source,reverse=False):
    entries=reviewed(lock)
    hoenn={r['name']:r['native_table_index']for r in entries if r['content_namespace']=='hoenn'}
    text=(source/'include/constants/opponents.h').read_text(encoding='utf-8')
    declarations={m[1]:int(m[2],0)for m in re.finditer(r'^#define[ \t]+(TRAINER_\w+)[ \t]+(0x[\da-fA-F]+|\d+)[ \t]*$',text,re.M)}
    if declarations!=hoenn:raise ValueError('all original Hoenn identities must remain unchanged')
    native=[r for r in entries if r['content_namespace']=='kanto']
    if len(native)!=624:raise ValueError('complete supplied native table required')
    if reverse:native.reverse()
    native.sort(key=lambda r:r['native_table_index'])
    lines=['/* Generated from the reviewed trainer lock; source ownership is not current region. */',
           '#ifndef ADMG_TRAINER_IDS_H','#define ADMG_TRAINER_IDS_H',
           '#define ADMG_KANTO_TRAINER_FIRST 0x0400','#define ADMG_KANTO_TRAINER_LAST 0x06FF',
           '#define ADMG_KANTO_TRAINER_SLOTS 768','#define ADMG_KANTO_TRAINERS_COUNT 624',
           '#define TRAINERS_COUNT_FRLG 624','#define MAX_TRAINERS_COUNT_FRLG 768',
           '#define ADMG_KANTO_DEFEAT_FLAG_FIRST 0x0E60','#define ADMG_KANTO_DEFEAT_FLAG_COUNT 768',
           '/* Zero remains a placeholder; its native row is selected through an owned lookup. */',
           '#ifndef TRAINER_NONE','#define TRAINER_NONE 0','#endif']
    for row in native:
        name=row['name'];index=row['native_table_index'];identifier=row['proposed_canonical_id']
        if name!='TRAINER_NONE':lines.append(f'#define {name} 0x{identifier:04X}')
        lines.append(f'#define ADMG_KANTO_INDEX_{name} {index}')
    lines+=['#endif','']
    operands={'format':'admg101-locked-trainer-operands','format_version':1,'source_commit':PIN,
        'trainer_lock_sha256':digest(canonical(lock)),'state_namespace_lock_sha256':STATE_LOCK_HASH,
        'entries':entries,'identity_inputs':['content_namespace','declared_symbol'],
        'array_designator_role':'native_table_index','reference_role':'proposed_canonical_id',
        'placeholder_role':'metadata only; owned table index zero; ordinary reference zero',
        'unused_positions_retained':True,'current_map_used':False,'progression_used':False,
        'gameplay_activation':'none','save_layout_modified':False}
    return {'include/constants/admg_trainer_ids.h':'\n'.join(lines).encode(),
            'tools/admg101/trainer-operands.json':canonical(operands)}


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--lock',required=True);parser.add_argument('--source',required=True)
    parser.add_argument('--emit-directory',required=True);parser.add_argument('--reverse-inputs',action='store_true')
    args=parser.parse_args()
    lock=json.loads(Path(args.lock).read_text(encoding='utf-8'))
    outputs=emit(lock,Path(args.source),args.reverse_inputs)
    for name,raw in outputs.items():
        target=Path(args.emit_directory)/name;target.parent.mkdir(parents=True,exist_ok=True)
        if target.exists():
            if target.read_bytes()!=raw:raise ValueError('preserve previous derived namespace; use a fresh candidate')
        else:target.write_bytes(raw)
    print(json.dumps({'status':'PASS_LOCKED_TRAINER_NAMESPACE','outputs':{name:digest(raw)for name,raw in outputs.items()}},sort_keys=True))


if __name__=='__main__':main()
