#!/usr/bin/env python3
"""Verify package11 Save30 candidate bytes, including independently downloaded copies."""
from pathlib import Path
import hashlib
import json
import re
import struct
import sys
import zlib

VERSION = '2026.10.14-shop77'
BUILD = 2026101405
PACKAGE = '7.7 - Comercio em lote'
ART_BYTES = 7470912
ART_CRC = 0x9b4bcfc0
ART_COUNT = 666
ART_SHA = '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56'
MINIMUM_PSRAM = 7638272


def verify(root=Path('release'), source=Path('.'), quiet=False):
    header = (source / 'firmware/RPG_POKET_2/ReleaseVersion.h').read_text()
    assert re.search(r'FW_VERSION="([^"]+)"', header).group(1) == VERSION
    assert int(re.search(r'FW_BUILD=(\d+)u', header).group(1)) == BUILD
    assert re.search(r'FW_PACKAGE="([^"]+)"', header).group(1) == PACKAGE
    save_header = (source / 'firmware/RPG_POKET_2/SaveVersion.h').read_text()
    assert int(re.search(r'CURRENT_SAVE_FORMAT=(\d+)', save_header).group(1)) == 30
    assert int(re.search(r'SAVE_RESET_EPOCH=(\d+)', save_header).group(1)) == 1
    rules = (source / 'firmware/RPG_POKET_2/Rules.h').read_text()
    assert re.search(r'RATION_LIMIT\s*=\s*10\b', rules), 'Expected reviewed ration cap 10'
    raw = (root / 'manifest.json').read_bytes()
    assert 0 < len(raw) <= 4096
    m = json.loads(raw)
    for key in ('schema', 'build', 'save_format', 'writes_save_format', 'usb_required_before_save_format',
                'save_reset_epoch', 'minimum_psram_bytes'):
        assert type(m[key]) is int, 'Integer manifest field required: '+key
    for key in ('reads_save_formats', 'reset_game_slots_once'):
        assert type(m[key]) is list and all(type(value) is int for value in m[key]), key
    assert m['version'] == VERSION and m['build'] == BUILD and m['package_name'] == PACKAGE
    assert m['schema'] == 2 and m['board'] == 'Waveshare-29667' and m['partition'] == 'app3M_fat9M_16MB'
    assert m['save_format'] == 7 and m['writes_save_format'] == 30 and m['reads_save_formats'] == [29, 30]
    assert m['installation'] == 'usb-first-then-confirmed-device-ota'
    assert m['usb_required_before_save_format'] == 29 and m['usb_erase_flash'] == 'none'
    assert type(m['ration_limit']) is int and m['ration_limit'] == 10
    assert type(m['historical_reader_ration_limit']) is int and m['historical_reader_ration_limit'] == 9
    assert m['firmware_downgrade_supported'] is False
    assert m['save_reset_epoch'] == 1 and m['reset_game_slots_once'] == [1, 2, 3]
    assert m['preserves'] == ['wifi-settings', 'sd-art-packages']
    base = f'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v{VERSION}/'
    inputs = {}
    for key, name in (('firmware', 'firmware.bin'), ('art', 'artes.pak')):
        data = (root / name).read_bytes()
        assert m[key]['url'] == base + name
        assert type(m[key]['bytes']) is int and len(data) == m[key]['bytes']
        assert hashlib.sha256(data).hexdigest() == m[key]['sha256']
        inputs[key] = data
        if not quiet:
            print(f"PASS: {name} {len(data)} bytes SHA256 {m[key]['sha256']}")
    fw = inputs['firmware']
    assert 24 <= len(fw) <= 0x300000 and fw[0] == 0xe9
    assert struct.unpack_from('<H', fw, 12)[0] == 9, 'Expected ESP32-S3 image'
    assert VERSION.encode() + b'\0' in fw, 'Wrong compiled release identity'
    for marker in (b'shop77-dev', b'package11-dev', b'pvp77-dev', b'package10-dev', b'package9-dev', b'clock77-dev', b'package8-dev', b'story77-dev', b'immersion77-dev', b'package7-dev', b'explore77-dev', b'package6-dev', b'camp77-dev', b'd20-camp77-dev', b'scale77-dev', b'scenes77-dev', b'touch77-dev', b'visual77-combined-dev', b'enemy77-dev'):
        assert marker not in fw, 'Development firmware identity present'
    art = inputs['art']
    assert struct.unpack('<4sIII', art[:16]) == (b'PKA1', ART_BYTES, ART_CRC, ART_COUNT)
    assert len(art) == ART_BYTES + 16 and zlib.crc32(art[16:]) == ART_CRC
    assert type(m['art']['assets']) is int and m['art']['assets'] == ART_COUNT and m['art']['payload_crc32'] == f'{ART_CRC:08x}'
    assert m['art']['sha256'] == ART_SHA, 'Package11 must retain the approved 666-asset pack'
    catalog = (source / 'firmware/RPG_POKET_2/AssetCatalog.h').read_text()
    assert int(re.search(r'ART_BYTES=(\d+)', catalog).group(1)) == ART_BYTES
    assert int(re.search(r'ART_CRC=0x([a-f0-9]+)', catalog).group(1), 16) == ART_CRC
    assert int(re.search(r'ART_COUNT=(\d+)', catalog).group(1)) == ART_COUNT
    decode = (source / 'firmware/RPG_POKET_2/EnemyArt.h').read_text()
    workspace = int(re.search(r'WORKSPACE_BYTES=(\d+)', decode).group(1))
    assert workspace == 13760 and m['minimum_psram_bytes'] == ART_BYTES + 153600 + workspace == MINIMUM_PSRAM
    assert MINIMUM_PSRAM < 8 * 1024 * 1024
    if not quiet:
        print(f'PASS: package11 Save30 reads29/30 and epoch1 contract; ration10/old readers at most9/no firmware downgrade; legacy USB boundary; 666 assets; OTA margin {0x300000-len(fw)}; known PSRAM {MINIMUM_PSRAM} (static estimate, not measured free heap)')
    return m


if __name__ == '__main__':
    assert len(sys.argv) <= 2
    verify(Path(sys.argv[1]) if len(sys.argv) == 2 else Path('release'))
