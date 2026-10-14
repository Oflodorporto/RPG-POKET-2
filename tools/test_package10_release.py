#!/usr/bin/env python3
"""Exact touch77 refusal and historical Save29/30 -> package10 OTA/read-back.

Default input is the exact prepared/downloaded release triple. The explicit
--synthetic-fixture mode tests host adapters only, never a compiled release.
Historical clients, codecs and their full quoted-header closures stay unchanged.
"""
from pathlib import Path
import argparse
import copy
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
from verify_package10_release import verify, VERSION, BUILD, PACKAGE, MINIMUM_PSRAM

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
IMMERSION_COMMIT = 'e775b5d1854174b45404ca0cd3305c1e24610861'
IMMERSION_TREE = '13adcc6eb887cd2e0c7004e908128cb3accb5979'
STORY_COMMIT = '9ca94294daf185ac80603e3ba409f0201eef72fb'
STORY_TREE = 'b122c04d68d2f6892e74058a77da5463c784064d'
CLOCK_COMMIT = '1ca079e2a93d8b99a71a03e98d69cb6c201dbb56'
CLOCK_TREE = 'ef9a3d89df891e9cb74f598d84038650d89c004e'
SAVE30_HISTORY = ('explore77', 'immersion77', 'story77', 'clock77')
HISTORY = (
    ('touch77', OLD_COMMIT, OLD_TREE, 'RPG_TEST_OLD_TOUCH77'),
    ('scenes77', SCENES_COMMIT, SCENES_TREE, 'RPG_TEST_OLD_SCENES77'),
    ('scale77', SCALE_COMMIT, SCALE_TREE, 'RPG_TEST_OLD_SCALE77'),
    ('camp77', CAMP_COMMIT, CAMP_TREE, 'RPG_TEST_OLD_CAMP77'),
    ('explore77', EXPLORE_COMMIT, EXPLORE_TREE, 'RPG_TEST_OLD_EXPLORE77'),
    ('immersion77', IMMERSION_COMMIT, IMMERSION_TREE, 'RPG_TEST_OLD_IMMERSION77'),
    ('story77', STORY_COMMIT, STORY_TREE, 'RPG_TEST_OLD_STORY77'),
    ('clock77', CLOCK_COMMIT, CLOCK_TREE, 'RPG_TEST_OLD_CLOCK77'),
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


# Reviewed inherited public pins stay literal. Clock publication and its final
# README-only qualification parent are intentionally unverified until replaced
# together with tests/fixtures/package10-published-baselines.json after review.
INHERITED_LINEAGE = {'schema': 1,
 'explore77': {'version': '2026.10.13-explore77',
               'build': 2026101309,
               'commit': '3f9d3221b99e6b4ba55dc270a2a1d0a9155ea27b',
               'tree': '34a32a69aa98ac7e8ba807a04ca065b438148dec',
               'firmware_sha256': '54a2c13b359dcfce3c32f9be9a8f01d350aab99fedae3350aabf98a6403b7cc3',
               'firmware_bytes': 2857904,
               'verified_public': True,
               'manifest_sha256': '4e4eb9eec4707bee35308f6a827a15f39f88432bcd1d7248e556886a5f912100',
               'manifest_bytes': 1113,
               'art_sha256': '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56',
               'publication_run': 38500805596,
               'firmware_url': 'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v2026.10.13-explore77/firmware.bin',
               'art_bytes': 7470928},
 'historical_readme_base': '02a94ca192b8ef915a452527444f8160b90c26aa',
 'historical_readme_base_tree': '8837911990b23450eb5ed1c4d9afbd0eb9afc374',
 'historical_readme_sha256': '8b3f6e5dc79048d2a690bd3d44c1dc6e0c973614061d3be8b4a2c3ff4f31bc59',
 'immersion77': {'version': '2026.10.14-immersion77',
                 'build': 2026101401,
                 'commit': 'e775b5d1854174b45404ca0cd3305c1e24610861',
                 'tree': '13adcc6eb887cd2e0c7004e908128cb3accb5979',
                 'firmware_sha256': '3c589ddebe0d4b0de00a0daefa4907ca47ce143ca8fbd64bc45501d94ff3b7d7',
                 'firmware_bytes': 2891632,
                 'verified_public': True,
                 'manifest_sha256': 'e46ec7455be349672fcef08420145d61b9c7314d452fcc4f978c869754df4301',
                 'manifest_bytes': 1108,
                 'art_sha256': '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56',
                 'art_bytes': 7470928,
                 'publication_run': 38521825552,
                 'firmware_url': 'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v2026.10.14-immersion77/firmware.bin'},
 'immersion_qualification_parent': 'dcacdce1fe3e36f461a92bfc7378e54e23f96e26',
 'immersion_qualification_parent_tree': '90ecaea82ca8809a27c0d4f09c14123e31edc2f3',
 'story_qualification_parent': '979822bb48ad10180d182df2232e2dcded4f9d66',
 'story_qualification_parent_tree': 'a1e76917fd848b5f2dbc77ad81b18a34c7654257',
 'story_qualification_parent_readme_sha256': 'a67f0bb6133c8cc2c82d4d5a02654a6bb9404ed52ed8e0305e1b9b8a91810be7',
 'story77': {'version': '2026.10.14-story77',
             'build': 2026101402,
             'commit': '9ca94294daf185ac80603e3ba409f0201eef72fb',
             'tree': 'b122c04d68d2f6892e74058a77da5463c784064d',
             'firmware_sha256': 'b52f98101fcd75414be57beb935696f4daeb9b8138de3938a7df72cc0f18dd7b',
             'firmware_bytes': 2898480,
             'verified_public': True,
             'manifest_sha256': 'a4eb17800562f2cdb830c6ae8dd2c6026cbea6f47a9f1c22e1bd4ba7df4c0212',
             'manifest_bytes': 1094,
             'art_sha256': '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56',
             'art_bytes': 7470928,
             'publication_run': 38557877229,
             'firmware_url': 'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v2026.10.14-story77/firmware.bin'},
 'clock_qualification_parent': '8aa3d649fc2bae4e3050d79e02ee4b141971e490',
 'clock_qualification_parent_tree': 'e0a9131d1c78205b6d463b13c049c3fe9e079f63',
 'clock_qualification_parent_readme_sha256': 'e33ba1c597cbdc82264f6e0323c3187788d06d1169a18743953090d65a3e1fb7'}
CLOCK_PUBLIC = {'version': '2026.10.14-clock77', 'build': 2026101403, 'commit': '1ca079e2a93d8b99a71a03e98d69cb6c201dbb56', 'tree': 'ef9a3d89df891e9cb74f598d84038650d89c004e', 'firmware_sha256': '62a1be9204a437991968c8db10bcbecc854021afa427dd1aed7b38958bd95be4', 'firmware_bytes': 2898656, 'verified_public': True, 'manifest_sha256': '3d6ddf04b216218a42795605cb70682b4052abb45199dceccd2b63076cb5afa2', 'manifest_bytes': 1094, 'art_sha256': '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56', 'art_bytes': 7470928, 'publication_run': 38569168609, 'firmware_url': 'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v2026.10.14-clock77/firmware.bin'}
QUALIFICATION_PARENT = '5300dada9959c85ff3302508940344d50602036b'
QUALIFICATION_PARENT_TREE = 'a8f043f4acecda081009c96b2a74c473c3d43399'
QUALIFICATION_PARENT_README_SHA256 = '4bae98284e1756a011f045d500faa8292e758d4cf69054a2a4d3f3c43c05028d'


def expected_lineage():
    """Return the exact reviewed schema; pending pins are not qualification."""
    expected = copy.deepcopy(INHERITED_LINEAGE)
    expected.update(clock77=copy.deepcopy(CLOCK_PUBLIC),
                    qualification_parent=QUALIFICATION_PARENT,
                    qualification_parent_tree=QUALIFICATION_PARENT_TREE,
                    qualification_parent_readme_sha256=QUALIFICATION_PARENT_README_SHA256)
    return expected


def require_final_lineage(expected):
    """No release-byte qualification before independent public/parent review."""
    clock = expected['clock77']
    assert clock['verified_public'] is True, 'PENDING: clock77 public release bytes are not independently verified'
    for field in ('firmware_sha256', 'manifest_sha256'):
        assert isinstance(clock[field], str) and re.fullmatch('[0-9a-f]{64}', clock[field]), 'PENDING: clock77 '+field
    for field in ('firmware_bytes', 'manifest_bytes', 'publication_run'):
        assert type(clock[field]) is int and clock[field] > 0, 'PENDING: clock77 '+field
    for field, length in (('qualification_parent', 40), ('qualification_parent_tree', 40),
                          ('qualification_parent_readme_sha256', 64)):
        value = expected[field]
        assert isinstance(value, str) and re.fullmatch('[0-9a-f]{'+str(length)+'}', value), 'PENDING: final '+field


def published_lineage(lineage=None):
    """Fail closed on every public identity and the exact 6 -> 7 -> 8 -> 9 -> 10 source chain."""
    expected = expected_lineage()
    require_final_lineage(expected)
    if lineage is None:
        lineage = json.loads((ROOT / 'tests/fixtures/package10-published-baselines.json').read_text())
    assert lineage == expected, 'Reviewed public bytes or exact qualification lineage changed'
    def git(*args):
        return subprocess.check_output(['git', *args], cwd=ROOT)
    for label in SAVE30_HISTORY:
        old = lineage[label]
        assert old['verified_public'] is True
        assert old['art_bytes'] == 7470928
        assert old['art_sha256'] == '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56'
        assert old['firmware_url'] == 'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v'+old['version']+'/firmware.bin'
        assert git('rev-parse', old['commit']+'^{tree}').decode().strip() == old['tree']
    historical = lineage['historical_readme_base']
    reviewed = lineage['immersion_qualification_parent']
    story_parent = lineage['story_qualification_parent']
    clock_parent = lineage['clock_qualification_parent']
    parent = lineage['qualification_parent']
    for commit, tree in ((historical, lineage['historical_readme_base_tree']),
                         (reviewed, lineage['immersion_qualification_parent_tree']),
                         (story_parent, lineage['story_qualification_parent_tree']),
                         (clock_parent, lineage['clock_qualification_parent_tree']),
                         (parent, lineage['qualification_parent_tree'])):
        assert git('rev-parse', commit+'^{tree}').decode().strip() == tree
    for child, ancestor in ((historical, EXPLORE_COMMIT), (reviewed, historical),
                            (IMMERSION_COMMIT, reviewed), (story_parent, IMMERSION_COMMIT),
                            (STORY_COMMIT, story_parent), (clock_parent, STORY_COMMIT),
                            (CLOCK_COMMIT, clock_parent), (parent, CLOCK_COMMIT)):
        assert git('rev-list', '--parents', '-n', '1', child).decode().split() == [child, ancestor], 'Exact published ancestry changed: '+child
    for ancestor, child, digest in (
        (EXPLORE_COMMIT, historical, lineage['historical_readme_sha256']),
        (IMMERSION_COMMIT, story_parent, lineage['story_qualification_parent_readme_sha256']),
        (STORY_COMMIT, clock_parent, lineage['clock_qualification_parent_readme_sha256']),
        (CLOCK_COMMIT, parent, lineage['qualification_parent_readme_sha256'])):
        assert git('diff', '--name-only', ancestor, child).decode().splitlines() == ['README.md'], 'Main housekeeping must be README-only'
        assert hashlib.sha256(git('show', child+':README.md')).hexdigest() == digest
    assert (ROOT/'README.md').read_bytes() == git('show', parent+':README.md'), 'Qualification README changed from current main'
    if os.environ.get('GITHUB_ACTIONS') == 'true':
        assert git('rev-list', '--parents', '-n', '1', 'HEAD').decode().split() == [git('rev-parse', 'HEAD').decode().strip(), parent], 'Qualification HEAD changed parent; requalify exact source SHA'
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
        assert os.environ.get('RPG_DETECT_LEAKS', '1') == '1', 'LeakSanitizer is mandatory; RPG_DETECT_LEAKS must be 1'
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
        env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1',
                   LSAN_OPTIONS='exitcode=23', RPG_DETECT_LEAKS='1')
    with tempfile.TemporaryDirectory(prefix='package10-release-') as temporary:
        temp = Path(temporary)
        folder = args.release_dir.resolve()
        if args.synthetic_fixture:
            folder = temp / 'synthetic'
            synthetic_fixture(folder)
            print('HOST FIXTURE MODE: synthetic firmware, real approved art; not firmware compilation or release validation.', flush=True)
        verify(folder, ROOT)
        if not args.synthetic_fixture:
            lineage = published_lineage()
            digest = hashlib.sha256((folder/'firmware.bin').read_bytes()).hexdigest()
            for label in SAVE30_HISTORY:
                assert digest != lineage[label]['firmware_sha256'], 'Recycled published '+label+' firmware bytes'
        binaries = {}
        for label, revision, _, define in (*HISTORY, ('package10', None, None, None)):
            staged = temp / (label + '-headers')
            closure = headers.stage_headers(staged, revision)
            if label != 'touch77':
                closure |= stage_persistence(staged, revision)
            original = snapshot(staged)
            exe = temp / (label + '-adapter')
            defines = ['-D' + define] if define else []
            subprocess.run([compiler, *flags, *defines, '-I', str(staged), '-I', str(staged / 'fake'),
                            str(ROOT / 'tests/package10_release.cpp'), str(staged / 'fake/cJSON.c'), '-lcrypto', '-o', str(exe)], check=True)
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
            subprocess.run([str(binaries['package10']), str(folder), str(saves), 'upgrade'], check=True, env=env)
            after = snapshot(saves)
            assert all(after[p] == data for p, data in before.items()), 'Original historical fixtures were modified'
            migrated = [p for p in after if p.parts[0] == 'migrated']
            assert len(migrated) == 12, 'Expected nine lazy migrations and three true procedural Save30 records'
            for path in migrated:
                assert struct.unpack_from('<H', after[path], 4)[0] == 30, 'Not an actual Save30 record'
            subprocess.run([str(binaries[label]), str(folder), str(saves), 'rollback'], check=True, env=env)
            assert snapshot(saves) == after, 'Rollback changed fixture bytes'
            print(f'PASS: {label} -> package10 -> exact {label} rollback, all three slots, active legacy floors 0/1, no load-time write/reset, real migrated Save30 rejection', flush=True)
        for label in SAVE30_HISTORY:
            saves = temp / (label + '-saves')
            saves.mkdir()
            (saves / 'preserved').mkdir()
            subprocess.run([str(binaries[label]), str(folder), str(saves), 'upgrade30'], check=True, env=env)
            before = snapshot(saves)
            assert len(before) == 31, 'Expected fifteen historical Save30 records/geometry-RNG witnesses and completed epoch'
            subprocess.run([str(binaries['package10']), str(folder), str(saves), 'upgrade30'], check=True, env=env)
            after = snapshot(saves)
            assert all(after[path] == data for path, data in before.items()), 'Original '+label+' fixtures were modified'
            saved = [path for path in after if path.parts[0] == 'preserved']
            assert len(saved) == 15 and all(struct.unpack_from('<H', after[path], 4)[0] == 30 for path in saved)
            subprocess.run([str(binaries[label]), str(folder), str(saves), 'rollback30'], check=True, env=env)
            assert snapshot(saves) == after, 'Exact '+label+' read-back modified fixture files'
            print(f'PASS: exact {label} -> package10 -> exact {label} read-only Save30 codec/journal; all 3 slots/home/both legacy and generated floors; epoch/seed/run/masks/position/geometry/next RNG unchanged; no firmware downgrade claim', flush=True)
    inputs = 'explicit synthetic firmware/real-art fixture' if args.synthetic_fixture else 'exact prepared/downloaded release bytes'
    print(f'PASS: touch77 refuses; scenes77/scale77/camp77/explore77/immersion77/story77/clock77 accept and install read29,30/write30 with real SHA256; candidate art repair; lazy all-slot migration and exact historical rollback fail closed; {inputs}. No publication/device I/O.')


if __name__ == '__main__':
    main()
