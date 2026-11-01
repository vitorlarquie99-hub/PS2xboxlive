#!/usr/bin/env python3
"""Read observed boot fields from an SCUS_974.72 diagnostic EE RAM capture.

GAMECORE fields require the reconstructed module layout used in the boot report.
Values are observations, not proof of successful initialization or disconnection.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

BOOT_SHA256 = "d16ad76ba8cdf87695d94b1316d51c1f248959ec0749814c31bb7a2d55d9a2e1"
RAM_SIZE = 32 * 1024 * 1024


def inspect(elf: Path, capture: Path) -> dict:
    if hashlib.sha256(elf.read_bytes()).hexdigest() != BOOT_SHA256:
        raise ValueError("unsupported boot ELF: addresses require the documented SCUS_974.72 hash")
    ram = capture.read_bytes()
    if len(ram) != RAM_SIZE:
        raise ValueError("expected exactly 32 MiB of EE RAM")

    def word(address: int) -> int:
        return struct.unpack_from("<I", ram, address)[0]

    pads = []
    for port in range(2):
        pad = 0x0013D840 + port * 0x140
        socket = word(pad + 0x11C)
        entry = 0x0014B598 + socket * 0x334
        # Socket values are guest data; never use an unchecked index to decode RAM.
        valid_entry = socket < 2
        pads.append({"port": port, "state": word(pad + 0x118), "socket": socket,
                     "dbc_device_status": word(0x00150D00 + socket * 4) if valid_entry else None,
                     "socket_open": word(entry) if valid_entry else None,
                     "socket_sync": word(entry + 4) if valid_entry else None,
                     "packet_hex": ram[pad + 0x104:pad + 0x10C].hex()})
    return {"boot_sha256": BOOT_SHA256, "ram_sha256": hashlib.sha256(ram).hexdigest(),
            "layout": "documented combined boot/KERNEL/MANAGER/GAMECORE diagnostic",
            "gp_finish_counter": struct.unpack_from("<Q", ram, 0x01465648)[0],
            "boot_mc_state": word(0x014664BC), "boot_mc_done": word(0x011E0158),
            "pads": pads}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=Path)
    parser.add_argument("ram", type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(inspect(args.elf, args.ram), indent=2))
    except (OSError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
