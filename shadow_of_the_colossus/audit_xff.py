#!/usr/bin/env python3
"""Inventory XFF2 imports and possible providers; does not link or relocate code."""

import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
import struct
import sys


def read_module(path):
    data = path.read_bytes()

    def span(offset, size):
        if offset < 0 or size < 0 or offset + size > len(data):
            raise ValueError(f"{path}: table extends past end of file")
        return data[offset:offset + size]

    def words(offset, count):
        return struct.unpack(f"<{count}I", span(offset, count * 4))

    def string(table, offset):
        if offset >= len(table):
            raise ValueError(f"{path}: invalid string offset")
        end = table.find(b"\0", offset)
        if end < 0:
            raise ValueError(f"{path}: unterminated string")
        return table[offset:end].decode("latin-1")

    if len(data) < 0x70 or data[:4] != b"xff2":
        raise ValueError(f"{path}: expected XFF2 header")
    h = words(0, 28)
    if h[5] != len(data):
        raise ValueError(f"{path}: header size does not match file")
    section_count, symbol_count, import_count, reloc_count = h[16], h[9], h[7], h[14]
    names = words(h[26], section_count)
    sections = [words(h[23] + i * 32, 8) for i in range(section_count)]
    # The string-table length is held in its section record.
    name_base = h[27]
    section_names = [string(span(name_base, len(data) - name_base), n) for n in names]
    try:
        string_section = sections[section_names.index(".strtab")]
        symbol_section = sections[section_names.index(".symtab")]
    except ValueError as error:
        raise ValueError(f"{path}: missing string or symbol table") from error
    strtab = span(h[22], string_section[2])
    if symbol_section[2] != symbol_count * 16:
        raise ValueError(f"{path}: inconsistent symbol count")
    raw_symbols = span(h[21], symbol_count * 16)
    symbols = []
    for i in range(symbol_count):
        name, value, size, info, other, section = struct.unpack_from("<IIIBBH", raw_symbols, i * 16)
        symbols.append({"name": string(strtab, name), "section": section, "binding": info >> 4,
                        "visibility": other & 3, "value": value, "size": size})
    imports = words(h[20], import_count)
    if any(index >= symbol_count or symbols[index]["section"] != 0 for index in imports):
        raise ValueError(f"{path}: invalid import index")
    relocation_types = Counter()
    validation = Counter(hi16_total=0, hi16_without_following_lo16=0,
                         saved_words_differ=0, saved_words_outside_target=0,
                         saved_words_unchecked=0)
    for i in range(reloc_count):
        table = words(h[25] + i * 28, 7)
        if table[2] >= section_count:
            raise ValueError(f"{path}: invalid relocation target")
        if table[0] != 9:
            raise ValueError(f"{path}: unsupported relocation table type {table[0]}")
        entries = span(table[5], table[1] * 8)
        saved_words = span(table[6], table[1] * 8)
        target = sections[table[2]]
        stored = target[4] in (1, 2, 0x7FFFF420)
        contents = span(target[7], target[2]) if stored else None
        pending_hi16 = 0
        for j in range(table[1]):
            offset, info = struct.unpack_from("<II", entries, j * 8)
            if info >> 8 >= symbol_count:
                raise ValueError(f"{path}: invalid relocation symbol")
            kind = info & 255
            relocation_types[str(kind)] += 1
            if kind == 5:
                validation["hi16_total"] += 1
                pending_hi16 += 1
            else:
                if kind != 6:
                    validation["hi16_without_following_lo16"] += pending_hi16
                pending_hi16 = 0
            if contents is None:
                validation["saved_words_unchecked"] += 1
            elif offset + 4 > len(contents):
                validation["saved_words_outside_target"] += 1
            elif contents[offset:offset + 4] != saved_words[j * 8:j * 8 + 4]:
                validation["saved_words_differ"] += 1
        validation["hi16_without_following_lo16"] += pending_hi16
    return {"name": path.name, "symbols": symbols, "imports": imports,
            "relocations": dict(sorted(relocation_types.items(), key=lambda item: int(item[0]))),
            "relocation_validation": dict(validation), "section_count": section_count}


def audit(modules):
    providers = defaultdict(list)
    for module in modules:
        for symbol in module["symbols"]:
            if (symbol["name"] and symbol["section"] != 0 and symbol["binding"] in (1, 2)
                    and symbol["visibility"] in (0, 3)):
                providers[symbol["name"]].append({"module": module["name"],
                    "section": symbol["section"], "binding": symbol["binding"],
                    "value": symbol["value"]})
    result = []
    for module in modules:
        imports = []
        for index in module["imports"]:
            symbol = module["symbols"][index]
            imports.append({"index": index, "name": symbol["name"],
                            "candidates": [p for p in providers[symbol["name"]]
                                           if p["module"] != module["name"]]})
        result.append({"module": module["name"], "symbols": len(module["symbols"]),
            "sections": module["section_count"], "relocation_types": module["relocations"],
            "relocation_validation": module["relocation_validation"],
            "imports": len(imports), "imports_with_candidates": sum(bool(i["candidates"]) for i in imports),
            "imports_without_candidates": sum(not i["candidates"] for i in imports),
            "imports_with_multiple_candidates": sum(len(i["candidates"]) > 1 for i in imports),
            "import_details": imports})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("modules", nargs="+", type=Path)
    args = parser.parse_args()
    if len({p.name for p in args.modules}) != len(args.modules):
        parser.error("module filenames must be unique")
    try:
        print(json.dumps(audit([read_module(path) for path in args.modules]), indent=2))
    except (OSError, ValueError, struct.error) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
