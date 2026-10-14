#!/usr/bin/env python3
"""Exact published touch77 refusal and scenes77/scale77/camp77 -> Save30 OTA.

Default input is the exact prepared/downloaded release triple. The explicit
--synthetic-fixture mode tests host adapters only, never a compiled release.
Historical clients, codecs and their full quoted-header closures stay unchanged.
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

sys.dont_write_bytecode = True
import test_release77_ota as headers
from verify_package7_release import verify, VERSION, BUILD, PACKAGE, MINIMUM_PSRAM

ROOT = Path(__file__).resolve().parents[1]
OLD_COMMIT = '91c64a364a7c1c697b9cd91b7e72027fa03f2dee'
OLD_TREE = '1099a6cb0713f22ed805a8ca28f3978f77734727'
SCENES_COMMIT = 'dd626414b62bf6f88638ffe5d542196230c51061'
SCENES_TREE = 'd1d9972abe0e792bd2adbf51f39cd720d354a074'
SCALE_COMMIT = 'ad8acd8b60f8c4c8bf961f87f8300445ad9d6be5'
SCALE_TREE = '004fbb47be6953c1437d4607bcba31ffab235ba6'
CAMP_COMMIT = '7a05617cd1e13843d73de72f970c57a7113af48a'
CAMP_TREE = 'b58a5c80339f1d87728723991efdc8006ff764f8'
EXPLORE_COMMIT = '3f9d3221b99e6b4ba55dc270a2a1d0a9155ea27b'
EXPLORE_TREE = '34a32a69aa98ac7e8ba807a04ca065b438148dec'
HISTORY = (
    ('touch77', OLD_COMMIT, OLD_TREE, 'RPG_TEST_OLD_TOUCH77'),
    ('scenes77', SCENES_COMMIT, SCENES_TREE, 'RPG_TEST_OLD_SCENES77'),
    ('scale77', SCALE_COMMIT, SCALE_TREE, 'RPG_TEST_OLD_SCALE77'),
    ('camp77', CAMP_COMMIT, CAMP_TREE, 'RPG_TEST_OLD_CAMP77'),
    ('explore77', EXPLORE_COMMIT, EXPLORE_TREE, 'RPG_TEST_OLD_EXPLORE77'),
)


def stage_persistence(destination, revision=None):
    pending = ['Persistence.h', 'ResetEpoch.h']
    copied = set()
    while pending:
        name = pending.pop()
        if name in copied:
            continue
        assert '..' not in Path(name).parts
        data = headers.source_bytes('firmware/RPG_POKET_2/' + name, revision)
        out = destination / name
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_bytes(data)
        assert out.read_bytes() == data
        copied.add(name)
        pending.extend(re.findall(r'^#include\s+"([^"]+)"', data.decode(), re.M))
    return copied


def synthetic_fixture(folder):
    """Explicit host fixture; no fallback when compiled firmware is missing."""
    fw = bytearray(10000)
    fw[0] = 0xe9
    struct.pack_into('<H', fw, 12, 9)
    marker = VERSION.encode() + b'\0'
    fw[32:32 + len(marker)] = marker
    art = (ROOT / 'cartao/RPGPOKET/artes.pak').read_bytes()
    m = dict(schema=2, version=VERSION, build=BUILD, package_name=PACKAGE,
             board='Waveshare-29667', partition='app3M_fat9M_16MB', minimum_psram_bytes=MINIMUM_PSRAM,
             installation='usb-first-then-confirmed-device-ota', usb_required_before_save_format=29,
             usb_erase_flash='none', save_format=7, writes_save_format=30, reads_save_formats=[29, 30],
             save_reset_epoch=1, reset_game_slots_once=[1, 2, 3], preserves=['wifi-settings', 'sd-art-packages'])
    base = f'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v{VERSION}/'
    folder.mkdir(parents=True, exist_ok=True)
    for key, name, data in (('firmware', 'firmware.bin', fw), ('art', 'artes.pak', art)):
        (folder / name).write_bytes(data)
        m[key] = dict(url=base + name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
    m['art'].update(assets=666, payload_crc32='9b4bcfc0')
    (folder / 'manifest.json').write_text(json.dumps(m, indent=2) + '\n')


def snapshot(folder):
    return {p.relative_to(folder): p.read_bytes() for p in folder.rglob('*') if p.is_file()}


def published_lineage():
    lineage = json.loads((ROOT / 'tests/fixtures/package7-published-baselines.json').read_text())
    old = lineage['explore77']
    assert lineage['schema'] == 1 and old['verified_public'] is True, 'Verified published explore77 bytes are still required'
    assert old['commit'] == EXPLORE_COMMIT and old['tree'] == EXPLORE_TREE
    assert old['version'] == '2026.10.13-explore77' and old['build'] == 2026101309
    assert re.fullmatch('[0-9a-f]{64}', old['firmware_sha256']) and 24 <= old['firmware_bytes'] <= 0x300000
    parent = lineage['qualification_parent']
    assert isinstance(parent, str) and re.fullmatch('[0-9a-f]{40}', parent), 'Final package6 README-only qualification parent is missing'
    assert subprocess.check_output(['git','diff','--name-only',EXPLORE_COMMIT,parent],cwd=ROOT,text=True).splitlines() == ['README.md'], 'Qualification parent must be package6 README-only housekeeping'
    if os.environ.get('GITHUB_ACTIONS') == 'true':
        assert subprocess.check_output(['git','rev-parse','HEAD^'],cwd=ROOT,text=True).strip() == parent, 'Qualification HEAD changed parent; requalify exact source SHA'
    return lineage


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('release_dir', nargs='?', type=Path, default=ROOT / 'release')
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--synthetic-fixture', action='store_true', help='Host fixture only, never a compiled-release qualification')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    assert shutil.which(compiler), 'C++ compiler unavailable; no OTA/migration gate ran'
    for label, commit, tree, _ in HISTORY:
        assert subprocess.check_output(['git', 'rev-parse', '--verify', commit + '^{commit}'], cwd=ROOT, text=True).strip() == commit, label + ' commit mismatch'
        assert subprocess.check_output(['git', 'rev-parse', '--verify', commit + '^{tree}'], cwd=ROOT, text=True).strip() == tree, label + ' tree mismatch'
    flags = ['-std=c++17', '-O1', '-g', '-UNDEBUG', '-Wno-deprecated-declarations']
    env = dict(os.environ)
    if args.sanitizers:
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
        env.update(ASAN_OPTIONS='detect_leaks=' + os.environ.get('RPG_DETECT_LEAKS', '1'), UBSAN_OPTIONS='halt_on_error=1')
    with tempfile.TemporaryDirectory(prefix='package7-release-') as temporary:
        temp = Path(temporary)
        folder = args.release_dir.resolve()
        if args.synthetic_fixture:
            folder = temp / 'synthetic'
            synthetic_fixture(folder)
            print('HOST FIXTURE MODE: synthetic firmware, real approved art; not firmware compilation or release validation.', flush=True)
        verify(folder, ROOT)
        if not args.synthetic_fixture:
            old = published_lineage()['explore77']
            assert hashlib.sha256((folder/'firmware.bin').read_bytes()).hexdigest() != old['firmware_sha256'], 'Recycled published explore77 firmware bytes'
        binaries = {}
        for label, revision, _, define in (*HISTORY, ('package7', None, None, None)):
            staged = temp / (label + '-headers')
            closure = headers.stage_headers(staged, revision)
            if label != 'touch77':
                closure |= stage_persistence(staged, revision)
            original = snapshot(staged)
            exe = temp / (label + '-adapter')
            defines = ['-D' + define] if define else []
            subprocess.run([compiler, *flags, *defines, '-I', str(staged), '-I', str(staged / 'fake'),
                            str(ROOT / 'tests/package7_release.cpp'), str(staged / 'fake/cJSON.c'), '-lcrypto', '-o', str(exe)], check=True)
            assert snapshot(staged) == original, 'Staged source mutated during compilation'
            for name in closure:
                assert (staged / name).read_bytes() == headers.source_bytes('firmware/RPG_POKET_2/' + name, revision)
            binaries[label] = exe
            print(f'PASS: {label}, {len(closure)} exact production headers and real cJSON unchanged; source {revision or "working tree"}', flush=True)
        subprocess.run([str(binaries['touch77']), str(folder), str(temp), 'upgrade'], check=True, env=env)
        for label in ('scenes77', 'scale77', 'camp77'):
            saves = temp / (label + '-saves')
            saves.mkdir()
            (saves / 'migrated').mkdir()
            subprocess.run([str(binaries[label]), str(folder), str(saves), 'upgrade'], check=True, env=env)
            before = snapshot(saves)
            assert len(before) == 19, 'Expected nine historical records, nine geometry/RNG witnesses, completed epoch'
            subprocess.run([str(binaries['package7']), str(folder), str(saves), 'upgrade'], check=True, env=env)
            after = snapshot(saves)
            assert all(after[p] == data for p, data in before.items()), 'Original historical fixtures were modified'
            migrated = [p for p in after if p.parts[0] == 'migrated']
            assert len(migrated) == 12, 'Expected nine lazy migrations and three true procedural Save30 records'
            for path in migrated:
                assert struct.unpack_from('<H', after[path], 4)[0] == 30, 'Not an actual Save30 record'
            subprocess.run([str(binaries[label]), str(folder), str(saves), 'rollback'], check=True, env=env)
            assert snapshot(saves) == after, 'Rollback changed fixture bytes'
            print(f'PASS: {label} -> package7 -> exact {label} rollback, all three slots, active legacy floors 0/1, no load-time write/reset, real migrated Save30 rejection', flush=True)
        saves = temp / 'explore77-saves'
        saves.mkdir()
        (saves / 'preserved').mkdir()
        subprocess.run([str(binaries['explore77']), str(folder), str(saves), 'upgrade30'], check=True, env=env)
        before = snapshot(saves)
        assert len(before) == 31, 'Expected fifteen historical Save30 records/geometry-RNG witnesses and completed epoch'
        subprocess.run([str(binaries['package7']), str(folder), str(saves), 'upgrade30'], check=True, env=env)
        after = snapshot(saves)
        assert all(after[path] == data for path, data in before.items()), 'Original explore77 fixtures were modified'
        saved = [path for path in after if path.parts[0] == 'preserved']
        assert len(saved) == 15 and all(struct.unpack_from('<H', after[path], 4)[0] == 30 for path in saved)
        subprocess.run([str(binaries['explore77']), str(folder), str(saves), 'rollback30'], check=True, env=env)
        assert snapshot(saves) == after, 'Exact explore77 read-back modified fixture files'
        print('PASS: exact explore77 -> package7 -> exact explore77 Save30 codec/journal; all 3 slots/home/both legacy and generated floors; epoch/seed/run/masks/position/geometry/next RNG unchanged', flush=True)
    inputs = 'explicit synthetic firmware/real-art fixture' if args.synthetic_fixture else 'exact prepared/downloaded release bytes'
    print(f'PASS: touch77 refuses; scenes77/scale77/camp77/explore77 accept and install read29,30/write30 with real SHA256; candidate art repair; lazy all-slot migration and exact historical rollback fail closed; {inputs}. No publication/device I/O.')


if __name__ == '__main__':
    main()
