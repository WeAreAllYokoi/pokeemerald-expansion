#!/usr/bin/env python3
"""Measure how Expansion's fixed configuration options change compiled data tables.

A fixed option (include/constants/admg_config_catalog.h) is read where the compiler bakes its value into data, for example a
move's power in gMovesInfo. For each such option and each value it can take, this compiles the data file with only that
option changed (in a private copy of include/, so the source tree is untouched), and compares every measured table in the
object with the default compile, field by field, using the descriptor's struct layouts.

The result lists, per option and value, the table rows and fields whose values differ. The SDK writes those fields at
content composition when a mod selects the option. A pointer field is written as placed data (local text or a list without
pointers of its own), as a named global the game already has, or as NULL. Anything else (a pointer to other local data with
pointers inside, or a change in data the descriptor does not measure that is not reached only through measured pointer
fields) is reported under "unsupported" and that option stays fixed.

Run: python extract_r38_config_variants.py <native source> <descriptor> <output> [--jobs N]. On Windows the build commands
run in WSL and the comparison in this interpreter. The build commands and plan are worked out once; values whose
preprocessed text matches the default or an already-compiled value are not compiled again, and the remaining compiles run
--jobs at a time (default: every processor). Each measured compile is saved in --cache as soon as it is done (only after its compile
succeeded and its measurement validated, written atomically), so a run that stops is continued, not repeated. Every compile
behind an option that is new or differs from --reference is recompiled independently and must agree exactly; a disagreeing
entry is quarantined and the run fails without writing its output. Reads the source; writes the output file, cache entries and a temporary directory it
removes.
"""
import argparse
import array
import bisect
import concurrent.futures
import hashlib
import importlib.util
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

# Data headers that hold fixed-option reads, and the translation unit that defines them
DATA_UNITS = {
    'src/data/moves_info.h': 'src/move.c',
    'src/data/items.h': 'src/item.c',
    'src/data/abilities.h': 'src/pokemon.c',
    'src/data/battle_environment.h': 'src/battle_bg.c',
    'src/data/pokemon/form_change_tables.h': 'src/pokemon.c',
    'include/constants/items.h': 'src/item.c',  # CONFUSE_BERRY_HEAL_FRACTION, used only by src/data/items.h
    'src/data/pokemon/item_effects.h': 'src/item.c',  # the gItemEffect_* arrays, reached only through gItemsInfo[].effect
    'src/data/battle_pool_rules.h': 'src/trainer_pools.c',  # defaultPoolRules and gPoolRulesetsList
    'src/data/contest_moves.h': 'src/contest_effect.c',  # gContestEffects
    'src/data/pokemon/admg_gba_style_aliases.h': 'src/pokemon.c',  # GBA-style graphics names under their options
    'src/data/pokemon/species_info.h': 'src/pokemon.c',
    'src/battle_main.c': 'src/battle_main.c',  # gTrainerClasses is defined in the unit itself
    'src/data/types_info.h': 'src/battle_main.c',  # gTypeEffectivenessTable (B_UPDATED_TYPE_MATCHUPS), a square table of values
    **{f'src/data/pokemon/species_info/gen_{n}_families.h': 'src/pokemon.c' for n in range(1, 10)},
}
# include/config options the game reads only in data tables (no GetConfig or selector read): selecting one writes its data
DATA_OPTIONS = ['I_GEM_BOOST_POWER', 'I_TYPE_BOOST_POWER', 'B_SKY_BATTLE_STRICT_ELIGIBILITY',
                # read only in item, move and species data (src/data/items.h, moves_info.h, species_info, form_change_tables.h)
                'B_X_ITEMS_CROSSUSE', 'C_UPDATED_MOVE_CATEGORIES', 'I_BERRY_PRICE', 'I_GRISEOUS_ORB_FORM_CHANGE', 'I_KEY_FOSSILS',
                'I_LAX_INCENSE_BOOST', 'I_PRICE', 'P_UPDATED_ABILITIES', 'P_UPDATED_EGG_GROUPS', 'P_UPDATED_EVS',
                'P_UPDATED_EXP_YIELDS', 'P_UPDATED_STATS', 'P_UPDATED_TYPES',
                # mixed backlog batch 2: item data and Castform/Cherrim follower form tables
                'I_POWER_ITEM_BOOST', 'I_USE_EVO_HELD_ITEMS_FROM_BAG', 'OW_FOLLOWERS_WEATHER_FORMS',
                # data batch 1: healing amounts (items.h, item_effects.h) and species friendship and perfect IVs
                'I_HEALTH_RECOVERY', 'I_SITRUS_BERRY_HEAL', 'P_UPDATED_FRIENDSHIP', 'P_LEGENDARY_PERFECT_IVS',
                # data batch 2: trainer pool rules (battle_pool_rules.h) and contest effects (contest_moves.h, moves_info.h)
                'B_POOL_RULE_SPECIES_CLAUSE', 'B_POOL_RULE_EXCLUDE_FORMS', 'B_POOL_RULE_ITEM_CLAUSE',
                'B_POOL_RULES_USE_ITEM_EXCLUSIONS', 'B_POOL_RULE_MEGA_STONE_CLAUSE', 'B_POOL_RULE_Z_CRYSTAL_CLAUSE',
                'C_UPDATED_MOVE_EFFECTS',
                # the level-up learnsets each generation's headers define (included by species_info.h; placed per species)
                'P_LVL_UP_LEARNSETS',
                # Arceus's form icons (always built; the species rows choose them)
                'P_ARCEUS_UNIQUE_FORM_ICONS',
                # GBA-style icons and footprints (both sets built; species data choose)
                'P_GBA_STYLE_SPECIES_ICONS', 'P_GBA_STYLE_SPECIES_FOOTPRINTS']
