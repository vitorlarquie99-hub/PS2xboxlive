#!/usr/bin/env python3
"""Identify a local PS2 ELF for game-specific analysis without copying it."""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys


def inspect(path: Path) -> dict:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        header = file.read(52)
        if len(header) != 52 or header[:4] != b"\x7fELF":
            raise ValueError("not a complete ELF32 header")
        if header[4:7] != b"\x01\x01\x01":
            raise ValueError("expected a little-endian ELF32 file")
        elf_type, machine, version, entry = struct.unpack_from("<HHII", header, 16)
        if elf_type != 2 or machine != 8 or version != 1:
            raise ValueError("expected an executable MIPS ELF")
        digest.update(header)
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return {"file": path.name, "sha256": digest.hexdigest(), "entry": f"0x{entry:08X}",
            "format": "ELF32 little-endian MIPS executable"}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=Path, help="path to the ELF from your own PS2 disc")
    args = parser.parse_args()
    try:
        print(json.dumps(inspect(args.elf), indent=2))
    except (OSError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
