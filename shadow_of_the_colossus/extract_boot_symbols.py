#!/usr/bin/env python3
"""List XFF absolute symbols as candidates for a boot ELF function map."""

import argparse
from pathlib import Path
import sys

from audit_xff import read_module


def absolute_symbols(module):
    rows = set()
    for symbol in module["symbols"]:
        name = symbol["name"]
        if symbol["section"] != 0xFFF1 or not name:
            continue
        if any(ch.isspace() for ch in name):
            raise ValueError("symbol name contains whitespace")
        rows.add((symbol["value"], name))
    return sorted(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xff", type=Path)
    args = parser.parse_args()
    try:
        rows = absolute_symbols(read_module(args.xff))
        for value, name in rows:
            print(f"{value:08x} {name}")
    except (OSError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
