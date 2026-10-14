#!/usr/bin/env python3
"""Clean-clone source/art contract for the independently reviewed package6 set."""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import zlib

ROOT = Path(__file__).resolve().parents[1]
BASELINE = '7a05617cd1e13843d73de72f970c57a7113af48a'
BASELINE_TREE = 'b58a5c80339f1d87728723991efdc8006ff764f8'
PACKAGING = {'tools/record_package6_identity.py', 'tests/fixtures/package6-procedural-only/firmware/RPG_POKET_2/Save.h', 'tests/fixtures/package6-procedural-only/firmware/RPG_POKET_2/Campaign.h', '.github/workflows/package6-check.yml', 'tests/fixtures/package6-procedural-only/firmware/RPG_POKET_2/Exploration.h', 'tools/check_package6_scope.py', 'tests/package6_save_review/geometry.inc', 'tests/package6_save_review/nvs_rollback29.cpp', 'tests/fixtures/package6-procedural-only/firmware/RPG_POKET_2/DailyTravel.h', 'firmware/RPG_POKET_2/ReleaseVersion.h', 'docs/RELEASE_PACKAGE6_NOTAS.txt', 'tests/fixtures/package6-procedural-only/firmware/RPG_POKET_2/Dungeon.h', 'tests/package6_save_review/nvs_migrate30.cpp', 'README.md', 'tests/fixtures/package6-procedural-only/firmware/RPG_POKET_2/GuildEvents.h', 'tools/verify_package6_release.py', 'tests/package6_release_gate.py', 'tools/release_package6_manifest.py', 'tools/test_package6_save_review.py', 'tests/package6_save_review/nvs_fake.inc', 'tools/test_package6.sh', 'tests/package6_save_review/generate29.cpp', 'tests/package6_save_review/rollback29.cpp', 'tests/fixtures/package6-procedural-only/firmware/RPG_POKET_2/Camp.h', 'tools/check_package6_release_gate.py', 'tests/fixtures/package6-procedural-only/firmware/RPG_POKET_2/Rules.h', 'tests/package6_release_verifier.py', 'tools/test_package6_release.py', '.github/workflows/release.yml', 'tests/fixtures/package6-procedural-only/firmware/RPG_POKET_2/Gear.h', 'tests/fixtures/package6-approved-sources.json', 'tests/fixtures/package6-procedural-only/firmware/RPG_POKET_2/Persistence.h', 'tests/fixtures/package6-procedural-only/firmware/RPG_POKET_2/DungeonGeneration.h', 'docs/PACKAGE6_RELEASE_QUALIFICATION.md', 'tests/fixtures/package6-procedural-only/firmware/RPG_POKET_2/SaveVersion.h', 'PACKAGE6_STAGING_ONLY.txt', 'tests/package6_release.cpp', 'tests/package6_save_review/migrate30.cpp'}


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def main():
    assert git('rev-parse', BASELINE + '^{tree}').decode().strip() == BASELINE_TREE
    manifest = json.loads((ROOT/'tests/fixtures/package6-approved-sources.json').read_text())
    assert manifest['schema'] == 1
    assert manifest['baseline_commit'] == BASELINE and manifest['baseline_tree'] == BASELINE_TREE
    assert set(manifest['packaging_exceptions']) == PACKAGING
    entries = manifest['approved_sources']
    assert len(entries) == 51 and len({e['path'] for e in entries}) == 51
    # The exact reviewed feature inventory is stable; packaging cannot silently
    # bless an additional source change by altering the tracked JSON alone.
    inventory_sha = hashlib.sha256(json.dumps(entries, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    assert inventory_sha == 'b92cd21a6d7ea7834e0dc9371ec4089ba7be33a8058b1611f6e5f6f602db3e0d', 'Reviewed feature inventory changed'
    assert manifest['frozen_feature_tree'] == '814f16a9f6e11a2d5fe0cf6a32f7da67c92bbb17'
    fixtures = manifest['qualification_inputs']
    assert hashlib.sha256(json.dumps(fixtures,sort_keys=True,separators=(',',':')).encode()).hexdigest() == '40a6d23820c9dac42ef3a7ce468fb31b8f9973c5ca87baa9da6b0a53ddcc9876', 'Frozen qualification fixtures changed'
    for entry in fixtures:
        path = ROOT / entry['path']
        assert not path.is_symlink() and path.is_file()
        data = path.read_bytes()
        assert len(data) == entry['bytes'] and hashlib.sha256(data).hexdigest() == entry['sha256'], entry['path']
    approved = {e['path'] for e in entries}
    assert not approved & PACKAGING
    for entry in entries:
        name = entry['path']
        assert not Path(name).is_absolute() and '..' not in Path(name).parts
        path = ROOT/name
        assert not path.is_symlink(), name
        data = path.read_bytes()
        assert len(data) == entry['bytes'] and hashlib.sha256(data).hexdigest() == entry['sha256'], name
    baseline_files = {}
    for line in git('ls-tree', '-r', BASELINE).decode().splitlines():
        metadata, name = line.split('\t', 1)
        mode, kind, blob = metadata.split()
        assert kind == 'blob'
        baseline_files[name] = (mode, blob)
    protected = 0
    for name, (mode, blob) in baseline_files.items():
        if name in approved | PACKAGING:
            continue
        path = ROOT/name
        assert path.is_file() and not path.is_symlink(), name
        data = path.read_bytes()
        actual = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        assert actual == blob, 'Protected baseline file changed: ' + name
        assert bool(path.stat().st_mode & 0o111) == (mode == '100755'), 'Mode changed: ' + name
        protected += 1
    actual_files = set(git('ls-files', '--cached', '--others', '--exclude-standard', '-z').decode().split('\0')) - {''}
    assert not (actual_files - set(baseline_files) - approved - PACKAGING), 'Unexpected files: ' + repr(sorted(actual_files - set(baseline_files) - approved - PACKAGING))
    # Do not let an ignore pattern hide an unreviewed production source.
    firmware_files = {str(p.relative_to(ROOT)) for p in (ROOT/'firmware').rglob('*') if p.is_file()}
    assert firmware_files <= set(baseline_files) | approved | PACKAGING
    for name in PACKAGING:
        assert (ROOT/name).is_file() and not (ROOT/name).is_symlink(), 'Missing packaging input: ' + name
    contract = manifest['contract']
    assert contract == {'version': '2026.10.13-explore77', 'build': 2026101309, 'package': '7.7 - Dados e exploracao das Ruinas', 'generation': '7.7', 'save_format': 30, 'reads_save_formats': [29, 30], 'save_reset_epoch': 1, 'art_assets': 666, 'art_payload_bytes': 7470912, 'art_total_bytes': 7470928, 'art_crc32': '9b4bcfc0', 'art_sha256': '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56', 'known_psram_bytes': 7638272, 'ota_partition_bytes': 3145728}
    header = (ROOT/'firmware/RPG_POKET_2/ReleaseVersion.h').read_text()
    assert re.search(r'FW_VERSION="([^"]+)"', header).group(1) == contract['version']
    assert int(re.search(r'FW_BUILD=(\d+)u', header).group(1)) == contract['build']
    assert re.search(r'FW_PACKAGE="([^"]+)"', header).group(1) == contract['package']
    save = (ROOT/'firmware/RPG_POKET_2/SaveVersion.h').read_text()
    assert 'CURRENT_SAVE_FORMAT=30' in save and 'SAVE_RESET_EPOCH=1' in save
    pack = (ROOT/'cartao/RPGPOKET/artes.pak').read_bytes()
    assert struct.unpack('<4sIII', pack[:16]) == (b'PKA1', 7470912, 0x9b4bcfc0, 666)
    assert len(pack) == 7470928 and zlib.crc32(pack[16:]) == 0x9b4bcfc0
    assert hashlib.sha256(pack).hexdigest() == contract['art_sha256']
    for name in ('D20View.h', 'TentMarker.h', 'CampView.h'):
        source = (ROOT/'firmware/RPG_POKET_2'/name).read_text()
        assert not re.search(r'\b(malloc|calloc|realloc|new|random|Preferences|SD)\s*[(<]', source), name
    report = dict(schema=1, source_commit=git('rev-parse', 'HEAD').decode().strip(),
        source_tree=git('rev-parse', 'HEAD^{tree}').decode().strip(),
        working_tree_clean=not bool(git('status', '--porcelain').strip()),
        baseline_commit=BASELINE, baseline_tree=BASELINE_TREE,
        approved_feature_files=len(entries), approved_inventory_sha256=inventory_sha,
        protected_files_byte_identical=protected, packaging_exceptions=sorted(PACKAGING),
        contract=contract, new_art_payload_bytes=0, new_psram_buffers=0,
        per_frame_render_heap_allocations=0, tent_const_flash_bytes=1080,
        d20_primary_stack_arrays_bytes=168, ruins_bag_stack_row_bytes=176,
        framebuffer_bytes=153600, decode_workspace_bytes=13760,
        known_psram_bytes=7638272, save_format=30, reads_save_formats=[29,30], save_reset_epoch=1, new_reset=False, legacy_active_geometry_preserved=True, procedural_cache_host_bytes=198,
        candidate_firmware_bytes=None, device_heap_fps_and_latency_measured=False,
        physical_validation=False, published=False,
        note='Source and known allocation bounds only; linked target size is verified after ESP32 compilation. No physical free-heap/FPS claim.')
    (ROOT/'build').mkdir(exist_ok=True)
    (ROOT/'build/package6-scope-budget.json').write_text(json.dumps(report, indent=2)+'\n')
    print(f'PASS: package6 reviewed51 hashes exact; {protected} inherited files exact; art666/Save30 reads29,30/epoch1 fixed; known PSRAM7638272. No publication/device I/O.')


if __name__ == '__main__':
    main()
