#!/usr/bin/env python3
"""Regression cases for the integrity of extract_r38_config_variants.py's measurement cache.

Run 15 (2026-10-23) failed during a compile and, 3 s earlier, had cached one measured result that was well formed but wrong
(a species-types option "changing" __func__ literals); run 15b reused it. These cases check that a result is published only
whole and only after its compile succeeded and its measurement validated, that a damaged entry is quarantined, and that a
cached result an independent recompile contradicts is quarantined and fails the run instead of being accepted or
overwritten. Run: python test_config_variants_cache.py (stdlib unittest, temp folders, no compiles)."""
import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
spec = importlib.util.spec_from_file_location('extract_r38_config_variants', HERE/'extract_r38_config_variants.py')
extract = importlib.util.module_from_spec(spec)
sys.modules['extract_r38_config_variants'] = extract
spec.loader.exec_module(extract)

DESCRIPTOR = {'tables': {'gSpeciesInfo': {'element': 'SpeciesInfo', 'dims': [4]}},
              'structs': {'SpeciesInfo': {'size': 8, 'fields': [
                  {'name': 'types', 'kind': 'array', 'size': 2, 'offset': 0},
                  {'name': 'description', 'kind': 'pointer', 'size': 4, 'offset': 4}]}}}
KEY = ('src/pokemon.c', 'base-digest', 'variant-digest')
GOOD = ([{'table': 'gSpeciesInfo', 'row': 1, 'field': 'types', 'value': 0x0B0A}], [])
# Well formed but wrong, like run 15's entry: problems a species-types option cannot cause
POISONED = ([], ['unmeasured data __func__.0 changes', 'unmeasured data gADMG_ExperienceTableCount changes'])


