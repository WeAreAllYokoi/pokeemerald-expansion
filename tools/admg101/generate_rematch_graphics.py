"""Deterministic native pose metadata; preserve ordinary graphics and asset bindings."""
import argparse
import hashlib
import json
from pathlib import Path
import re

PIN = 'c5f026363afda6dd56225bebb2f2d2cdcd9d84f0'
INPUT_SHA256 = 'f0a8975c9d4f22c0bab857942c8792efff23c07a98d578aed471b6022dc10dbe'


def canonical(value):
    return (json.dumps(value,sort_keys=True,indent=2)+'\n').encode()


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def body_hash(text,symbol):
    matches = re.findall(r'\b'+re.escape(symbol)+r'\s*(?:\[\])?\s*=\s*\{(.*?)\};',text,re.S)
    if len(matches) != 1:
        raise ValueError('unique source descriptor required: '+symbol)
    return sha(matches[0].encode())


def emit(raw,source):
    if sha(raw) != INPUT_SHA256:
        raise ValueError('reviewed native graphics input changed')
    doc = json.loads(raw)
    if doc['target_commit'] != PIN or len(doc['bindings']) != 35:
        raise ValueError('complete pinned native pose bindings required')
    info = (source/'src/data/object_events/object_event_graphics_info.h').read_text(encoding='utf-8')
    pics = (source/'src/data/object_events/object_event_pic_tables.h').read_text(encoding='utf-8')
    pointers = (source/'src/data/object_events/object_event_graphics_info_pointers.h').read_text(encoding='utf-8')
    lines = ['/* Generated from reviewed native graphics provenance. No pixels or ordinary bindings change. */\n',
             '#ifndef ADMG_REMATCH_GRAPHICS_DATA_H\n#define ADMG_REMATCH_GRAPHICS_DATA_H\n',
             'const struct ADMG_NativeRematchGraphics gADMG_NativeRematchGraphics[] = {\n']
    operands = []
    for row in sorted(doc['bindings'],key=lambda r:r['graphics_symbol']):
        if body_hash(info,row['info_symbol']) != row['info_body_sha256']:
            raise ValueError('source graphics definition changed: '+row['info_symbol'])
        if body_hash(pics,row['native_picture_binding']) != row['picture_body_sha256']:
            raise ValueError('source picture table changed: '+row['native_picture_binding'])
        if not re.search(r'\['+re.escape(row['graphics_symbol'])+r'\]\s*=\s*&'+re.escape(row['info_symbol'])+r'\b',pointers):
            raise ValueError('source pointer catalogue changed: '+row['graphics_symbol'])
        if row['frames'] < 10 or row['frame_bytes'] <= 0 or sha((source/row['asset']).read_bytes()) != row['asset_sha256']:
            raise ValueError('native frame payload changed: '+row['asset'])
        if row['kind'] not in ('STOP','JUMP','SWIM'):
            raise ValueError('unknown native movement policy')
        lines.append('    { &'+row['info_symbol']+', '+row['native_picture_binding']+', '+str(row['frames'])+', '
                     +str(row['frame_bytes'])+', '+row['graphics_symbol']+', ADMG_NATIVE_REMATCH_'+row['kind']+' },\n')
        operands.append({key:row[key] for key in ('graphics_symbol','info_symbol','kind','frame_bytes','frames',
                                                 'ordinary_picture_binding','native_picture_binding','asset_sha256')})
    lines += ['};\nconst u32 gADMG_NativeRematchGraphicsCount = ARRAY_COUNT(gADMG_NativeRematchGraphics);\n',
              '_Static_assert(ARRAY_COUNT(gADMG_NativeRematchGraphics) == 35, "complete compatible native pose metadata");\n',
              '#endif\n']
    return {'include/data/admg_rematch_graphics.h':''.join(lines).encode(),
            'tools/admg101/native-rematch-graphics-operands.json':canonical({'format':'admg101-native-rematch-graphics-operands',
                'format_version':1,'target_commit':PIN,'input_sha256':INPUT_SHA256,'bindings':operands,
                'ordinary_animation_and_picture_bindings_changed':False})}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source',required=True)
    parser.add_argument('--output',required=True)
    args = parser.parse_args()
    source,output = Path(args.source),Path(args.output)
    raw = Path(__file__).with_name('native-rematch-graphics.json').read_bytes()
    outputs = emit(raw,source)
    for relative,data in outputs.items():
        target = output/relative
        target.parent.mkdir(parents=True,exist_ok=True)
        if target.exists():
            if target.read_bytes() != data:
                raise ValueError('preserve prior output; use a fresh candidate: '+relative)
        else:
            target.write_bytes(data)
    print(json.dumps({'status':'PASS_NATIVE_REMATCH_GRAPHICS_GENERATION','outputs':{name:sha(data) for name,data in outputs.items()}}))


if __name__=='__main__':
    main()