sys.path.insert(0, str(Path(__file__).resolve().parent))
from config_source_value_reads import SOURCE_VALUE_READS  # noqa: E402  (also read by the inventory as extract.SOURCE_VALUE_READS)

GENERATIONS = list(range(0, 10))  # GEN_1 (0) .. GEN_CHAMPIONS (9), include/config/general.h


ON_WINDOWS = os.name == 'nt'


def native(path):
    """A path as the build environment sees it: /mnt/c/... when this runs on Windows and builds through WSL."""
    path = Path(path).resolve()
    return '/mnt/' + path.drive[0].lower() + path.as_posix()[2:] if ON_WINDOWS else str(path)


def run(command, cwd, stdin=None):
    """A build command, in WSL when this runs on Windows (the comparison itself runs in this interpreter)."""
    if ON_WINDOWS:
        line = command if isinstance(command, str) else ' '.join(command)
        command = ['wsl.exe', '--cd', native(cwd), '--exec', 'bash', '-c', line]
    result = subprocess.run(command, cwd=cwd, input=stdin, capture_output=True, shell=not ON_WINDOWS and isinstance(command, str))
    if result.returncode:
        raise SystemExit(f'command failed ({result.returncode}): {command}\n{result.stderr.decode(errors="replace")[-3000:]}')
    return result.stdout


def compile_commands(source, units):
    """The build's own cpp | preproc | cc1 | as pipeline for each unit, as one `make -n` prints them. The dry run walks the
    whole build's dependencies (most of a minute), so it runs once for all units."""
    objects = ['build/emerald/' + unit[:-2] + '.o' for unit in units]
    text = run(['make', '-n'] + [a for unit in units for a in ('-W', unit)] + objects, source).decode()
    commands = {}
    for unit in units:
        commands[unit] = next(l for l in text.splitlines() if l.startswith('arm-none-eabi-cpp') and unit in l)
    return commands


def preprocess(source, command, include_root):
    cpp = command.split(' | ')[0].replace('-iquote include', f'-iquote {native(include_root)}')
    preproc = command.split(' | ')[1]
    return run(f'{cpp} | {preproc}', source)


def assemble(source, command, preprocessed, output):
    stages = command.split(' | ')[2:]
    stages[-1] = re.sub(r' -o \S+ -$', f' -o {native(output)} -', stages[-1])
    run(' | '.join(stages) if ON_WINDOWS else 'bash -c ' + json.dumps(' | '.join(stages)), source, stdin=preprocessed)


