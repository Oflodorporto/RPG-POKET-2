#!/usr/bin/env python3
"""Exact touch77 refusal and historical Save29/30 -> package11 OTA/read-back.

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
from verify_package11_release import verify, VERSION, BUILD, PACKAGE, MINIMUM_PSRAM

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
PVP_COMMIT = '0541056de5dc94a48b4d750ee0cb4c040df322b1'
PVP_TREE = '0ecf0cbd36379b00963f0794a4ed5c7dd0bc882d'
SAVE30_HISTORY = ('explore77', 'immersion77', 'story77', 'clock77', 'pvp77')
HISTORY = (
    ('touch77', OLD_COMMIT, OLD_TREE, 'RPG_TEST_OLD_TOUCH77'),
    ('scenes77', SCENES_COMMIT, SCENES_TREE, 'RPG_TEST_OLD_SCENES77'),
    ('scale77', SCALE_COMMIT, SCALE_TREE, 'RPG_TEST_OLD_SCALE77'),
    ('camp77', CAMP_COMMIT, CAMP_TREE, 'RPG_TEST_OLD_CAMP77'),
    ('explore77', EXPLORE_COMMIT, EXPLORE_TREE, 'RPG_TEST_OLD_EXPLORE77'),
    ('immersion77', IMMERSION_COMMIT, IMMERSION_TREE, 'RPG_TEST_OLD_IMMERSION77'),
    ('story77', STORY_COMMIT, STORY_TREE, 'RPG_TEST_OLD_STORY77'),
    ('clock77', CLOCK_COMMIT, CLOCK_TREE, 'RPG_TEST_OLD_CLOCK77'),
    ('pvp77', PVP_COMMIT, PVP_TREE, 'RPG_TEST_OLD_PVP77'),
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
             ration_limit=10, historical_reader_ration_limit=9, firmware_downgrade_supported=False,
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


# These literal pins describe independently verified PUBLIC bytes, not the
# earlier qualification build. The PvP publication rebuild has distinct hashes.
REVIEWED_LINEAGE = {'schema': 1,
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
 'clock_qualification_parent_readme_sha256': 'e33ba1c597cbdc82264f6e0323c3187788d06d1169a18743953090d65a3e1fb7',
 'clock77': {'version': '2026.10.14-clock77',
             'build': 2026101403,
             'commit': '1ca079e2a93d8b99a71a03e98d69cb6c201dbb56',
             'tree': 'ef9a3d89df891e9cb74f598d84038650d89c004e',
             'firmware_sha256': '62a1be9204a437991968c8db10bcbecc854021afa427dd1aed7b38958bd95be4',
             'firmware_bytes': 2898656,
             'verified_public': True,
             'manifest_sha256': '3d6ddf04b216218a42795605cb70682b4052abb45199dceccd2b63076cb5afa2',
             'manifest_bytes': 1094,
             'art_sha256': '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56',
             'art_bytes': 7470928,
             'publication_run': 38569168609,
             'firmware_url': 'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v2026.10.14-clock77/firmware.bin'},
 'pvp_qualification_parent': '5300dada9959c85ff3302508940344d50602036b',
 'pvp_qualification_parent_tree': 'a8f043f4acecda081009c96b2a74c473c3d43399',
 'pvp_qualification_parent_readme_sha256': '4bae98284e1756a011f045d500faa8292e758d4cf69054a2a4d3f3c43c05028d',
 'pvp77': {'version': '2026.10.14-pvp77',
           'build': 2026101404,
           'commit': '0541056de5dc94a48b4d750ee0cb4c040df322b1',
           'tree': '0ecf0cbd36379b00963f0794a4ed5c7dd0bc882d',
           'firmware_sha256': 'a1e5eb0b08ab685b0f45f31ed773f07491e1960b853c3ec83fef468aa5a1b2ef',
           'firmware_bytes': 2913664,
           'verified_public': True,
           'manifest_sha256': '7a3be612e6e847e31ab970e8ebe5c145f032a4ba5c638f5240288427c3c0666c',
           'manifest_bytes': 1081,
           'art_sha256': '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56',
           'art_bytes': 7470928,
           'publication_run': 38587457500,
           'firmware_url': 'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v2026.10.14-pvp77/firmware.bin'},
 'pvp_readme_commit': 'dfc68c49f3264d4f2d3642266d44b82ca9b7c563',
 'pvp_readme_commit_tree': '6002164bf57a814e13088c84cc6b45f9be91f228',
 'pvp_readme_commit_readme_sha256': 'b367ad0495e042534e28b706ab526bd2f8e2e3e1ea4f3b1ed333906c6a410357',
 'showcase_commit': '6b04f9443b1dc55fa09259056aaa6217d05c7719',
 'showcase_commit_tree': '58297738fab8db6600b05feccacfcd47cf934987',
 'showcase_commit_readme_sha256': 'f224def52b4816dcd8585c27292168f68bd7b26e386426e5216569d320a32519',
 'qualification_parent': '6a23dda54114db611b818b51e4b1b942019098fa',
 'qualification_parent_tree': 'd75fd4455d677559d15cccaba6cd90861c6acf20',
 'qualification_parent_readme_sha256': 'e6fc33f5bb97bbd0e6931e953bf9858b203da2085ebc1a8c93dfa0af7f2710de'}
QUALIFICATION_PARENT = REVIEWED_LINEAGE['qualification_parent']
QUALIFICATION_PARENT_TREE = REVIEWED_LINEAGE['qualification_parent_tree']
QUALIFICATION_PARENT_README_SHA256 = REVIEWED_LINEAGE['qualification_parent_readme_sha256']
SHOWCASE_PATHS = (
    'README.md', 'docs/ESTADO_ATUAL.md', 'docs/GUIA_RAPIDO.md', 'docs/INDICE.md',
    'docs/MIDIA_README.md', 'docs/historico/README-2026-10-14.md',
    'docs/historico/README-original-2026-10-14.txt', 'docs/media/acampamento-host.png',
    'docs/media/carvalho-host.png', 'docs/media/pvp-duas-perspectivas-host.gif',
    'docs/media/relogio-pulso-host.gif', 'docs/media/ruinas-host.png',
)


def expected_lineage():
    """Return the exact reviewed schema; mutable fixture edits cannot qualify it."""
    return copy.deepcopy(REVIEWED_LINEAGE)


def require_final_lineage(expected):
    """No release-byte qualification before independent public/parent review."""
    for label in SAVE30_HISTORY:
        old = expected[label]
        assert old['verified_public'] is True, 'Unverified public baseline: '+label
        for field in ('firmware_sha256', 'manifest_sha256', 'art_sha256'):
            assert isinstance(old[field], str) and re.fullmatch('[0-9a-f]{64}', old[field]), label+' '+field
        for field in ('firmware_bytes', 'manifest_bytes', 'art_bytes', 'publication_run'):
            assert type(old[field]) is int and old[field] > 0, label+' '+field
    for field, length in (('qualification_parent', 40), ('qualification_parent_tree', 40),
                          ('qualification_parent_readme_sha256', 64)):
        assert isinstance(expected[field], str) and re.fullmatch('[0-9a-f]{'+str(length)+'}', expected[field]), field


def published_lineage(lineage=None):
    """Fail closed on public bytes, full exact ancestry, and reviewed current main."""
    expected = expected_lineage()
    require_final_lineage(expected)
    if lineage is None:
        lineage = json.loads((ROOT / 'tests/fixtures/package11-published-baselines.json').read_text())
    assert json.dumps(lineage, sort_keys=True) == json.dumps(expected, sort_keys=True), 'Reviewed public bytes, field types, or exact qualification lineage changed'
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
    pvp_parent = lineage['pvp_qualification_parent']
    pvp_readme = lineage['pvp_readme_commit']
    showcase = lineage['showcase_commit']
    parent = lineage['qualification_parent']
    for key in ('historical_readme_base', 'immersion_qualification_parent',
                'story_qualification_parent', 'clock_qualification_parent',
                'pvp_qualification_parent', 'pvp_readme_commit', 'showcase_commit', 'qualification_parent'):
        assert git('rev-parse', lineage[key]+'^{tree}').decode().strip() == lineage[key+'_tree']
    for child, ancestor in ((historical, EXPLORE_COMMIT), (reviewed, historical),
                            (IMMERSION_COMMIT, reviewed), (story_parent, IMMERSION_COMMIT),
                            (STORY_COMMIT, story_parent), (clock_parent, STORY_COMMIT),
                            (CLOCK_COMMIT, clock_parent), (pvp_parent, CLOCK_COMMIT),
                            (PVP_COMMIT, pvp_parent), (pvp_readme, PVP_COMMIT),
                            (showcase, pvp_readme), (parent, showcase)):
        assert git('rev-list', '--parents', '-n', '1', child).decode().split() == [child, ancestor], 'Exact published ancestry changed: '+child
    for ancestor, child, digest, changed in (
        (EXPLORE_COMMIT, historical, lineage['historical_readme_sha256'], ('README.md',)),
        (IMMERSION_COMMIT, story_parent, lineage['story_qualification_parent_readme_sha256'], ('README.md',)),
        (STORY_COMMIT, clock_parent, lineage['clock_qualification_parent_readme_sha256'], ('README.md',)),
        (CLOCK_COMMIT, pvp_parent, lineage['pvp_qualification_parent_readme_sha256'], ('README.md',)),
        (PVP_COMMIT, pvp_readme, lineage['pvp_readme_commit_readme_sha256'], ('README.md',)),
        (pvp_readme, showcase, lineage['showcase_commit_readme_sha256'], SHOWCASE_PATHS),
        (showcase, parent, lineage['qualification_parent_readme_sha256'], ('README.md',))):
        assert git('diff', '--name-only', ancestor, child).decode().splitlines() == list(changed), 'Reviewed documentation-only main history changed'
        assert hashlib.sha256(git('show', child+':README.md')).hexdigest() == digest
    assert (ROOT/'README.md').read_bytes() == git('show', parent+':README.md'), 'Qualification README changed from current main'
    if os.environ.get('GITHUB_ACTIONS') == 'true':
        assert git('rev-list', '--parents', '-n', '1', 'HEAD').decode().split() == [git('rev-parse', 'HEAD').decode().strip(), parent], 'Qualification HEAD must be a single child of exact current main'
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
        detect_leaks = os.environ.get('RPG_DETECT_LEAKS', '1')
        assert detect_leaks in ('0', '1'), 'RPG_DETECT_LEAKS must be 0 or 1'
        assert os.environ.get('GITHUB_ACTIONS') != 'true' or detect_leaks == '1', 'LeakSanitizer is mandatory in CI'
        if detect_leaks == '0':
            print('LOCAL LIMIT: LeakSanitizer disabled; ASan/UBSan only, not full release qualification.', flush=True)
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
        env.update(ASAN_OPTIONS='detect_leaks='+detect_leaks+':halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1',
                   LSAN_OPTIONS='exitcode=23', RPG_DETECT_LEAKS=detect_leaks)
    with tempfile.TemporaryDirectory(prefix='package11-release-') as temporary:
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
        for label, revision, _, define in (*HISTORY, ('package11', None, None, None)):
            staged = temp / (label + '-headers')
            closure = headers.stage_headers(staged, revision)
            if label != 'touch77':
                closure |= stage_persistence(staged, revision)
            original = snapshot(staged)
            exe = temp / (label + '-adapter')
            defines = ['-D' + define] if define else []
            subprocess.run([compiler, *flags, *defines, '-I', str(staged), '-I', str(staged / 'fake'),
                            str(ROOT / 'tests/package11_release.cpp'), str(staged / 'fake/cJSON.c'), '-lcrypto', '-o', str(exe)], check=True)
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
            subprocess.run([str(binaries['package11']), str(folder), str(saves), 'upgrade'], check=True, env=env)
            after = snapshot(saves)
            assert all(after[p] == data for p, data in before.items()), 'Original historical fixtures were modified'
            migrated = [p for p in after if p.parts[0] == 'migrated']
            assert len(migrated) == 12, 'Expected nine lazy migrations and three true procedural Save30 records'
            for path in migrated:
                assert struct.unpack_from('<H', after[path], 4)[0] == 30, 'Not an actual Save30 record'
            subprocess.run([str(binaries[label]), str(folder), str(saves), 'rollback'], check=True, env=env)
            assert snapshot(saves) == after, 'Rollback changed fixture bytes'
            print(f'PASS: {label} -> package11 -> exact {label} rollback, all three slots, active legacy floors 0/1, no load-time write/reset, real migrated Save30 rejection', flush=True)
        for label in SAVE30_HISTORY:
            saves = temp / (label + '-saves')
            saves.mkdir()
            (saves / 'preserved').mkdir()
            (saves / 'ration-values').mkdir()
            subprocess.run([str(binaries[label]), str(folder), str(saves), 'upgrade30'], check=True, env=env)
            before = snapshot(saves)
            assert len(before) == 31, 'Expected fifteen historical Save30 records/geometry-RNG witnesses and completed epoch'
            subprocess.run([str(binaries['package11']), str(folder), str(saves), 'upgrade30'], check=True, env=env)
            after = snapshot(saves)
            assert all(after[path] == data for path, data in before.items()), 'Original '+label+' fixtures were modified'
            saved = [path for path in after if path.parts[0] == 'preserved']
            assert len(saved) == 15 and all(struct.unpack_from('<H', after[path], 4)[0] == 30 for path in saved)
            values = [path for path in after if path.parts[0] == 'ration-values']
            assert len(values) == 165 and all(struct.unpack_from('<H', after[path], 4)[0] == 30 for path in values)
            subprocess.run([str(binaries[label]), str(folder), str(saves), 'rollback30'], check=True, env=env)
            assert snapshot(saves) == after, 'Exact '+label+' read-back modified fixture files'
            print(f'PASS: exact {label} -> package11 -> exact {label} read-only Save30 codec/journal; all 3 slots/home/both legacy and generated floors; epoch/seed/run/masks/position/geometry/next RNG unchanged for prior-domain values; rations0..9 readable, rations10 rejected with possible older-journal fallback; no firmware downgrade claim', flush=True)
    inputs = 'explicit synthetic firmware/real-art fixture' if args.synthetic_fixture else 'exact prepared/downloaded release bytes'
    print(f'PASS: touch77 refuses; scenes77/scale77/camp77/explore77/immersion77/story77/clock77/pvp77 accept and install read29,30/write30 with real SHA256; candidate art repair; lazy all-slot migration, historical Save29 rejection and Save30 value-limit probes; rations10 may recover an older <=9 side or block both10 without writes; no firmware downgrade; {inputs}. No publication/device I/O.')


if __name__ == '__main__':
    main()
