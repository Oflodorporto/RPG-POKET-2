#!/usr/bin/env python3
"""Test 7.6 -> final 7.7 OTA using unchanged version-specific production headers.

Default input: release/{manifest.json,firmware.bin,artes.pak}. An explicit input
folder can contain the three downloaded release files. No network or device I/O.
--synthetic-fixture is an explicit host-only fallback and never validates a real
compiled firmware. The ordinary mode requires all three prepared/downloaded files.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
OLD_COMMIT = '26a43145a8cd7e45e8716b1f1a34b358c41d47f3'
FINAL_VERSION = '2026.10.12-visual77'
FINAL_BUILD = 2026101208
ART_BYTES = 6661440
ART_CRC = 0x0b84d93f
ART_COUNT = 636
ART_SHA = '5416f6d507601bc131751e89106332f291f8d1d09642faa264a9b61215dd43f1'
OLD_ART_SHA = 'bf8c7a085791805b3f9a72ab4f9f6877b9c46a0cb842e25f073029d053a524f8'
MINIMUM_PSRAM = 6828800


def require(condition, message):
    if not condition:
        raise ValueError(message)


def source_bytes(path, revision=None):
    if revision:
        result = subprocess.run(['git', 'show', f'{revision}:{path}'], cwd=ROOT,
                                check=True, capture_output=True)
        return result.stdout
    return (ROOT / path).read_bytes()


def stage_headers(destination, revision=None):
    """Copy the entire quoted-include closure without rewriting any header."""
    pending = ['Save.h', 'AssetCatalog.h', 'UpdateDevice.h']
    copied = set()
    while pending:
        name = pending.pop()
        if name in copied:
            continue
        require('..' not in Path(name).parts, f'Unexpected parent include: {name}')
        content = source_bytes('firmware/RPG_POKET_2/' + name, revision)
        target = destination / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content)
        require(target.read_bytes() == content, f'Changed staged header: {name}')
        copied.add(name)
        pending.extend(re.findall(r'^#include\s+"([^"]+)"', content.decode(), re.M))
    # The real cJSON parser also comes from this version, rather than a stub.
    fake = destination / 'fake'
    fake.mkdir()
    for name in ('cJSON.c', 'cJSON.h'):
        content = source_bytes('tests/update_fake/' + name, revision)
        (fake / name).write_bytes(content)
    # Reuse the existing network/SD/OTA/RTOS/test-digest adapters unchanged.
    for name in ('FakeDevice.h', 'HTTPClient.h', 'NetworkClientSecure.h',
                 'esp_ota_ops.h', 'mbedtls/sha256.h'):
        target = fake / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / 'tests/update_fake' / name, target)
        require(target.read_bytes() == (ROOT / 'tests/update_fake' / name).read_bytes(),
                f'Changed fake adapter: {name}')
    return copied


def source_identity(version, build):
    header = (ROOT / 'firmware/RPG_POKET_2/ReleaseVersion.h').read_text()
    require(re.search(r'FW_VERSION="([^"]+)"', header).group(1) == version,
            'Working-tree firmware version is not the expected final identity')
    require(int(re.search(r'FW_BUILD=(\d+)u', header).group(1)) == build,
            'Working-tree firmware build is not the expected final identity')
    return re.search(r'FW_PACKAGE="([^"]+)"', header).group(1)


def fixture(folder, version, build, package):
    """Explicit synthetic image, never a fallback for a missing release artifact."""
    from prepare_enemy_art import build as build_art, verify_generated
    result = build_art()
    verify_generated(result)
    firmware = bytearray((i * 7) % 256 for i in range(10000))
    firmware[:24] = bytes(24)
    firmware[0] = 0xe9
    struct.pack_into('<H', firmware, 12, 9)
    marker = version.encode() + b'\0'
    firmware[64:64 + len(marker)] = marker
    base = f'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v{version}/'
    manifest = dict(schema=2, package_name=package, version=version, build=build,
                    board='Waveshare-29667', partition='app3M_fat9M_16MB',
                    minimum_psram_bytes=MINIMUM_PSRAM, installation='confirmed-device-ota',
                    save_format=7, writes_save_format=28, reads_save_formats=list(range(1, 29)))
    for key, name, data in (('firmware', 'firmware.bin', firmware),
                            ('art', 'artes.pak', result.pack)):
        (folder / name).write_bytes(data)
        manifest[key] = dict(url=base + name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
    manifest['art'].update(assets=ART_COUNT, payload_crc32=f'{ART_CRC:08x}')
    (folder / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')


def verify_files(folder, version, build, package):
    raw = (folder / 'manifest.json').read_bytes()
    require(0 < len(raw) <= 4096, 'Manifest exceeds the real 4096-byte parser input limit')
    manifest = json.loads(raw)
    require(manifest['version'] == version and manifest['build'] == build,
            'Manifest is not the expected final version/build')
    require(manifest['package_name'] == package, 'Manifest package label differs from firmware')
    require(manifest['schema'] == 2 and manifest['board'] == 'Waveshare-29667'
            and manifest['partition'] == 'app3M_fat9M_16MB', 'Wrong schema/board/partition')
    require(manifest['save_format'] == 7 and manifest['writes_save_format'] == 28
            and manifest['reads_save_formats'] == list(range(1, 29)), 'Save28/1..28 contract changed')
    require(manifest['installation'] == 'confirmed-device-ota', 'Wrong installation mode')
    require(manifest['minimum_psram_bytes'] == MINIMUM_PSRAM, 'Wrong full PSRAM budget')
    require(manifest['art']['assets'] == ART_COUNT
            and manifest['art']['payload_crc32'] == f'{ART_CRC:08x}'
            and manifest['art']['sha256'] == ART_SHA, 'Wrong final 636-asset identity')
    base = f'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v{version}/'
    inputs = {}
    for key, name in (('firmware', 'firmware.bin'), ('art', 'artes.pak')):
        data = (folder / name).read_bytes()
        require(manifest[key]['url'] == base + name, f'Wrong final URL for {name}')
        require(len(data) == manifest[key]['bytes'], f'Size mismatch for {name}')
        actual = hashlib.sha256(data).hexdigest()
        require(actual == manifest[key]['sha256'], f'Real SHA-256 mismatch for {name}')
        inputs[key] = data
        print(f'PASS: input {name}, {len(data)} bytes, real SHA256 {actual}', flush=True)
    firmware = inputs['firmware']
    require(24 <= len(firmware) <= 0x300000 and firmware[0] == 0xe9
            and struct.unpack_from('<H', firmware, 12)[0] == 9, 'Expected OTA-size ESP32-S3 image header')
    require(version.encode() + b'\0' in firmware, 'Final version marker missing from firmware bytes')
    for dev in (b'2026.10.12-visual77-combined-dev', b'2026.10.12-visual77-dev',
                b'2026.10.12-enemy77-dev'):
        require(dev not in firmware, 'Development identity remains in firmware')
    art = inputs['art']
    require(len(art) == ART_BYTES + 16, 'Wrong final art length')
    require(struct.unpack('<4sIII', art[:16]) == (b'PKA1', ART_BYTES, ART_CRC, ART_COUNT),
            'Wrong final PKA1 header')
    require(zlib.crc32(art[16:]) == ART_CRC, 'Final art payload CRC mismatch')
    require(len(art) + 153600 < MINIMUM_PSRAM < 8 * 1048576, 'PSRAM budget cannot satisfy OTA parser')
    decode = (ROOT / 'firmware/RPG_POKET_2/EnemyArt.h').read_text()
    workspace = int(re.search(r'WORKSPACE_BYTES=(\d+)', decode).group(1))
    require(ART_BYTES + 153600 + workspace == MINIMUM_PSRAM, 'Full decode PSRAM budget changed')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('release_dir', nargs='?', type=Path, default=ROOT / 'release')
    parser.add_argument('--expected-version', default=FINAL_VERSION,
                        help='Strict manifest and firmware source identity gate (default: final 7.7)')
    parser.add_argument('--expected-build', type=int, default=FINAL_BUILD,
                        help='Strict manifest and firmware source build gate (default: final 7.7)')
    parser.add_argument('--sanitizers', action='store_true', help='Use ASan/UBSan with the existing host adapters')
    parser.add_argument('--synthetic-fixture', action='store_true',
                        help='Explicit host-only ESP32-header fixture; does not validate a compiled binary')
    args = parser.parse_args()
    require(args.expected_version == FINAL_VERSION and args.expected_build == FINAL_BUILD,
            'This final-7.7 transition gate only supports 2026.10.12-visual77 / 2026101208')
    package = source_identity(args.expected_version, args.expected_build)
    compiler = os.environ.get('CXX', 'g++')
    require(shutil.which(compiler), f'C++ compiler {compiler} unavailable; no transition gate ran')
    baseline = subprocess.run(['git', 'cat-file', '-e', OLD_COMMIT + '^{commit}'],
                              cwd=ROOT, capture_output=True)
    require(baseline.returncode == 0,
            f'Exact 7.6 commit {OLD_COMMIT} unavailable; checkout history must contain it')
    from unpack_art import reconstruct_legacy_pack
    old = reconstruct_legacy_pack()
    require(hashlib.sha256(old).hexdigest() == OLD_ART_SHA, 'Retained 7.6 pack SHA-256 mismatch')
    with tempfile.TemporaryDirectory(prefix='release77-ota-') as temporary:
        temporary = Path(temporary)
        if args.synthetic_fixture:
            folder = temporary / 'synthetic-release'
            folder.mkdir()
            fixture(folder, args.expected_version, args.expected_build, package)
            print('HOST FIXTURE MODE: synthetic 10000-byte firmware + real final art; no compiled/released binary claim.', flush=True)
        else:
            folder = args.release_dir.resolve()
            print(f'RELEASE INPUT MODE: verifying exact prepared/downloaded bytes in {folder}', flush=True)
        verify_files(folder, args.expected_version, args.expected_build, package)
        (temporary / 'old.pak').write_bytes(old)
        for label, revision in (('old', OLD_COMMIT), ('new', None)):
            staged = temporary / label
            copied = stage_headers(staged, revision)
            executable = temporary / ('release77_ota_' + label)
            flags = ['-std=c++17', '-O1', '-g', '-UNDEBUG']
            if args.sanitizers:
                flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
            command = [compiler, *flags, '-I', str(staged), '-I', str(staged / 'fake'),
                       str(ROOT / 'tests/release77_ota.cpp'), str(staged / 'fake/cJSON.c'),
                       '-o', str(executable)]
            subprocess.run(command, check=True)
            env = dict(os.environ)
            if args.sanitizers:
                env['ASAN_OPTIONS'] = env.get('ASAN_OPTIONS', '') + ':detect_leaks=' + env.get('RPG_DETECT_LEAKS', '0')
                env['UBSAN_OPTIONS'] = 'halt_on_error=1'
            subprocess.run([str(executable), label, str(folder / 'manifest.json'),
                            str(folder / 'firmware.bin'), str(folder / 'artes.pak'), str(temporary / 'old.pak'),
                            args.expected_version, str(args.expected_build), str(MINIMUM_PSRAM)],
                           check=True, env=env)
            print(f'PASS: {label} staged {len(copied)} production headers and real cJSON unchanged; source {revision or "working tree"}', flush=True)
    print('LIMITS: real SHA-256 checked in Python; only installation-plan hashes use the existing fake mbedTLS digest. '
          'SD/network/OTA/RTOS are host adapters. No actual flash, TLS, hardware power-loss, boot or runtime-memory claim.')
    if args.synthetic_fixture:
        print('RESULT: focused synthetic-firmware transition cases passed; rerun without --synthetic-fixture on the real final three files.')
    else:
        print('RESULT: focused 7.6 -> final 7.7 transition and same-identity art repair passed for the supplied three files.')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError, KeyError) as error:
        raise SystemExit(f'FAIL: {error}')