class CacheIntegrity(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        self.cache = extract.MeasurementCache(root/'cache', root/'quarantine', 'context', DESCRIPTOR)

    def tearDown(self):
        self.tmp.cleanup()

    def files(self, folder='cache'):
        path = Path(self.tmp.name)/folder
        return sorted(p.name for p in path.iterdir()) if path.exists() else []

    def test_a_failed_compilation_publishes_nothing(self):
        # The compile fails before a result exists: the run's code never reaches save, and nothing is left behind
        def compile_and_measure():
            raise RuntimeError('cc1 terminated')
        with self.assertRaises(RuntimeError):
            self.cache.save(*KEY, compile_and_measure())
        self.assertEqual([], self.files())
        # A compile that produced no complete ELF object is refused before it is measured
        broken = Path(self.tmp.name)/'broken.o'
        broken.write_bytes(b'\x7fEL')
        with self.assertRaises(RuntimeError):
            extract.object_elf(broken)
        with self.assertRaises(RuntimeError):
            extract.object_elf(Path(self.tmp.name)/'missing.o')

    def test_an_invalid_measurement_is_never_cached(self):
        for bad in [([{'table': 'gSpeciesInfo', 'row': 9, 'field': 'types', 'value': 1}], []),   # row outside the table
                    ([{'table': 'gSpeciesInfo', 'row': 1, 'field': 'types', 'value': 1 << 16}], []),  # value wider than the field
                    ([{'table': 'gNotMeasured', 'row': 1, 'field': 'types', 'value': 1}], []),
                    ([{'table': 'gSpeciesInfo', 'row': 1, 'field': 'types', 'value': 1, 'pointer': 0}], []),
                    ([{'table': 'gSpeciesInfo', 'row': 1, 'field': 'description', 'data': 'abc'}], []),
                    ([], [42]),
                    'not a measurement']:
            with self.assertRaises(ValueError, msg=repr(bad)):
                self.cache.save(*KEY, bad)
        self.assertEqual([], self.files())

    def test_an_interrupted_write_publishes_nothing(self):
        with mock.patch.object(extract.json, 'dump', side_effect=KeyboardInterrupt):
            with self.assertRaises(KeyboardInterrupt):
                self.cache.save(*KEY, GOOD)
        self.assertEqual([], self.files())  # neither the entry nor its temporary file
        self.cache.save(*KEY, GOOD)
        self.assertEqual(GOOD, tuple(self.cache.load(*KEY)))

    def test_a_damaged_entry_is_quarantined_on_load(self):
        self.cache.save(*KEY, GOOD)
        path = self.cache.path(self.cache.key(*KEY))
        path.write_text('{"key": "', encoding='utf-8')
        with self.assertRaises(SystemExit):
            self.cache.load(*KEY)
        self.assertEqual([], self.files())
        self.assertIn(path.name, self.files('quarantine'))

    def test_a_poisoned_entry_is_caught_quarantined_and_fails_the_run(self):
        # Run 15's pattern: a well-formed wrong result is cached; a later run reuses it for an option that now differs
        # from the committed measurement, so it is recompiled independently, disagrees, and is quarantined
        self.cache.save(*KEY, POISONED)
        used = {KEY: self.cache.load(*KEY)}
        recompiled = []
        mismatches = extract.verify_compiles([KEY], used, lambda key: recompiled.append(key) or GOOD, self.cache)
        self.assertEqual([KEY], mismatches)
        self.assertEqual([KEY], recompiled)
        self.assertEqual([], self.files())                    # not left for the next run
        name = self.cache.path(self.cache.key(*KEY)).name
        self.assertIn(name, self.files('quarantine'))         # kept as evidence, with the reason
        reason = (Path(self.tmp.name)/'quarantine'/name.replace('.json', '.reason.txt')).read_text(encoding='utf-8')
        self.assertIn('independent recompile disagreed', reason)
        quarantined = json.loads((Path(self.tmp.name)/'quarantine'/name).read_text(encoding='utf-8'))
        self.assertEqual(list(POISONED[1]), quarantined['problems'])  # never overwritten with the recompiled result

    def test_agreeing_entries_are_kept(self):
        self.cache.save(*KEY, GOOD)
        used = {KEY: self.cache.load(*KEY)}
        self.assertEqual([], extract.verify_compiles([KEY], used, lambda key: GOOD, self.cache))
        self.assertEqual([self.cache.path(self.cache.key(*KEY)).name], self.files())
        self.assertEqual([], self.files('quarantine'))

    def test_only_new_or_changed_options_are_verified(self):
        reference = {'options': {'SAME': {'values': {'1': []}}, 'CHANGED': {'values': {'1': []}}},
                     'unsupported': {'STILL_REFUSED': ['x'], 'NOW_REFUSED': ['y']}}
        options = {'SAME': {'values': {'1': []}}, 'CHANGED': {'values': {'1': [{'w': 1}]}}, 'NEW': {'values': {}}}
        unsupported = {'STILL_REFUSED': ['x'], 'NOW_REFUSED': ['z']}
        self.assertEqual(['CHANGED', 'NEW', 'NOW_REFUSED'], extract.options_to_verify(options, unsupported, reference))
        self.assertEqual(sorted([*options, *unsupported]), extract.options_to_verify(options, unsupported, None))


class FakeElf:
    """Just what unmeasured() reads: one .rodata symbol whose second word is relocated to a local target."""

    def __init__(self, data, target):
        self.data = data
        self.sections = [{'name': ''}, {'name': '.rodata'}]
        self.symbols = [{'name': 'sTable', 'section': 1, 'value': 0, 'size': len(data)}]
        self.target = target

    def bytes_at(self, section, offset, size):
        return self.data[offset:offset + size]

    def relocations_in(self, section, start, end):
        return [o for o in [4] if start <= o < end]

    def pointer(self, section, offset):
        return ('local', self.target, False)


class UnmeasuredHashes(unittest.TestCase):
    # OW_FOLLOWERS_WEATHER_FORMS and I_GRISEOUS_ORB_FORM_CHANGE were refused because a longer form-change table moved later
    # data, changing the addends REL relocations store in sTeachableLearnsetVariants although every pointer still reached
    # the same learnset. A relocated word is compared by what it resolves to, not by its stored bytes.
    def test_a_moved_addend_with_the_same_target_is_not_a_change(self):
        before = FakeElf(b'\x01\x00\x00\x00\x10\x00\x00\x00', 'learnset')
        after = FakeElf(b'\x01\x00\x00\x00\x30\x00\x00\x00', 'learnset')
        self.assertEqual(extract.unmeasured(before), extract.unmeasured(after))

    def test_a_different_target_or_plain_data_is_a_change(self):
        before = FakeElf(b'\x01\x00\x00\x00\x10\x00\x00\x00', 'learnset')
        self.assertNotEqual(extract.unmeasured(before), extract.unmeasured(FakeElf(before.data, 'other learnset')))
        self.assertNotEqual(extract.unmeasured(before), extract.unmeasured(FakeElf(b'\x02' + before.data[1:], 'learnset')))



def tiny_elf(inner_at, inner_bytes=b'abcd'):
    """An Elf without a file: one .rodata section holding a field pointing at local data sLocal (8 bytes), whose first word is
    relocated through the section symbol to sInner (4 bytes at inner_at). Moving sInner changes only that word's addend."""
    elf = object.__new__(extract.Elf)
    data = bytearray(32)
    data[8:12] = inner_at.to_bytes(4, 'little')  # the REL addend of sLocal's pointer
    data[12:16] = b'wxyz'
    data[inner_at:inner_at + 4] = inner_bytes
    elf.data = bytes(data)
    elf.sections = [{'name': '', 'offset': 0}, {'name': '.rodata', 'offset': 0}]
    elf.symbols = [{'name': '', 'value': 0, 'size': 0, 'section': 0, 'bind': 0, 'kind': 0},
                   {'name': '', 'value': 0, 'size': 0, 'section': 1, 'bind': 0, 'kind': 3},
                   {'name': 'sLocal', 'value': 8, 'size': 8, 'section': 1, 'bind': 0, 'kind': 1},
                   {'name': 'sInner', 'value': inner_at, 'size': 4, 'section': 1, 'bind': 0, 'kind': 1}]
    elf.relocations = {1: {0: 2, 8: 1}}
    elf.relocated = {1: [0, 8]}
    elf.local_data, elf._contents = {}, {}
    return elf


class PointedDataByContent(unittest.TestCase):
    # P_SHEDINJA_BALL was refused: dropping Nincada's condition list moved later data, so every species' animation table
    # (local data holding pointers) had different stored addends and its frontAnimFrames "pointed elsewhere"
    def test_a_moved_inner_target_is_the_same_data(self):
        self.assertEqual(tiny_elf(16).pointer(1, 0), tiny_elf(20).pointer(1, 0))

    def test_different_inner_content_is_different_data(self):
        self.assertNotEqual(tiny_elf(16).pointer(1, 0), tiny_elf(16, b'abce').pointer(1, 0))
        self.assertTrue(tiny_elf(16).pointer(1, 0)[2])  # it holds a pointer of its own


    # A field the descriptor reads as one value (a species' overworldData struct) can hold pointers; run 22c measured every
    # species' overworldData as changed because only their stored addends moved
    def test_a_value_field_holding_a_pointer_compares_by_target(self):
        field = {'name': 'overworldData', 'kind': 'value', 'offset': 0, 'size': 8}
        self.assertEqual(extract.read_field(tiny_elf(16), 1, 8, field), extract.read_field(tiny_elf(20), 1, 8, field))
        self.assertNotEqual(extract.read_field(tiny_elf(16), 1, 8, field), extract.read_field(tiny_elf(16, b'abce'), 1, 8, field))
        plain = {'name': 'baseHP', 'kind': 'value', 'offset': 4, 'size': 4}  # bytes 12..16 hold no relocation
        self.assertEqual(int.from_bytes(b'wxyz', 'little'), extract.read_field(tiny_elf(16), 1, 8, plain))


class FunctionPointersByName(unittest.TestCase):
    # C_UPDATED_MOVE_EFFECTS was refused: gContestEffects[].function points at local effect functions whose code reads
    # gContestEffects, so the functions' content digests changed with the descriptions (and depended on the order the cycle
    # was walked); a pointer to code is the function it names
    @staticmethod
    def with_local_function(inner_bytes):
        elf = tiny_elf(16, inner_bytes)
        elf.symbols[2] = dict(elf.symbols[2], name='ContestEffect_Local', kind=2)
        return elf

    def test_a_function_reaching_changed_data_is_the_same_function(self):
        self.assertEqual(self.with_local_function(b'abcd').pointer(1, 0), self.with_local_function(b'abce').pointer(1, 0))
        self.assertEqual(('symbol', 'ContestEffect_Local', 0), self.with_local_function(b'abcd').reference(1, 0, ()))


if __name__ == '__main__':
    unittest.main()
