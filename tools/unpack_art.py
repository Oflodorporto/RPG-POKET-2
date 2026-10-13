#!/usr/bin/env python3
"""Reconstruct original archives, then apply the typed compact enemy replacement."""
from pathlib import Path
import argparse
import base64
import gzip
import struct
import sys
import zlib

sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parents[1]


def checked_pack(payload, size, crc, count):
    if len(payload) != size or zlib.crc32(payload) != crc:
        raise ValueError('Original asset archive length/CRC mismatch')
    return struct.pack('<4sIII', b'PKA1', size, crc, count) + payload


def reconstruct_legacy_pack(root=ROOT):
    """Rebuild the immutable 488-entry fixture, never reading today's catalog."""
    folder = Path(root) / 'cartao/RPGPOKET'
    base = gzip.decompress((folder / 'artes.pak.gz').read_bytes())
    if base[:4] != b'PKA1':
        raise ValueError('Original asset envelope is not PKA1')
    # Panels intentionally replace the end of the oldest base archive.
    payload = base[16:16 + 6158144] + gzip.decompress((folder / 'paineis1.bin.gz').read_bytes())
    checked_pack(payload, 6772544, 0x1a8982c3, 425)
    payload += gzip.decompress((folder / 'origens1.bin.gz').read_bytes())
    checked_pack(payload, 6846528, 0x19aa0797, 459)
    payload += gzip.decompress(base64.b64decode((folder / 'ilha1.bin.gz.b64').read_text()))
    checked_pack(payload, 6910016, 0x415cb17d, 480)
    payload += gzip.decompress(base64.b64decode((folder / 'mundovivo1.bin.gz.b64').read_text()))
    return checked_pack(payload, 6991936, 0x53dcc3a5, 488)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline-only', action='store_true', help='Emit the retained 488-entry old fixture for transition tests')
    parser.add_argument('--output', type=Path, default=ROOT / 'cartao/RPGPOKET/artes.pak')
    args = parser.parse_args()
    pack = reconstruct_legacy_pack()
    if not args.baseline_only:
        from prepare_enemy_art import build, verify_generated
        result = build(baseline_pack=pack)
        verify_generated(result)
        from prepare_scene_art import build as build_scenes, verify_generated as verify_scenes
        scene_result = build_scenes(result)
        verify_scenes(scene_result)
        pack = scene_result.pack
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(pack)
    _, size, crc, count = struct.unpack('<4sIII', pack[:16])
    print(f'PKA1: {count} assets, {size} payload bytes, CRC32 {crc:08x}')


if __name__ == '__main__':
    main()
