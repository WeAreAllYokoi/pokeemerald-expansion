#!/usr/bin/env python3
"""Find Expansion configuration options (the GetConfig table) that are still read outside GetConfig.

An option read only through GetConfig follows the run-time selection (patch 0045). An option also read directly in a data
table, a header constant, a preprocessor condition or battle-script assembly keeps its compiled value there, so selecting
it would half-apply the rule. Those are "fixed" options.

The tool writes include/constants/admg_config_catalog.h, which the descriptor measures so the SDK can accept options by
the names romhack authors edit in include/config/*.h:
  ADMG_ConfigFixed           options the SDK refuses because compiled data also reads them
  ADMG_ConfigSourcePrefixed  options whose source name carries the P_ prefix (P_BALL_INHERITANCE for CONFIG_BALL_INHERITANCE)

Usage: audit_r38_config_reads.py <native source> [--emit | --check]
  (default) report; --emit writes the header; --check fails when the header is out of date or code reads remain.
Reads the source only (except --emit, which writes that one header).
"""
import argparse
import functools
import re
import sys
from pathlib import Path

HEADER = 'include/constants/admg_config_catalog.h'
SKIP = {'include/constants/config_changes.h', 'include/config_changes.h', 'src/config_changes.c', HEADER}


def options(source):
    text = (source/'include/constants/config_changes.h').read_text(encoding='utf-8')
    found = []
    for section, prefix in [('BATTLE_CONFIG_DEFINITIONS', ''), ('POKEMON_CONFIG_DEFINITIONS', 'P_'), ('AI_CONFIG_DEFINITIONS', '')]:
        body = text.split('#define ' + section + '(F)')[1].split('\n\n')[0]
        found += [(m.group(1), prefix + m.group(1)) for m in re.finditer(r'F\((\w+),', body)]
    return found


SELECTOR_CALL = re.compile(r'(?<!\w)GetConfig\s*\(\s*$')
# A designated initializer line in a .c file ([INDEX] = ... or .field = ...): table data, not a statement
DESIGNATED_INITIALIZER = re.compile(r'\s*(\[[^\]]+\]\s*=(?!=)|\.[A-Za-z_][\w.]*\s*=(?!=))')


def find_reads(text, names):
    """Each whole-identifier occurrence of one of `names` in `text` that is not the argument of a GetConfig(...) selector
    call, as (name, offset). Everything else reads the compiled value: `if (B_X >= GEN_5)`, `.x = B_X ? 1 : 2`, `#if B_X`,
    `.if B_X` in assembly. CONFIG_B_X and GetConfig_B_X() are other identifiers and never match."""
    if not names:
        return []
    pattern = name_pattern(tuple(sorted(names)))
    return [(m.group(1), m.start()) for m in pattern.finditer(text) if not SELECTOR_CALL.search(text[max(0, m.start() - 64):m.start()])]


@functools.lru_cache(maxsize=64)
def name_pattern(names):
    return re.compile(r'(?<!\w)(' + '|'.join(sorted(map(re.escape, names), key=len, reverse=True)) + r')(?!\w)')


def strip_comment(rel, line):
    """A line without its trailing comment: `@` in assembly, `//` in C (block comments are removed per file first)."""
    return line.split('@')[0] if rel.endswith(('.s', '.inc')) else line.split('//')[0]


def without_block_comments(rel, text):
    """C text with each /* ... */ comment blanked, keeping its newlines so line numbers stay true. A name in a comment is
    documentation, not a read; refusing an option for it would refuse one that works."""
    if rel.endswith(('.s', '.inc')):
        return text
    return re.sub(r'/\*.*?\*/', lambda m: re.sub(r'[^\n]', ' ', m.group()), text, flags=re.S)


def kind_of(rel, line):
    """Where a read keeps its compiled value: assembly, a macro definition (baked into every use), a preprocessor condition,
    a data table, a header, or C code. Anything but C code makes the option fixed; a C code read fails --check."""
    if rel.endswith(('.s', '.inc')):
        return 'assembly'
    if line.strip().startswith('#') and line.strip()[1:].lstrip().startswith('define'):
        return 'macro'
    if line.strip().startswith('#'):
        return 'preprocessor'
    if rel.startswith('src/data/') or DESIGNATED_INITIALIZER.match(line):
        return 'data'
    if rel.startswith('include/'):
        return 'header'
    return 'code'


def direct_reads(source, table):
    macros = {macro: tag for tag, macro in table}
    reads = []
    for base, globs in [('src', ['*.c', '*.h']), ('include', ['*.h']), ('data', ['*.s', '*.inc'])]:
        for glob in globs:
            for path in sorted((source/base).rglob(glob)):
                rel = path.relative_to(source).as_posix()
                # include/test/ is the test framework, compiled only into test builds, never into the game
                if rel.startswith(('include/config/', 'include/test/')) or rel in SKIP:
                    continue
                text = without_block_comments(rel, path.read_text(encoding='utf-8', errors='ignore'))
                for number, line in enumerate(text.split('\n'), 1):
                    code = strip_comment(rel, line)
                    for name, _ in find_reads(code, macros):
                        reads.append((macros[name], rel, number, kind_of(rel, code)))
    return reads


def header(fixed, prefixed):
    def enum(name, entries):
        return ['', f'enum {name}', '{'] + [f'    {entry},' for entry in entries] + ['};']
    lines = ['/* Generated by tools/admg101/audit_r38_config_reads.py',
             ' * ADMG_ConfigSourcePrefixed gives ConfigTag values their P_-prefixed source names',
             ' * ADMG_ConfigFixed lists options still used in data tables, header constants,',
             ' * preprocessor conditions or battle-script assembly',
             ' * Those uses keep their compiled values even when GetConfig changes */',
             '#ifndef GUARD_CONSTANTS_ADMG_CONFIG_CATALOG_H', '#define GUARD_CONSTANTS_ADMG_CONFIG_CATALOG_H',
             '#include "constants/config_changes.h"']
    lines += enum('ADMG_ConfigFixed', [f'ADMG_CONFIG_FIXED_{tag} = CONFIG_{tag}' for tag in fixed])
    lines += enum('ADMG_ConfigSourcePrefixed', [f'ADMG_CONFIG_SOURCE_{macro} = CONFIG_{tag}' for tag, macro in prefixed])
    lines += ['', '#endif', '']
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('source', type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--emit', action='store_true')
    mode.add_argument('--check', action='store_true')
    args = parser.parse_args()
    source = args.source.resolve()
    table = options(source)
    reads = direct_reads(source, table)
    code = [r for r in reads if r[3] == 'code']
    fixed = sorted({tag for tag, _, _, kind in reads if kind != 'code'}, key=[t for t, _ in table].index)
    text = header(fixed, [(tag, macro) for tag, macro in table if macro != tag])
    print(f'{len(table)} options: {len(table) - len(fixed)} read only through GetConfig, {len(fixed)} fixed, '
          f'{len(code)} direct code reads')
    for tag, rel, number, _ in code:
        print(f'  direct code read: {tag} at {rel}:{number}')
    target = source/HEADER
    if args.emit:
        target.write_text(text, encoding='utf-8', newline='\n')
        print('wrote', HEADER)
    elif args.check:
        current = target.read_text(encoding='utf-8') if target.exists() else ''
        if code or current != text:
            print('FAIL: route the direct code reads or regenerate ' + HEADER, file=sys.stderr)
            return 1
        print('PASS')
    return 0


if __name__ == '__main__':
    sys.exit(main())
