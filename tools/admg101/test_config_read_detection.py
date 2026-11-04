#!/usr/bin/env python3
"""Regression cases for the configuration read finder shared by audit_r38_config_reads.py and
extract_r38_config_variants.py: every form that reads a compiled value is found, and only a genuine GetConfig(...) selector
call is excluded. Run: python test_config_read_detection.py (stdlib unittest; builds a small source tree in a temp dir)."""
import importlib.util
import io
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

HERE = Path(__file__).resolve().parent


def load(name):
    spec = importlib.util.spec_from_file_location(name, HERE/f'{name}.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


audit = load('audit_r38_config_reads')

CONFIG_CHANGES = '''#define BATTLE_CONFIG_DEFINITIONS(F) \\
    F(B_OPT, opt, (u32, GEN_COUNT - 1)) \\
    F(B_OTHER, other, (u32, 1))

#define POKEMON_CONFIG_DEFINITIONS(F) \\
    F(EGG_MOVES, eggMoves, (u32, 1))

#define AI_CONFIG_DEFINITIONS(F) \\
    F(AI_FLAG, aiFlag, (u32, 1))

'''


class FindReads(unittest.TestCase):
    def reads(self, text):
        return [name for name, _ in audit.find_reads(text, ['B_OPT', 'P_EGG_MOVES'])]

    def test_parenthesised_code_read_is_found(self):
        # The form the old lookbehind skipped
        self.assertEqual(self.reads('    if (B_OPT >= GEN_5)'), ['B_OPT'])
        self.assertEqual(self.reads('x = y / (B_OPT >= GEN_7 ? 4 : 2);'), ['B_OPT'])
        self.assertEqual(self.reads('f(a, (B_OPT == TRUE ? 1 : 0))'), ['B_OPT'])

    def test_selector_call_is_excluded(self):
        self.assertEqual(self.reads('if (GetConfig(B_OPT) >= GEN_5)'), [])
        self.assertEqual(self.reads('GetConfig( B_OPT )'), [])
        self.assertEqual(self.reads('GetConfig(\tP_EGG_MOVES)'), [])

    def test_other_identifiers_are_not_reads(self):
        self.assertEqual(self.reads('GetConfigInternal(CONFIG_B_OPT); GetConfig_B_OPT(); B_OPTION_X; MY_B_OPT'), [])

    def test_a_call_that_only_looks_like_the_selector_is_a_read(self):
        self.assertEqual(self.reads('MyGetConfig(B_OPT)'), ['B_OPT'])
        self.assertEqual(self.reads('GetConfigs(B_OPT)'), ['B_OPT'])

    def test_each_read_on_a_line_is_found(self):
        self.assertEqual(self.reads('(B_OPT < GEN_5 || GetConfig(B_OPT) > 2 || B_OPT == 3)'), ['B_OPT', 'B_OPT'])


class DirectReads(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = self.root = Path(self.tmp.name)
        files = {
            'include/constants/config_changes.h': CONFIG_CHANGES,
            'include/config/battle.h': '#define B_OPT GEN_LATEST\n',
            'src/code.c': 'void f(void)\n{\n    if (B_OPT >= GEN_5)\n        g();\n    if (GetConfig(B_OPT) >= GEN_5)\n        g();\n'
                          '    // a comment naming B_OPT is not a read\n}\n',
            'src/data/table.h': '    .power = B_OPT >= GEN_6 ? 40 : 35,\n    .accuracy = (B_OPT >= GEN_4) ? 100 : 95,\n',
            'src/cond.c': '#if B_OPT >= GEN_4\nint x;\n#endif\n',
            'include/constants/derived.h': '#define DERIVED (P_EGG_MOVES ? 1 : 2)\n',
            'data/scripts.s': '.if B_OPT >= GEN_7\n\tcall A\n.endif\n\tcreatevisualtask T, 2, GEN_7 @ chosen by B_OPT at run time\n',
            # Not reads: block comments (one line or several) and the test framework, which the game build never compiles
            'src/commented.c': '/* B_OPT is documented here */\n/*\n * WITH_CONFIG(B_OPT, GEN_6);\n */\nint y = 1; /* B_OPT */\n',
            'include/test/battle.h': '#define TEST_OPT (B_OPT ? 1 : 0)\n',
            'test/battle/move.c': 'if (B_OPT >= GEN_5) {}\n',
        }
        for rel, text in files.items():
            (root/rel).parent.mkdir(parents=True, exist_ok=True)
            (root/rel).write_text(text, encoding='utf-8')

    def tearDown(self):
        self.tmp.cleanup()

    def test_every_form_is_classified(self):
        reads = sorted((tag, rel, kind) for tag, rel, _, kind in audit.direct_reads(self.root, audit.options(self.root)))
        self.assertEqual(reads, sorted([
            ('B_OPT', 'src/code.c', 'code'),
            ('B_OPT', 'src/data/table.h', 'data'),
            ('B_OPT', 'src/data/table.h', 'data'),
            ('B_OPT', 'src/cond.c', 'preprocessor'),
            ('EGG_MOVES', 'include/constants/derived.h', 'macro'),
            ('B_OPT', 'data/scripts.s', 'assembly'),
        ]))

    def test_designated_initializers_in_c_files_are_data(self):
        # A table row or field initializer in a .c file keeps its compiled value like src/data/; a statement stays code
        self.assertEqual(audit.kind_of('src/battle_main.c', '    [TRAINER_CLASS_HIKER] = { 10, B_OPT >= GEN_8 ? 2 : 1 },'), 'data')
        self.assertEqual(audit.kind_of('src/battle_message.c', '        .color.foreground = B_OPT ? 13 : 12,'), 'data')
        self.assertEqual(audit.kind_of('src/code.c', '    if (array[B_OPT] == 1)'), 'code')
        self.assertEqual(audit.kind_of('src/code.c', '    x = B_OPT;'), 'code')

    def run_audit(self, *flags):
        argv, out, err = sys.argv, io.StringIO(), io.StringIO()
        sys.argv = ['audit', str(self.root), *flags]
        try:
            with redirect_stdout(out), redirect_stderr(err):
                code = audit.main()
        finally:
            sys.argv = argv
        return code, out.getvalue()

    def test_check_fails_on_a_direct_code_read(self):
        self.run_audit('--emit')
        code, out = self.run_audit('--check')
        self.assertEqual(code, 1)
        self.assertIn('direct code read: B_OPT at src/code.c:3', out)

    def test_check_passes_once_the_read_is_routed(self):
        path = self.root/'src/code.c'
        path.write_text(path.read_text(encoding='utf-8').replace('if (B_OPT >= GEN_5)', 'if (GetConfig(B_OPT) >= GEN_5)'), encoding='utf-8')
        self.run_audit('--emit')
        self.assertEqual(self.run_audit('--check')[0], 0)

    def test_options_read_outside_code_are_fixed(self):
        self.run_audit('--emit')
        header = (self.root/audit.HEADER).read_text(encoding='utf-8')
        self.assertIn('ADMG_CONFIG_FIXED_B_OPT = CONFIG_B_OPT', header)
        self.assertIn('ADMG_CONFIG_FIXED_EGG_MOVES = CONFIG_EGG_MOVES', header)
        self.assertNotIn('ADMG_CONFIG_FIXED_B_OTHER', header)


class ExtractorPlan(unittest.TestCase):
    """extract_r38_config_variants.plan finds data units with the same finder and keeps code reads fixed."""

    def test_parenthesised_data_read_selects_the_unit_and_code_reads_stay_elsewhere(self):
        extract = load('extract_r38_config_variants')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            files = {header: '' for header in extract.DATA_UNITS}
            files.update({
                'include/constants/config_changes.h': CONFIG_CHANGES,
                'include/constants/admg_config_catalog.h': 'enum ADMG_ConfigFixed\n{\n    ADMG_CONFIG_FIXED_B_OPT = CONFIG_B_OPT,\n};\n',
                'include/constants/admg_config_extra.h': '',
                'include/config/battle.h': '#define B_OPT GEN_LATEST\n#define B_OTHER FALSE\n',
                'src/data/moves_info.h': '        .accuracy = (B_OPT >= GEN_4) ? 100 : 95,\n',
                'src/battle_main.c': 'void f(void)\n{\n    if (B_OPT >= GEN_5)\n        g();\n}\n',
            })
            for rel, text in files.items():
                (root/rel).parent.mkdir(parents=True, exist_ok=True)
                (root/rel).write_text(text, encoding='utf-8')
            fixed, configs, macros, units, elsewhere = extract.plan(root)
            self.assertIn('src/move.c', units['B_OPT'])  # the old lookbehind found no unit here
            self.assertEqual(elsewhere['B_OPT'], ['src/battle_main.c:3'])  # a code read, even in a data unit


if __name__ == '__main__':
    unittest.main()
