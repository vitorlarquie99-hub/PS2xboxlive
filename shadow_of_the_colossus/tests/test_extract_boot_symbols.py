import importlib.util
from pathlib import Path
import sys
import unittest

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root))
spec = importlib.util.spec_from_file_location('extract_boot_symbols', root / 'extract_boot_symbols.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class BootSymbolsTest(unittest.TestCase):
    def test_selects_absolute_names_and_deduplicates(self):
        symbols = [
            {'section': 0xfff1, 'value': 0x100010, 'name': 'start'},
            {'section': 0xfff1, 'value': 0x100010, 'name': 'start'},
            {'section': 1, 'value': 0x10, 'name': 'local'},
            {'section': 0, 'value': 0, 'name': 'import'},
            {'section': 0xfff1, 'value': 0, 'name': ''},
            {'section': 0xfff1, 'value': 3, 'name': 'other'},
        ]
        self.assertEqual(module.absolute_symbols({'symbols': symbols}), [(3, 'other'), (0x100010, 'start')])

    def test_rejects_names_that_would_corrupt_map(self):
        with self.assertRaises(ValueError):
            module.absolute_symbols({'symbols': [{'section': 0xfff1, 'value': 0, 'name': 'bad name'}]})


if __name__ == '__main__':
    unittest.main()
