import importlib.util
from pathlib import Path
import struct
import sys
import unittest

spec = importlib.util.spec_from_file_location("xff_relocations", Path(__file__).resolve().parents[1] / "xff_relocations.py")
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)
R = module.Relocation


class RelocationsTest(unittest.TestCase):
    def test_words_and_jump_match_loader_addition(self):
        source = bytes(16)
        entries = [R(0, 2, 1, 0xFFFFFFF0), R(4, 4, 2, 0x0BFFFFFF),
                   R(8, 6, 3, 0x2484FFFF), R(12, 0, 100, 0)]
        result = module.relocate_section(source, entries, {1: 0x30, 2: 4, 3: 2})
        self.assertEqual(struct.unpack('<4I', result), (0x20, 0x0C000000, 0x24840001, 0))
        self.assertEqual(source, bytes(16))

    def test_hi_lo_signed_carry_and_multiple_hi(self):
        entries = [R(0, 5, 1, 0x3C010001), R(4, 5, 1, 0x3C020002),
                   R(8, 6, 1, 0x24218000)]
        result = module.relocate_section(bytes(12), entries, {1: 0x12348000})
        self.assertEqual(struct.unpack('<3I', result), (0x3C011235, 0x3C021236, 0x24210000))
        self.assertEqual(module.relocate_section(result, entries, {1: 0x12348000}), result)

    def test_rejects_unresolved_invalid_and_unpaired(self):
        cases = [([R(0, 2, 3, 0)], {}), ([R(1, 2, 3, 0)], {3: 0}),
                 ([R(4, 2, 3, 0)], {3: 0}), ([R(0, 7, 3, 0)], {3: 0}),
                 ([R(0, 5, 3, 0)], {3: 0}), ([R(0, 2, 3, 0)], {3: -1}),
                 ([R(0, 2, 3, -1)], {3: 0})]
        for entries, symbols in cases:
            with self.subTest(entries=entries, symbols=symbols), self.assertRaises(ValueError):
                module.relocate_section(bytes(4), entries, symbols)


if __name__ == "__main__":
    unittest.main()
