import importlib.util
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

path = Path(__file__).resolve().parents[1] / "inspect_ioprp.py"
spec = importlib.util.spec_from_file_location("inspect_ioprp", path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def fixture():
    data = bytearray(96)
    for i, (name, size) in enumerate(((b'RESET', 0), (b'ROMDIR', 64), (b'FILEIO', 5))):
        struct.pack_into('<10sHI', data, i * 16, name, 0, size)
    data[64:69] = b'hello'
    return data


class InspectIoprpTest(unittest.TestCase):
    def test_alignment_and_extraction_never_overwrites(self):
        with tempfile.TemporaryDirectory() as directory:
            image = Path(directory) / 'image'
            dest = Path(directory) / 'out'
            image.write_bytes(fixture())
            self.assertEqual(module.parse_image(image.read_bytes())['FILEIO'], (64, 5))
            cmd = [sys.executable, str(path), str(image), '--extract', 'FILEIO', '--output', str(dest)]
            self.assertEqual(subprocess.run(cmd, capture_output=True).returncode, 0)
            self.assertEqual(dest.read_bytes(), b'hello')
            self.assertNotEqual(subprocess.run(cmd, capture_output=True).returncode, 0)
            self.assertEqual(dest.read_bytes(), b'hello')

    def test_rejects_truncated_duplicate_and_unterminated(self):
        bad = []
        data = fixture(); struct.pack_into('<I', data, 44, 0xffff); bad.append(data)
        data = fixture(); data[32:42] = b'RESET\0\0\0\0\0'; bad.append(data)
        data = fixture(); data[48:58] = b'NOEND\0\0\0\0\0'; bad.append(data)
        data = fixture(); data[32:42] = b'../bad\0\0\0\0'; bad.append(data)
        for data in bad:
            with self.subTest(header=bytes(data[:64])), self.assertRaises(ValueError):
                module.parse_image(data)


if __name__ == '__main__':
    unittest.main()
