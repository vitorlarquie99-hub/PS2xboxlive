import importlib.util
from pathlib import Path
import tomllib
import tempfile
import subprocess
import sys
import unittest

spec = importlib.util.spec_from_file_location('prepare_boot_config', Path(__file__).resolve().parents[1] / 'prepare_boot_config.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def fixture():
    stubs = ',\n'.join('  "' + x + '"' for x in (*module.GUEST_FUNCTIONS, 'sceOpen@0x100000'))
    return '[general]\ninput = "old"\nghidra_output = ""\noutput = "old"\nstubs = [\n' + stubs + ',\n]\n'


class PrepareConfigTest(unittest.TestCase):
    def test_retains_unrelated_stubs_and_sets_paths(self):
        result = tomllib.loads(module.prepare(fixture(), '/a b/game', '/a b/map', '/a b/output'))['general']
        self.assertEqual(result['stubs'], ['sceOpen@0x100000'])
        self.assertEqual(result['input'], '/a b/game')
        self.assertEqual(result['ghidra_output'], '/a b/map')
        self.assertEqual(result['output'], '/a b/output/')
        self.assertEqual(result['recompile_addresses'], [0x109288, 0x1092B8, 0x10A740, 0x10A8C0])

    def test_preserves_existing_guest_choices(self):
        text = fixture().replace('[general]', '[general]\nrecompile_addresses = [0x100000]')
        result = tomllib.loads(module.prepare(text, '/a', '/b', '/c'))['general']
        self.assertIn(0x100000, result['recompile_addresses'])
        self.assertIn(0x10A8C0, result['recompile_addresses'])

    def test_rejects_invalid_guest_addresses(self):
        for value in ('true', '-4', '0x1001', '0x100000000'):
            text = fixture().replace('[general]', f'[general]\nrecompile_addresses = [{value}]')
            with self.assertRaises(ValueError):
                module.prepare(text, '/a', '/b', '/c')

    def test_rejects_wrong_elf_without_creating_config(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'game').write_bytes(b'wrong ELF')
            (root / 'config').write_text(fixture())
            (root / 'map').write_text('Name,Start,End,Size\n')
            cmd = [sys.executable, str(Path(module.__file__)), '--elf', str(root / 'game'),
                   '--config', str(root / 'config'), '--function-map', str(root / 'map'),
                   '--generated-dir', str(root / 'output'), '--output-config', str(root / 'prepared')]
            result = subprocess.run(cmd, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse((root / 'prepared').exists())

    def test_refuses_missing_or_duplicate_binding(self):
        text = fixture()
        for changed in (text.replace(module.GUEST_FUNCTIONS[0], 'missing'),
                        text.replace('  "' + module.GUEST_FUNCTIONS[0] + '",',
                                     '  "' + module.GUEST_FUNCTIONS[0] + '",\n  "' + module.GUEST_FUNCTIONS[0] + '",')):
            with self.assertRaises(ValueError):
                module.prepare(changed, '/a', '/b', '/c')


if __name__ == '__main__':
    unittest.main()
