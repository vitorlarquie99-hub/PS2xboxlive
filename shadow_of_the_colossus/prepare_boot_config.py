#!/usr/bin/env python3
"""Prepare an experimental analyzer configuration for the verified SCUS ELF."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
import tomllib

EXPECTED_SHA256 = 'd16ad76ba8cdf87695d94b1316d51c1f248959ec0749814c31bb7a2d55d9a2e1'
GUEST_FUNCTIONS = ('_sceFsSemInit@0x0010A740', '_sceRpcGetFPacket@0x00109288',
                   '_sceRpcGetFPacket2@0x001092B8', 'sceFsInit@0x0010A8C0')


def prepare(text, input_elf, function_map, generated_dir):
    config = tomllib.loads(text)
    general = config.get('general', {})
    stubs = general.get('stubs', [])
    addresses = general.get('recompile_addresses', [])
    if not isinstance(addresses, list) or any(type(a) is not int or a < 0 or a > 0xFFFFFFFF or a % 4 for a in addresses):
        raise ValueError('recompile_addresses requires aligned 32-bit addresses')
    addresses = sorted(set(addresses) | {int(name.split('@')[1], 16) for name in GUEST_FUNCTIONS})
    setting = 'recompile_addresses = [' + ', '.join(f'0x{a:08X}' for a in addresses) + ']'
    if 'recompile_addresses' in general:
        # Refuse unfamiliar formatting instead of leaving conflicting TOML keys.
        text, count = re.subn(r'(?m)^recompile_addresses[ \t]*=[ \t]*\[[^\r\n]*\][ \t]*$', lambda _: setting, text)
        if count != 1:
            raise ValueError('unsupported recompile_addresses formatting')
    else:
        text, count = re.subn(r'(?m)^\[general\][ \t]*$', lambda m: m[0] + '\n' + setting, text)
        if count != 1:
            raise ValueError('expected exactly one general table')
    for name in GUEST_FUNCTIONS:
        if stubs.count(name) != 1:
            raise ValueError(f'expected exactly one stub: {name}')
        pattern = r'(?m)^\s*' + re.escape(json.dumps(name)) + r'\s*,[ \t]*\r?\n'
        text, count = re.subn(pattern, '', text)
        if count != 1:
            raise ValueError(f'unsupported stub formatting: {name}')
    for key, value in (('input', input_elf), ('ghidra_output', function_map), ('output', str(generated_dir).rstrip('/\\') + '/')):
        pattern = r'(?m)^' + re.escape(key) + r'[ \t]*=[^\r\n]*$'
        text, count = re.subn(pattern, lambda _: f'{key} = {json.dumps(str(value))}', text)
        if count != 1:
            raise ValueError(f'expected exactly one {key} setting')
    tomllib.loads(text)
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', required=True, type=Path)
    parser.add_argument('--config', required=True, type=Path)
    parser.add_argument('--function-map', required=True, type=Path)
    parser.add_argument('--generated-dir', required=True, type=Path)
    parser.add_argument('--output-config', required=True, type=Path)
    args = parser.parse_args()
    try:
        with args.elf.open('rb') as f:
            digest = hashlib.file_digest(f, 'sha256').hexdigest()
        if digest != EXPECTED_SHA256:
            raise ValueError('ELF SHA-256 does not match the supported SCUS_974.72 build')
        if not args.function_map.is_file():
            raise ValueError('function map does not exist')
        result = prepare(args.config.read_text(encoding='utf-8'), args.elf.resolve(),
                         args.function_map.resolve(), args.generated_dir.resolve())
        with args.output_config.open('x', encoding='utf-8', newline='\n') as out:
            out.write(result)
    except (OSError, ValueError, tomllib.TOMLDecodeError) as error:
        print(f'Error: {error}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