class Elf:
    def __init__(self, data):
        self.data = data
        (shoff,) = struct.unpack_from('<I', data, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from('<HHH', data, 0x2E)
        self.sections = []
        for i in range(shnum):
            name, kind, _, _, offset, size, link, info, _, entsize = struct.unpack_from('<10I', data, shoff + i * shentsize)
            self.sections.append(dict(name=name, kind=kind, offset=offset, size=size, link=link, info=info, entsize=entsize))
        names = self.sections[shstrndx]
        for section in self.sections:
            section['name'] = self.string(names, section['name'])
        symtab = next(s for s in self.sections if s['kind'] == 2)
        strings = self.sections[symtab['link']]
        self.symbols = []
        for i in range(symtab['size'] // 16):
            name, value, size, info, _, shndx = struct.unpack_from('<IIIBBH', data, symtab['offset'] + i * 16)
            self.symbols.append(dict(name=self.string(strings, name), value=value, size=size, section=shndx, bind=info >> 4, kind=info & 15))
        self.local_data = {}  # content digest -> (section, start, end): bytes read only when a write needs them
        self._contents = {}   # (section, start, end) -> content digest of local data
        self.relocations = {}  # section index -> {offset: symbol index}
        self.relocated = {}    # section index -> sorted relocation offsets, for range lookups
        for section in self.sections:
            if section['kind'] == 9:  # SHT_REL: (offset, info) pairs of little-endian words, decoded in one call
                assert array.array('I').itemsize == 4
                words = array.array('I', data[section['offset']:section['offset'] + section['size'] // 8 * 8])
                if sys.byteorder != 'little':
                    words.byteswap()
                self.relocations.setdefault(section['info'], {}).update(zip(words[0::2], map((8).__rrshift__, words[1::2])))
        for section, offsets in self.relocations.items():
            self.relocated[section] = sorted(offsets)

    def relocations_in(self, section, start, end):
        """Relocation offsets of a section in [start, end), in order."""
        offsets = self.relocated.get(section, [])
        return offsets[bisect.bisect_left(offsets, start):bisect.bisect_left(offsets, end)]

    def string(self, section, offset):
        start = section['offset'] + offset
        return self.data[start:self.data.index(b'\0', start)].decode()

    def symbol(self, name):
        return next((s for s in self.symbols if s['name'] == name and s['section'] not in (0, 0xFFF1)), None)

    def bytes_at(self, section, offset, size):
        base = self.sections[section]['offset'] + offset
        return self.data[base:base + size]

    def holder(self, section, address):
        """The sized data symbol of a section that contains an address (one pass indexes every section)."""
        if not hasattr(self, '_holders'):
            self._holders = {}
            for i, s in enumerate(self.symbols):
                if s['kind'] == 1 and s['size']:
                    self._holders.setdefault(s['section'], []).append((s['value'], s['size'], i))
            for index in self._holders.values():
                index.sort()
        index = self._holders.get(section, [])
        position = bisect.bisect_right(index, (address, float('inf'), 0)) - 1
        if position >= 0:
            low, size, symbol = index[position]
            if low <= address < low + size:
                return self.symbols[symbol]
        return None

    def pointer(self, section, offset):
        """What a pointer field refers to. Local data (game text, compound literals) is compared by content (content()): the
        range from the pointed-at position to the end of its symbol, and whether that range holds pointers of its own."""
        return self.reference(section, offset, ())

    def content(self, section, start, end, visiting):
        """Digest of local data by content. A relocated word's bytes are a REL addend that changes when data before its target
        moves, so each one is zeroed and described by what it points at, local targets by their own content (recursively;
        a cycle is described by its depth). Otherwise a species' animation table would look changed whenever an earlier
        compound literal was dropped."""
        key = (section, start, end)
        if key in self._contents:
            return self._contents[key]
        if key in visiting:
            return f'cycle:{len(visiting) - visiting.index(key)}'
        data = bytearray(self.bytes_at(section, start, end - start))
        refs = []
        for offset in self.relocations_in(section, start, end):
            size = min(4, end - offset)
            data[offset - start:offset - start + size] = bytes(size)
            refs.append((offset - start, self.reference(section, offset, visiting + (key,))))
        digest = hashlib.sha256(bytes(data) + repr(refs).encode()).hexdigest()
        if 'cycle:' not in repr(refs):
            self._contents[key] = digest
        return digest

    def reference(self, section, offset, visiting):
        index = self.relocations.get(section, {}).get(offset)
        if index is None:
            return ('null', struct.unpack_from('<I', self.bytes_at(section, offset, 4))[0])
        target = self.symbols[index]
        addend = struct.unpack_from('<I', self.bytes_at(section, offset, 4))[0]
        if target['section'] in (0, 0xFFF1):
            return ('symbol', target['name'], addend)
        if target['kind'] == 3:  # a section symbol: the data symbol that holds the address
            holder = self.holder(target['section'], addend)
            if holder is None:
                return ('symbol', self.sections[target['section']]['name'], addend)
            target, addend = holder, addend - holder['value']
        if target['kind'] == 2:  # a function: code is not measured data; a pointer to one is the function it names
            return ('symbol', target['name'], addend)
        if target['bind'] != 0 or not target['size']:
            return ('symbol', target['name'], addend)
        start, end = target['value'] + addend, target['value'] + target['size']
        relocated = bool(self.relocations_in(target['section'], start, end))
        digest = self.content(target['section'], start, end, visiting)
        self.local_data[digest] = (target['section'], start, end)
        return ('local', digest, relocated)


def leaves(layout, name):
    """Writable leaf fields of a struct; aggregate entries are covered by their members."""
    return [f for f in layout[name]['fields'] if not f.get('aggregate')]


def read_field(elf, section, base, field):
    if field['kind'] == 'pointer':
        return elf.pointer(section, base + field['offset'])
    start = base + field['offset']
    offsets = elf.relocations_in(section, start, start + field['size'])
    if offsets:
        # A field the descriptor does not break down (a species' overworldData struct) can hold pointers: their bytes are
        # REL addends that move with earlier data, so they are zeroed and each is described by what it points at. Such a
        # field compares equal when only addends moved; a real change cannot be written as a value (measure reports it).
        data = bytearray(elf.bytes_at(section, start, field['size']))
        for offset in offsets:
            size = min(4, start + field['size'] - offset)
            data[offset - start:offset - start + size] = bytes(size)
        return ('holds-pointers', int.from_bytes(data, 'little'),
                tuple((offset - start, elf.pointer(section, offset)) for offset in offsets))
    raw = int.from_bytes(elf.bytes_at(section, start, field['size']), 'little')
    if 'bitfield' in field:
        raw = (raw >> field['bitfield']['shift']) & ((1 << field['bitfield']['bits']) - 1)
    return raw


# Square tables of plain values the descriptor measures (element type, size in bytes); a cell is the field "[column]"
VALUE_ELEMENTS = {'u8': 1, 'u16': 2, 'u32': 4, 's8': 1, 's16': 2, 's32': 4}


def value_matrix(table):
    """The element size of a two-dimensional table of plain values, or None."""
    dims = table.get('dims', [])
    return VALUE_ELEMENTS.get(table.get('element')) if len(dims) == 2 else None


def tables(elf, descriptor):
    """Every descriptor-measured struct table this object defines, decoded row by row, and every square table of values
    (the type chart), decoded as rows of "[column]" cells."""
    found = {}
    for symbol, table in descriptor['tables'].items():
        element = table.get('element')
        sym = elf.symbol(symbol)
        if sym is not None and (size := value_matrix(table)):
            rows, columns = table['dims']
            data = elf.bytes_at(sym['section'], sym['value'], rows * columns * size)
            found[symbol] = [{f'[{c}]': int.from_bytes(data[(r * columns + c) * size:(r * columns + c + 1) * size], 'little')
                              for c in range(columns)} for r in range(rows)]
            continue
        if sym is None or element not in descriptor['structs']:
            continue
        stride = descriptor['structs'][element]['size']
        fields = leaves(descriptor['structs'], element)
        rows = []
        for row in range(sym['size'] // stride):
            base = sym['value'] + row * stride
            row_values = {}
            for f in fields:
                row_values[f['name']] = read_field(elf, sym['section'], base, f)
            rows.append(row_values)
        found[symbol] = rows
    return found


def unmeasured(elf):
    """Hashes of every data symbol, so a change outside the measured tables is detected. A pointer's bytes in an object are
    a relocation's addend, not its target, so each relocation inside the symbol is hashed by what it points at (as measured
    pointer fields are compared): otherwise a table whose pointers alone change would look unchanged."""
    hashes = {}
    for s in elf.symbols:
        if not s['size'] or s['section'] >= len(elf.sections) or s['section'] == 0:
            continue
        if not elf.sections[s['section']]['name'].startswith(('.rodata', '.data')):
            continue
        # Anonymous text and compound literals are reached through measured pointer fields and compared by content there
        if s['name'].startswith(('sCompoundString_', '__compound_literal.')):
            continue
        start, end = s['value'], s['value'] + s['size']
        offsets = elf.relocations_in(s['section'], start, end)
        # A REL relocation keeps its addend in the data: when data before its target moves (a longer form-change table),
        # the stored bytes change though the pointer still reaches the same thing. Relocated words are hashed by what they
        # resolve to, not by their bytes.
        data = bytearray(elf.bytes_at(s['section'], start, s['size']))
        for offset in offsets:
            data[offset - start:min(offset - start + 4, len(data))] = bytes(min(4, len(data) - (offset - start)))
        digest = hashlib.sha256(data)
        for offset in offsets:
            digest.update(repr((offset - start, elf.pointer(s['section'], offset))).encode())
        hashes[s['name']] = digest.hexdigest()
    return hashes


def plan(source):
    """The options to measure, which data units read each one, where each is defined, and reads outside measured data.

    They are the GetConfig options also read in data (admg_config_catalog.h), DATA_OPTIONS, and the selector options
    (admg_config_extra.h) whose source value (ADMG_DEFAULT_<name>) a data table reads."""
    fixed = re.findall(r'ADMG_CONFIG_FIXED_(\w+) =', (source/'include/constants/admg_config_catalog.h').read_text())
    data_text = {header: (source/header).read_text(encoding='utf-8') for header in DATA_UNITS}
    extra = re.findall(r'F\((\w+),', (source/'include/constants/admg_config_extra.h').read_text(encoding='utf-8'))
    # A source-value read listed in SOURCE_VALUE_READS (data kept built, declarations kept) is not data a selection rewrites
    extra_data = [name for name in extra if any('ADMG_DEFAULT_' + name in text and (header, 'ADMG_DEFAULT_' + name) not in SOURCE_VALUE_READS
                                              for header, text in data_text.items())]
    configs = {}
    for header in sorted((source/'include/config').glob('*.h')):
        for number, line in enumerate(header.read_text(encoding='utf-8').split('\n')):
            match = re.match(r'#define\s+(\w+)\s+(\S+)', line)
            if match:
                configs[match.group(1)] = (header.relative_to(source).as_posix(), number, match.group(2))
    macros = {}
    for name in fixed:
        macros[name] = 'P_' + name if name not in configs and 'P_' + name in configs else name
    for name in DATA_OPTIONS:
        macros[name] = name
    for name in extra_data:
        macros[name] = 'ADMG_DEFAULT_' + name
    fixed = fixed + DATA_OPTIONS + extra_data
    # The audit's read finder: a name counts wherever it is not the argument of a GetConfig(...) selector call
    spec = importlib.util.spec_from_file_location('audit', Path(__file__).with_name('audit_r38_config_reads.py'))
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    units = {}
    for header, unit in DATA_UNITS.items():
        text = data_text[header]
        for name in fixed:
            if audit.find_reads(text, [macros[name]]) and unit not in units.get(name, []):
                units.setdefault(name, []).append(unit)
    # Every read outside GetConfig must be in a measured data unit; any other (assembly, a header constant, an #if in
    # code) keeps the compiled value there, so the option stays fixed whatever its tables do.
    elsewhere = {}
    for tag, rel, number, kind in audit.direct_reads(source, audit.options(source)):
        # A C code read keeps its compiled value whatever the data measures, even in a unit that also defines a table
        if kind == 'code' or rel not in DATA_UNITS:
            elsewhere.setdefault(tag, []).append(f'{rel}:{number}')
    # The other options' measured macro must not be read anywhere else either
    others = {macros[name]: name for name in DATA_OPTIONS + extra_data}
    pattern = re.compile(r'\b(' + '|'.join(map(re.escape, others)) + r')\b')
    for path in sorted(p for root in ('src', 'include', 'data') for p in (source/root).rglob('*') if p.suffix in ('.c', '.h', '.s', '.inc')):
        rel = path.relative_to(source).as_posix()
        if rel in DATA_UNITS or rel.startswith('include/config/') or rel.endswith('admg_config_extra.h'):
            continue
        text = path.read_text(encoding='utf-8', errors='replace')
        if path.suffix in ('.c', '.h'):  # a name in a comment is not a read; keep the line numbers
            text = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), text, flags=re.S)
            text = re.sub(r'//[^\n]*', '', text)
        for number, line in enumerate(text.split('\n'), 1):
            for macro in pattern.findall(line):
                if (rel, macro) not in SOURCE_VALUE_READS:
                    elsewhere.setdefault(others[macro], []).append(f'{rel}:{number}')
    return fixed, configs, macros, units, elsewhere


def text_digest(text, include_root):
    """Preprocessed text identity without the job's private include path, which cpp writes into its line markers."""
    return hashlib.sha256(text.replace(native(include_root).encode(), b'include')).hexdigest()


def unit_dependencies(source):
    """The source files the data units' compiles read: each unit and every include/ and src/ file its build dependency file
    (build/emerald/<unit>.d, written by the build's scaninc) lists. Preparation runs on a completed build, so they exist."""
    files = set()
    for unit in sorted(set(DATA_UNITS.values())):
        depfile = source/'build/emerald'/(unit[:-2] + '.d')
        if not depfile.exists():
            raise SystemExit(f'{depfile} is missing: build the native source first')
        files.add(unit)
        files.update(word for word in depfile.read_text(encoding='utf-8').replace('\\\n', ' ').split()
                     if word.startswith(('include/', 'src/')) and not word.endswith(':'))
    return sorted(files)


def layouts_digest(descriptor):
    return hashlib.sha256(json.dumps({'tables': descriptor['tables'], 'structs': descriptor['structs']}, sort_keys=True).encode()).hexdigest()


def input_digest(source, descriptor):
    """What a measurement depends on: the files the data units compile (their dependency lists, which include every
    include/config header they read), this tool and the read finder it plans with, and the descriptor's layouts.
    Preparation recomputes it and refuses a saved measurement whose inputs have changed. Headers no data unit includes, and
    config_source_value_reads.py, are not inputs: a new compiled read of a measured option elsewhere fails the inventory gate."""
    digest = hashlib.sha256()
    for rel in unit_dependencies(source):
        digest.update(rel.encode() + b'\0' + (source/rel).read_bytes() + b'\0')
    for tool in (Path(__file__), Path(__file__).with_name('audit_r38_config_reads.py')):
        digest.update(tool.read_bytes())
    digest.update(layouts_digest(descriptor).encode())
    return digest.hexdigest()


def object_elf(path):
    """A compiled object, refused unless the compile left a complete ELF file (a failed or interrupted compile is not measured)."""
    data = Path(path).read_bytes() if Path(path).exists() else b''
    if len(data) < 0x34 or data[:4] != b'\x7fELF':
        raise RuntimeError(f'{path}: the compile did not produce an ELF object')
    return Elf(data)


def validate_measurement(result, descriptor):
    """Raise ValueError unless a measured compile is well formed for this descriptor: a list of writes and a list of problem
    strings, each write naming a measured table, a row inside it and one of its leaf fields, and carrying exactly one of a
    value that fits the field, placed data, a symbol with its addend, or a constant pointer. A result that fails is never
    cached or used."""
    if not isinstance(result, (list, tuple)) or len(result) != 2:
        raise ValueError('a measured compile is (writes, problems)')
    writes, problems = result
    if not isinstance(writes, list) or not isinstance(problems, list) or not all(isinstance(p, str) for p in problems):
        raise ValueError('writes and problems must be lists; problems are strings')
    for write in writes:
        if not isinstance(write, dict):
            raise ValueError(f'write is not an object: {write!r}')
        table = descriptor['tables'].get(write.get('table'))
        if table is not None and (size := value_matrix(table)):
            column = re.fullmatch(r'\[(\d+)\]', str(write.get('field')))
            row = write.get('row')
            if not column or int(column.group(1)) >= table['dims'][1] or not isinstance(row, int) or not 0 <= row < table['dims'][0]:
                raise ValueError(f'write to a cell the square table does not have: {write!r}')
            if set(write) != {'table', 'row', 'field', 'value'} or not isinstance(write['value'], int) or not 0 <= write['value'] < 1 << (8 * size):
                raise ValueError(f'a square-table write carries one value that fits its element: {write!r}')
            continue
        if table is None or table.get('element') not in descriptor['structs']:
            raise ValueError(f'write to a table the descriptor does not measure: {write!r}')
        fields = {f['name']: f for f in leaves(descriptor['structs'], table['element'])}
        field = fields.get(write.get('field'))
        row = write.get('row')
        rows = table.get('dims', [None])[0]
        if field is None or not isinstance(row, int) or row < 0 or (rows is not None and row >= rows):
            raise ValueError(f'write to a row or field the table does not have: {write!r}')
        kinds = [k for k in ('value', 'data', 'symbol', 'pointer') if k in write]
        if len(kinds) != 1:
            raise ValueError(f'write must carry exactly one of value, data, symbol or pointer: {write!r}')
        if 'value' in write:
            bits = field['bitfield']['bits'] if 'bitfield' in field else 8 * field['size']
            if not isinstance(write['value'], int) or not 0 <= write['value'] < (1 << bits):
                raise ValueError(f'value does not fit {table["element"]}.{field["name"]}: {write!r}')
        elif 'data' in write:
            if not isinstance(write['data'], str) or len(write['data']) % 2:
                raise ValueError(f'placed data is not hex bytes: {write!r}')
            bytes.fromhex(write['data'])
        elif 'symbol' in write:
            if not isinstance(write['symbol'], str) or not isinstance(write.get('addend'), int):
                raise ValueError(f'symbol write needs a name and an addend: {write!r}')
        elif not isinstance(write['pointer'], int) or not 0 <= write['pointer'] < 1 << 32:
            raise ValueError(f'constant pointer out of range: {write!r}')


class MeasurementCache:
    """Measured compiles on disk, one file each, keyed by what produced them (the unit, the default and variant preprocessed
    texts, the descriptor's layouts and the measuring code). An entry is published only after its compile succeeded and its
    measurement validated, and only whole: it is written to a private temporary file, flushed to disk and renamed into place,
    so a failed or interrupted compile leaves nothing behind. An entry that fails to load or that an independent recompile
    contradicts is moved to the quarantine folder with the reason, never overwritten in place."""

    def __init__(self, folder, quarantine, context, descriptor):
        self.folder, self.quarantine_folder, self.context, self.descriptor = Path(folder), Path(quarantine), context, descriptor
        self.folder.mkdir(parents=True, exist_ok=True)

    def key(self, unit, base_digest, digest):
        return hashlib.sha256('\0'.join((self.context, unit, base_digest, digest)).encode()).hexdigest()

    def path(self, key):
        return self.folder/f'{key}.json'

    def load(self, unit, base_digest, digest):
        key = self.key(unit, base_digest, digest)
        path = self.path(key)
        if not path.exists():
            return None
        try:
            entry = json.loads(path.read_text(encoding='utf-8'))
            if entry.get('key') != key or entry.get('unit') != unit:
                raise ValueError('entry does not match its key')
            result = (entry['writes'], entry['problems'])
            validate_measurement(result, self.descriptor)
        except (ValueError, KeyError, TypeError) as error:
            self.quarantine(key, f'failed to load: {error}')
            raise SystemExit(f'{path}: invalid cache entry quarantined ({error})')
        return result

    def save(self, unit, base_digest, digest, result):
        validate_measurement(result, self.descriptor)  # an invalid measurement is never published
        key = self.key(unit, base_digest, digest)
        temporary = self.folder/f'{key}.{os.getpid()}.{threading.get_ident()}.tmp'
        try:
            with temporary.open('w', encoding='utf-8') as stream:
                json.dump({'key': key, 'unit': unit, 'writes': result[0], 'problems': result[1]}, stream)
                stream.flush()
                os.fsync(stream.fileno())
            os.replace(temporary, self.path(key))
        finally:
            if temporary.exists():
                temporary.unlink()

    def quarantine(self, key, reason):
        """Move an entry out of the cache with the reason beside it; a later run recompiles that text."""
        self.quarantine_folder.mkdir(parents=True, exist_ok=True)
        path = self.path(key)
        if path.exists():
            os.replace(path, self.quarantine_folder/path.name)
        (self.quarantine_folder/f'{key}.reason.txt').write_text(reason + '\n', encoding='utf-8')


def options_to_verify(options, unsupported, reference):
    """The options whose measurement is new or differs from the committed one (reference: a config-variants.json); their
    compiles are recompiled independently before the result is accepted."""
    if reference is None:
        return sorted(set(options) | set(unsupported))
    names = []
    for name in sorted(set(options) | set(unsupported)):
        mine = ('options', options[name]) if name in options else ('unsupported', unsupported[name])
        theirs = (('options', reference['options'][name]) if name in reference.get('options', {})
                  else ('unsupported', reference['unsupported'][name]) if name in reference.get('unsupported', {}) else None)
        if mine != theirs:
            names.append(name)
    return names


def verify_compiles(keys, used, recompute, cache, normalize=lambda r: json.loads(json.dumps(r))):
    """Recompile each (unit, base digest, text digest) in keys independently (recompute bypasses the cache) and compare it
    with the result the run used. A mismatch is quarantined; the list of mismatches is returned and the caller fails."""
    mismatches = []
    for key in keys:
        fresh = recompute(key)
        validate_measurement(fresh, cache.descriptor)
        if normalize(fresh) != normalize(used[key]):
            cache.quarantine(cache.key(*key), 'an independent recompile disagreed with this entry')
            mismatches.append(key)
    return mismatches


def measuring_code_digest():
    """The code that turns one compiled object into writes; a cached result is reused only while it is unchanged."""
    import inspect
    parts = [Elf, leaves, read_field, value_matrix, tables, unmeasured, pointer_slots, references, global_symbol, measure]
    return hashlib.sha256('\n'.join(inspect.getsource(part) for part in parts).encode()).hexdigest()


def pointer_slots(elf, descriptor):
    """(section, offset) of every pointer field in the descriptor-measured tables this object defines."""
    slots = set()
    for symbol, table in descriptor['tables'].items():
        sym, element = elf.symbol(symbol), table.get('element')
        if sym is None or element not in descriptor['structs']:
            continue
        stride = descriptor['structs'][element]['size']
        offsets = [f['offset'] for f in leaves(descriptor['structs'], element) if f['kind'] == 'pointer']
        for row in range(sym['size'] // stride):
            slots.update((sym['section'], sym['value'] + row * stride + o) for o in offsets)
    return slots


def references(elf, name):
    """(section, offset) of every relocation in the object that points into the local data symbol `name`."""
    sym = next((s for s in elf.symbols if s['name'] == name and s['size'] and s['bind'] == 0), None)
    if sym is None:
        return None
    found = []
    for section, relocations in elf.relocations.items():
        if not elf.sections[section]['name'].startswith(('.text', '.rodata', '.data')):
            continue  # debug information and other sections the game never loads
        for offset, index in relocations.items():
            target = elf.symbols[index]
            if target['name'] == name and target['section'] == sym['section']:
                found.append((section, offset))
            elif target['kind'] == 3 and target['section'] == sym['section']:  # section symbol + addend
                addend = struct.unpack_from('<I', elf.bytes_at(section, offset, 4))[0]
                if sym['value'] <= addend < sym['value'] + sym['size']:
                    found.append((section, offset))
    return found


def global_symbol(elf, name):
    """Whether a pointer target is a global the linked game defines once (here or in another unit), so the SDK can resolve
    it by name in the game's symbols."""
    return any(s['name'] == name and (s['bind'] != 0 or s['section'] == 0) for s in elf.symbols)


def measure(elf, descriptor, base):
    """Field writes and problems of one variant object against the default object of the same unit."""
    variant_tables, variant_symbols = tables(elf, descriptor), unmeasured(elf)
    writes, problems = [], []
    slots = None
    for symbol in sorted(set(base['symbols']) | set(variant_symbols)):
        if symbol not in base['tables'] and base['symbols'].get(symbol) != variant_symbols.get(symbol):
            # Local data reached only through measured pointer fields (a species' form-change table) is compared, and
            # written as placed data, through those fields; data referenced any other way (code, another table) stays fixed
            slots = slots if slots is not None else pointer_slots(elf, descriptor)
            found = references(elf, symbol)
            if not found or any(reference not in slots for reference in found):
                problems.append(f'unmeasured data {symbol} changes')
    for symbol, rows in variant_tables.items():
        for row in range(min(len(rows), len(base['tables'][symbol]))):
            old, new = base['tables'][symbol][row], rows[row]
            for field, now in new.items():
                if old[field] == now:
                    continue
                write = {'table': symbol, 'row': row, 'field': field}
                if isinstance(now, tuple) and now[0] == 'local' and not now[2]:
                    section, start, end = elf.local_data[now[1]]
                    writes.append(write | {'data': elf.bytes_at(section, start, end - start).hex()})
                elif isinstance(now, tuple) and now[0] == 'symbol' and global_symbol(elf, now[1]):
                    writes.append(write | {'symbol': now[1], 'addend': now[2]})  # data the game already has
                elif isinstance(now, tuple) and now[0] == 'null':
                    writes.append(write | {'pointer': now[1]})  # NULL (or another constant address)
                elif isinstance(now, tuple) and now[0] == 'holds-pointers':
                    problems.append(f'{symbol}[{row}].{field} (a field holding pointers) changes')
                elif isinstance(now, tuple):
                    problems.append(f'{symbol}[{row}].{field} points elsewhere')
                else:
                    writes.append(write | {'value': now})
    return writes, problems


def main():
    """Measure every fixed option in one job. The build commands and the plan are worked out once; each option value is
    preprocessed (cheap), and only preprocessed texts not seen before are compiled and compared, several at a time."""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('source', type=Path)
    parser.add_argument('descriptor', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--jobs', type=int, default=os.cpu_count(), help='compiles at once (default: every processor)')
    parser.add_argument('--reference', type=Path, default=None,
                        help='the previous measurement (the last build\'s config-variants.json): options new or different from it '
                             'are recompiled independently; without one, every option is verified')
    parser.add_argument('--cache', type=Path, default=Path('build/config-variants-cache'),
                        help='measured compiles, one file each, keyed by what was compiled (default: build/config-variants-cache)')
    args = parser.parse_args()
    source = args.source.resolve()
    descriptor = json.loads(args.descriptor.read_text(encoding='utf-8'))
    started = time.monotonic()

    fixed, configs, macros, units, elsewhere = plan(source)
    all_units = sorted({u for us in units.values() for u in us})
    commands = compile_commands(source, all_units)
    print(f'planned {len(units)} options over {len(all_units)} units in {time.monotonic() - started:.0f} s', flush=True)

    work = Path(tempfile.mkdtemp(prefix='admg-config-variants-'))
    lock = threading.Lock()
    # Each measured compile is saved as soon as it is done, under a key naming exactly what produced it: the unit, the
    # default and variant preprocessed texts, the descriptor's layouts and the measuring code. A run that stops (a crash is
    # still a failed run and is reported) leaves its finished compiles there, and the next run compiles only the rest.
    context = hashlib.sha256((layouts_digest(descriptor) + measuring_code_digest()).encode()).hexdigest()
    cache = MeasurementCache(args.cache, args.cache.parent/'config-variants-quarantine', context, descriptor)
    cached, compiled = [0], [0]
    producers = {}  # (unit, text digest) -> the (option, value) that first produced it, to recompile it independently
    measured = {}  # (unit, text digest) -> (writes, problems); filled by whichever job compiles that text first
    used = {}      # (option, value, unit) -> (unit, base digest, text digest) of a compiled (non-default) text
    pending = {}   # (unit, text digest) -> Event set once measured
    baselines = {}

    def include_with(job, macro=None, value=None):
        root = work/f'{job}/include'
        shutil.copytree(source/'include', root)
        if macro is not None:
            path, line_number, _ = configs[macro]
            config_file = root/path[len('include/'):]
            lines = config_file.read_text(encoding='utf-8').split('\n')
            lines[line_number] = f'#define {macro} {value}'
            config_file.write_text('\n'.join(lines), encoding='utf-8')
        return root

    def baseline(unit):
        root = include_with('base-' + unit.replace('/', '_'))
        text = preprocess(source, commands[unit], root)
        return unit, {'digest': text_digest(text, root), 'root': root, 'text': text, 'loaded': None, 'lock': threading.Lock()}

    def loaded_baseline(unit):
        """The default object's tables and data hashes, compiled the first time an uncached variant needs them."""
        base = baselines[unit]
        with base['lock']:
            if base['loaded'] is None:
                obj = base['root'].parent/'base.o'
                assemble(source, commands[unit], base['text'], obj)
                elf = object_elf(obj)
                base['loaded'] = {'tables': tables(elf, descriptor), 'symbols': unmeasured(elf)}
            return base['loaded']

    def variant(name, value):
        job = f'{name}-{value}'
        root = include_with(job, macros[name], value)
        results = {}
        try:
            for unit in units[name]:
                text = preprocess(source, commands[unit], root)
                key = (unit, text_digest(text, root))
                if key[1] == baselines[unit]['digest']:
                    results[unit] = ([], [])  # the default text: nothing changes, nothing compiled
                    continue
                with lock:
                    owner = key not in pending
                    if owner:
                        pending[key] = threading.Event()
                        producers[key] = (name, value)
                if owner:
                    try:
                        result = cache.load(unit, baselines[unit]['digest'], key[1])
                        if result is not None:
                            with lock:
                                cached[0] += 1
                        else:
                            obj = root.parent/f'{unit.replace("/", "_")}.o'
                            assemble(source, commands[unit], text, obj)
                            result = measure(object_elf(obj), descriptor, loaded_baseline(unit))
                            cache.save(unit, baselines[unit]['digest'], key[1], result)  # validates, then publishes whole
                            with lock:
                                compiled[0] += 1
                        measured[key] = result
                    except BaseException as error:
                        measured[key] = error  # jobs waiting on the same text fail too instead of waiting forever
                        raise
                    finally:
                        pending[key].set()
                else:
                    pending[key].wait()
                if isinstance(measured[key], BaseException):
                    raise RuntimeError(f'{unit} with {name}={value}: the shared compile failed') from measured[key]
                results[unit] = measured[key]
                used[(name, value, unit)] = (unit, baselines[unit]['digest'], key[1])
        finally:
            shutil.rmtree(root.parent, ignore_errors=True)
        return name, value, results

    try:
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
            for unit, base in pool.map(baseline, all_units):
                baselines[unit] = base
            jobs = []
            for name in sorted(units):
                default = configs[macros[name]][2]
                for value in (GENERATIONS if default.startswith('GEN_') else [0, 1]):
                    jobs.append(pool.submit(variant, name, value))
            values = {}
            for done, job in enumerate(concurrent.futures.as_completed(jobs), 1):
                name, value, results = job.result()
                values.setdefault(name, {})[value] = results
                if done % 50 == 0 or done == len(jobs):
                    print(f'{done}/{len(jobs)} values measured, {len(measured)} distinct texts ({compiled[0]} compiled, '
                          f'{cached[0]} from the cache), {time.monotonic() - started:.0f} s', flush=True)

            result = {'formatVersion': 1, 'inputDigest': input_digest(source, descriptor), 'generations': GENERATIONS,
                      'options': {}, 'unsupported': {}, 'notInDataTables': sorted(set(fixed) - set(units))}
            for name in sorted(units):
                option = {'macro': macros[name], 'default': configs[macros[name]][2], 'values': {}}
                problems = []
                for value in sorted(values[name]):
                    option['values'][str(value)] = [w for unit in units[name] for w in values[name][value][unit][0]]
                    problems += [f'{value}: {p}' for unit in units[name] for p in values[name][value][unit][1]]
                problems += [f'also read at {where}' for where in elsewhere.get(name, [])[:10]]
                if problems:
                    result['unsupported'][name] = sorted(set(problems))[:40]
                else:
                    result['options'][name] = option

            # Independent verification: every compile behind an option that is new or differs from the committed
            # measurement is recompiled from scratch (a fresh include tree, a fresh default object, no cache) and must agree
            # exactly with the result this run used, whether that came from the cache or from this run's own compile.
            reference = json.loads(args.reference.read_text(encoding='utf-8')) if args.reference.exists() else None
            names = options_to_verify(result['options'], result['unsupported'], reference)
            keys = sorted({used[(name, value, unit)] for (name, value, unit) in used if name in names})
            by_key = {k: measured[k[0], k[2]] for k in keys}
            verify_bases = {}
            verify_lock = threading.Lock()

            def verify_base(unit):
                with verify_lock:
                    if unit not in verify_bases:
                        root = include_with('verify-base-' + unit.replace('/', '_'))
                        text = preprocess(source, commands[unit], root)
                        if text_digest(text, root) != baselines[unit]['digest']:
                            raise SystemExit(f'{unit}: the default text changed during the run')
                        obj = root.parent/'base.o'
                        assemble(source, commands[unit], text, obj)
                        elf = object_elf(obj)
                        verify_bases[unit] = {'tables': tables(elf, descriptor), 'symbols': unmeasured(elf)}
                    return verify_bases[unit]

            def recompute(key):
                unit, _, digest = key
                name, value = producers[unit, digest]
                root = include_with(f'verify-{name}-{value}-{unit.replace("/", "_")}', macros[name], value)
                text = preprocess(source, commands[unit], root)
                if text_digest(text, root) != digest:
                    raise SystemExit(f'{unit} with {name}={value}: the preprocessed text changed during the run')
                obj = root.parent/'verify.o'
                assemble(source, commands[unit], text, obj)
                return measure(object_elf(obj), descriptor, verify_base(unit))

            if keys:
                print(f'verifying {len(keys)} compiles behind {len(names)} new or changed options', flush=True)
                fresh = dict(zip(keys, pool.map(recompute, keys)))
                mismatches = verify_compiles(keys, by_key, fresh.__getitem__, cache)
                if mismatches:
                    raise SystemExit(f'{len(mismatches)} compiles disagreed with an independent recompile and were quarantined '
                                     f'in {cache.quarantine_folder}: ' + ', '.join(f'{u} {producers[u, d]}' for u, _, d in mismatches[:10]))
                print(f'verified {len(keys)} compiles: all agree', flush=True)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    args.output.write_text(json.dumps(result, indent=1) + '\n', encoding='utf-8')
    print(f'{len(result["options"])} options measured, {len(result["unsupported"])} unsupported, '
          f'{len(result["notInDataTables"])} not in data tables; {len(measured)} distinct texts ({compiled[0]} compiled, '
          f'{cached[0]} from the cache); '
          f'{time.monotonic() - started:.0f} s')


if __name__ == '__main__':
    main()
