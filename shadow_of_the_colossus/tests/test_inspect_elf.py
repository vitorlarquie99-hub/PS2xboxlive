import hashlib
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("inspect_elf", Path(__file__).resolve().parents[1] / "inspect_elf.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def elf_header():
    header = bytearray(52)
    header[:7] = b"\x7fELF\x01\x01\x01"
    struct.pack_into("<HHII", header, 16, 2, 8, 1, 0x100000)
    return header


class InspectElfTest(unittest.TestCase):
    def test_reports_identity_and_entry(self):
        data = elf_header() + b"synthetic payload"
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "test.elf"
            path.write_bytes(data)
            report = module.inspect(path)
        self.assertEqual(report["sha256"], hashlib.sha256(data).hexdigest())
        self.assertEqual(report["entry"], "0x00100000")
        self.assertEqual(report["file"], "test.elf")

    def test_rejects_truncated_and_wrong_architecture(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.elf"
            for data in (b"\x7fELF", bytes(elf_header()[:18] + b"\x3e\x00" + elf_header()[20:])):
                path.write_bytes(data)
                with self.assertRaises(ValueError):
                    module.inspect(path)


if __name__ == "__main__":
    unittest.main()
