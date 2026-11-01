"""Experimental XFF2 relocation arithmetic for explicitly resolved symbols.

Based on the SCUS_974.72 loader at 0x001000d0. This module does not choose
load addresses, resolve imports, map memory, or execute the game.
"""

from dataclasses import dataclass
import struct
from typing import Mapping, Sequence


@dataclass(frozen=True)
class Relocation:
    offset: int
    kind: int
    symbol: int
    saved_word: int


def relocate_section(section: bytes, entries: Sequence[Relocation],
                     symbols: Mapping[int, int]) -> bytes:
    """Apply one ordered XFF table to a copy using already resolved symbol values.

    Saved words are the unrelocated originals from XFF, so repeated relocation
    does not use prior results as addends. Raises on unsupported or missing data.
    """
    result = bytearray(section)
    for i, entry in enumerate(entries):
        kind = entry.kind
        if kind == 0:
            continue
        if kind not in (2, 4, 5, 6):
            raise ValueError(f"unsupported relocation type {kind} at index {i}")
        if entry.offset < 0 or entry.offset % 4 or entry.offset + 4 > len(result):
            raise ValueError(f"invalid relocation offset at index {i}")
        if not 0 <= entry.saved_word <= 0xFFFFFFFF:
            raise ValueError(f"invalid saved word at index {i}")
        if entry.symbol not in symbols or not 0 <= symbols[entry.symbol] <= 0xFFFFFFFF:
            raise ValueError(f"unresolved or invalid symbol {entry.symbol} at index {i}")
        value = symbols[entry.symbol]
        word = entry.saved_word
        if kind == 2:
            relocated = (word + value) & 0xFFFFFFFF
        elif kind == 4:
            relocated = (word + ((value >> 2) & 0x03FFFFFF)) & 0xFFFFFFFF
        elif kind == 6:
            relocated = (word & 0xFFFF0000) | ((word + value) & 0xFFFF)
        else:
            next_index = i + 1
            while next_index < len(entries) and entries[next_index].kind == 5:
                next_index += 1
            if next_index == len(entries) or entries[next_index].kind != 6:
                raise ValueError(f"HI16 without following LO16 at index {i}")
            low_word = entries[next_index].saved_word
            if not 0 <= low_word <= 0xFFFFFFFF:
                raise ValueError(f"invalid LO16 saved word at index {next_index}")
            low = low_word & 0xFFFF
            if low & 0x8000:
                low -= 0x10000
            full = (value + (word << 16) + low) & 0xFFFFFFFF
            high = ((full + 0x8000) >> 16) & 0xFFFF
            relocated = (word & 0xFFFF0000) | high
        struct.pack_into("<I", result, entry.offset, relocated)
    return bytes(result)
