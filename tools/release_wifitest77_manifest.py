#!/usr/bin/env python3
"""Package the exact Save30 firmware/art candidate pair. No build, flash, or publication."""
if not __debug__:
    raise RuntimeError("Qualification requires assertions; optimized Python is forbidden")
from pathlib import Path
import hashlib
import json
import os
import re
import struct
import zlib


def main():
    header = Path('firmware/RPG_POKET_2/ReleaseVersion.h').read_text()
    compiled_version = re.search(r'FW_VERSION="([^"]+)"', header).group(1)
    version = os.environ.get('RELEASE_VERSION') or compiled_version
    assert re.fullmatch(r'[a-z0-9][a-z0-9.-]{0,38}', version), 'Versao invalida'
    assert version == compiled_version, 'A versao da Release deve ser igual a ReleaseVersion.h'
    build = int(re.search(r'FW_BUILD=(\d+)u', header).group(1))
    save_header = Path('firmware/RPG_POKET_2/SaveVersion.h').read_text()
    save_format = int(re.search(r'CURRENT_SAVE_FORMAT=(\d+)', save_header).group(1))
    epoch = int(re.search(r'SAVE_RESET_EPOCH=(\d+)', save_header).group(1))
    package = re.search(r'FW_PACKAGE="([^"]+)"', header).group(1)
    assert compiled_version == '2026.10.17-wifitest77' and build == 2026101701
    assert package == '7.7 - Teste Wi-Fi', 'Identidade de pacote nao corresponde ao candidato revisado'
    rules = Path('firmware/RPG_POKET_2/Rules.h').read_text()
    assert re.search(r'RATION_LIMIT\s*=\s*10\b', rules), 'Expected reviewed ration cap 10'
    assert save_format == 30 and epoch == 1, 'Requalificar qualquer nova geracao/reset'
    firmware = Path('build/RPG_POKET_2.ino.bin').read_bytes()
    assert 24 <= len(firmware) <= 0x300000, 'Firmware nao cabe em uma particao OTA'
    assert firmware[0] == 0xe9 and struct.unpack_from('<H', firmware, 12)[0] == 9, 'Imagem ESP32-S3 obrigatoria'
    assert version.encode() + b'\0' in firmware, 'Identidade compilada nao corresponde ao fonte'
    artpath = Path('cartao/RPGPOKET/artes.pak')
    if not artpath.exists():
        from unpack_art import main as unpack
        unpack()
    art = artpath.read_bytes()
    magic, size, crc, count = struct.unpack('<4sIII', art[:16])
    assert magic == b'PKA1' and size == len(art) - 16 and zlib.crc32(art[16:]) == crc
    catalog = Path('firmware/RPG_POKET_2/AssetCatalog.h').read_text()
    assert int(re.search(r'ART_BYTES=(\d+)', catalog).group(1)) == size
    assert int(re.search(r'ART_CRC=0x([a-f0-9]+)', catalog).group(1), 16) == crc
    assert int(re.search(r'ART_COUNT=(\d+)', catalog).group(1)) == count
    decode = Path('firmware/RPG_POKET_2/EnemyArt.h').read_text()
    decode_bytes = int(re.search(r'WORKSPACE_BYTES=(\d+)', decode).group(1))
    minimum_psram = size + 153600 + decode_bytes
    assert minimum_psram < 8 * 1024 * 1024, 'Orcamento conhecido excede PSRAM'
    base = f'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v{version}/'
    manifest = {
        'schema': 2,
        'package_name': package,
        'version': version, 'build': build,
        'board': 'Waveshare-29667', 'partition': 'app3M_fat9M_16MB',
        'firmware': {'url': base + 'firmware.bin', 'bytes': len(firmware), 'sha256': hashlib.sha256(firmware).hexdigest()},
        'art': {'url': base + 'artes.pak', 'bytes': len(art), 'sha256': hashlib.sha256(art).hexdigest(), 'payload_crc32': f'{crc:08x}', 'assets': count},
        'minimum_psram_bytes': minimum_psram,
        'installation': 'usb-first-then-confirmed-device-ota',
        'usb_required_before_save_format': 29,
        'usb_erase_flash': 'none',
        # Legacy OTA protocol marker; actual codec compatibility is below.
        'save_format': 7, 'writes_save_format': save_format, 'reads_save_formats': [29, 30],
        # Save30 layout is unchanged, but old readers reject rations=10.
        'ration_limit': 10, 'historical_reader_ration_limit': 9,
        'firmware_downgrade_supported': False,
        'save_reset_epoch': epoch, 'reset_game_slots_once': [1, 2, 3],
        'preserves': ['wifi-settings', 'sd-art-packages'],
    }
    raw = json.dumps(manifest, indent=2) + '\n'
    assert len(raw.encode()) <= 4096, 'Manifesto excede limite do cliente'
    out = Path('release')
    out.mkdir(exist_ok=True)
    (out / 'firmware.bin').write_bytes(firmware)
    (out / 'artes.pak').write_bytes(art)
    (out / 'manifest.json').write_text(raw)
    print('PASS: exact ESP32-S3 identity/OTA size; paired art CRC/catalog; Save30 reads29/30; epoch1/no new reset; ration10/old readers at most9/no firmware downgrade; USB-first/no full erase; SHA256 generated')


if __name__ == '__main__':
    main()
