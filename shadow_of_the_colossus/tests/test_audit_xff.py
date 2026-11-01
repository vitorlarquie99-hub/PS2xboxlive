import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("audit_xff", Path(__file__).resolve().parents[1] / "audit_xff.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def fixture():
    data = bytearray(0x280)
    names = b"\0.shstrtab\0.strtab\0.symtab\0"
    strings = b"\0provided\0missing\0"
    data[0x120:0x120 + len(names)] = names
    data[0x150:0x150 + len(strings)] = strings
    struct.pack_into("<4I", data, 0x70, 0, 1, 11, 19)
    for index, (size, offset) in enumerate(((0, 0), (len(names), 0x120), (len(strings), 0x150), (48, 0x170))):
        struct.pack_into("<8I", data, 0x80 + index * 32, 0, 0, size, 4, 1, 0, 0, offset)
    struct.pack_into("<IIIBBH", data, 0x180, 1, 8, 4, 0x12, 0, 1)
    struct.pack_into("<IIIBBH", data, 0x190, 10, 0, 0, 0x10, 0, 0)
    struct.pack_into("<I", data, 0x1a0, 2)
    struct.pack_into("<7I", data, 0x1b0, 9, 1, 1, 0, 0, 0x1d0, 0x210)
    struct.pack_into("<II", data, 0x1d0, 4, (2 << 8) | 4)
    data[0x210:0x214] = data[0x124:0x128]
    data[:4] = b"xff2"
    header = [0] * 28
    header[0] = int.from_bytes(b"xff2", "little")
    header[5], header[7], header[9], header[14], header[16] = len(data), 1, 3, 1, 4
    header[20], header[21], header[22], header[23] = 0x1a0, 0x170, 0x150, 0x80
    header[25], header[26], header[27] = 0x1b0, 0x70, 0x120
    struct.pack_into("<28I", data, 0, *header)
    return data


class AuditXffTest(unittest.TestCase):
    def test_parse_tables_and_resolve_by_name(self):
        with tempfile.TemporaryDirectory() as directory:
            first = Path(directory) / "one.xff"
            second = Path(directory) / "two.xff"
            first.write_bytes(fixture())
            data = fixture()
            struct.pack_into("<I", data, 0x190, 1)
            second.write_bytes(data)
            one = module.read_module(first)
            two = module.read_module(second)
        result = module.audit([one, two])
        self.assertEqual(one["relocations"], {"4": 1})
        self.assertEqual(result[0]["imports_without_candidates"], 1)
        self.assertEqual(result[1]["imports_with_candidates"], 1)
        self.assertEqual(result[1]["import_details"][0]["candidates"][0]["module"], "one.xff")

    def test_hi16_pairs_and_saved_word_validation(self):
        data = fixture()
        struct.pack_into("<I", data, 0x1b4, 5)
        for index, (offset, kind) in enumerate(((0, 5), (4, 5), (8, 6), (12, 5), (0x100, 4))):
            struct.pack_into("<II", data, 0x1d0 + index * 8, offset, (2 << 8) | kind)
            if offset < 0x100:
                data[0x210 + index * 8:0x214 + index * 8] = data[0x120 + offset:0x124 + offset]
        data[0x218] ^= 1
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "pairs.xff"
            path.write_bytes(data)
            result = module.read_module(path)["relocation_validation"]
        self.assertEqual(result["hi16_total"], 3)
        self.assertEqual(result["hi16_without_following_lo16"], 1)
        self.assertEqual(result["saved_words_differ"], 1)
        self.assertEqual(result["saved_words_outside_target"], 1)
        self.assertEqual(result["saved_words_unchecked"], 0)

    def test_hi16_at_end_and_unstored_target(self):
        data = fixture()
        struct.pack_into("<I", data, 0xb0, 8)  # SHT_NOBITS
        struct.pack_into("<II", data, 0x1d0, 4, (2 << 8) | 5)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "end.xff"
            path.write_bytes(data)
            result = module.read_module(path)["relocation_validation"]
        self.assertEqual(result["hi16_without_following_lo16"], 1)
        self.assertEqual(result["saved_words_unchecked"], 1)

    def test_rejects_invalid_tables(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.xff"
            for offset, value in ((0x14, 1), (0x1a0, 99), (0x1b0 + 20, 0x27f),
                                  (0x1b0 + 24, 0x27f), (0x1b0, 4)):
                data = fixture()
                struct.pack_into("<I", data, offset, value)
                path.write_bytes(data)
                with self.assertRaises(ValueError):
                    module.read_module(path)


if __name__ == "__main__":
    unittest.main()
