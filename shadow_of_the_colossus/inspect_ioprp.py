#!/usr/bin/env python3
"""Inspect a ROMDIR-at-offset-zero IOP reboot image or extract one member."""

import argparse
import json
from pathlib import Path
import struct
import sys

MAX_IMAGE_SIZE = 64 * 1024 * 1024


def parse_image(data: bytes) -> dict[str, tuple[int, int]]:
    if len(data) < 48 or len(data) > MAX_IMAGE_SIZE:
        raise ValueError("invalid IOP image size")
    first = struct.unpack_from("<10sHI", data, 0)
    second = struct.unpack_from("<10sHI", data, 16)
    if first[0].rstrip(b"\0") != b"RESET" or first[2] != 0 or second[0].rstrip(b"\0") != b"ROMDIR":
        raise ValueError("expected RESET and ROMDIR at offset zero")
    dir_size = second[2]
    if dir_size < 48 or dir_size % 16 or dir_size > len(data):
        raise ValueError("invalid ROMDIR size")
    entries = {}
    offset = 0
    terminated = False
    for pos in range(0, dir_size, 16):
        raw_name, _, size = struct.unpack_from("<10sHI", data, pos)
        if raw_name == bytes(10):
            terminated = True
            break
        name_bytes = raw_name.split(b"\0", 1)[0]
        if raw_name[len(name_bytes):].strip(b"\0"):
            raise ValueError("invalid member name padding")
        try:
            name = name_bytes.decode("ascii")
        except UnicodeDecodeError as error:
            raise ValueError("non-ASCII member name") from error
        if not name or not all(ch.isascii() and (ch.isalnum() or ch in "_.-") for ch in name):
            raise ValueError("invalid member name")
        if name in entries:
            raise ValueError("duplicate member name")
        if offset + size > len(data):
            raise ValueError("member exceeds image")
        entries[name] = (offset, size)
        offset = (offset + size + 15) & ~15
    if not terminated:
        raise ValueError("ROMDIR terminator missing")
    return entries


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("--extract", metavar="MEMBER")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if bool(args.extract) != bool(args.output):
        parser.error("--extract and --output must be supplied together")
    try:
        if args.image.stat().st_size > MAX_IMAGE_SIZE:
            raise ValueError("image exceeds maximum size")
        data = args.image.read_bytes()
        entries = parse_image(data)
        if args.extract:
            if args.extract not in entries:
                raise ValueError(f"member not found: {args.extract}")
            offset, size = entries[args.extract]
            with args.output.open("xb") as out:
                out.write(data[offset:offset + size])
        else:
            print(json.dumps({name: {"offset": offset, "size": size}
                              for name, (offset, size) in entries.items()}, indent=2))
    except (OSError, ValueError, struct.error) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
