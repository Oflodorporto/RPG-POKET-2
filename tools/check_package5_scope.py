#!/usr/bin/env python3
"""Clean-clone source/art contract for the independently reviewed package5 set."""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import zlib

ROOT = Path(__file__).resolve().parents[1]
BASELINE = 'ad8acd8b60f8c4c8bf961f87f8300445ad9d6be5'
BASELINE_TREE = '004fbb47be6953c1437d4607bcba31ffab235ba6'
PACKAGING = {
    'firmware/RPG_POKET_2/ReleaseVersion.h', '.github/workflows/release.yml', 'README.md',
    'tests/fixtures/package5-approved-sources.json', 'tools/check_package5_scope.py',
    'tools/test_package5.sh', 'tools/verify_package5_release.py', 'tools/test_package5_release.py',
    'tests/package5_release.cpp', 'tests/package5_release_verifier.py',
    '.github/workflows/package5-check.yml', 'docs/PACKAGE5_RELEASE_QUALIFICATION.md',
    'docs/RELEASE_PACKAGE5_NOTAS.txt',
}


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def main():
    assert git('rev-parse', BASELINE + '^{tree}').decode().strip() == BASELINE_TREE
    manifest = json.loads((ROOT/'tests/fixtures/package5-approved-sources.json').read_text())
    assert manifest['schema'] == 1
    assert manifest['baseline_commit'] == BASELINE and manifest['baseline_tree'] == BASELINE_TREE
    assert set(manifest['packaging_exceptions']) == PACKAGING
    entries = manifest['approved_sources']
    assert len(entries) == 217 and len({e['path'] for e in entries}) == 217
    # The exact reviewed feature inventory is stable; packaging cannot silently
    # bless an additional source change by altering the tracked JSON alone.
    inventory_sha = hashlib.sha256(json.dumps(entries, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    assert inventory_sha == 'c2f08a03e46e3eecf19efce60bf8e1d85cfb2fd79027e565431f66293898c5d7', 'Reviewed feature inventory changed'
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
    assert contract == dict(version='2026.10.13-camp77', build=2026101308,
        package='7.7 - D20, acampamento e bolsa', generation='7.7', save_format=29,
        save_reset_epoch=1, art_assets=666, art_payload_bytes=7470912,
        art_total_bytes=7470928, art_crc32='9b4bcfc0',
        art_sha256='14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56',
        known_psram_bytes=7638272, ota_partition_bytes=3145728)
    header = (ROOT/'firmware/RPG_POKET_2/ReleaseVersion.h').read_text()
    assert re.search(r'FW_VERSION="([^"]+)"', header).group(1) == contract['version']
    assert int(re.search(r'FW_BUILD=(\d+)u', header).group(1)) == contract['build']
    assert re.search(r'FW_PACKAGE="([^"]+)"', header).group(1) == contract['package']
    save = (ROOT/'firmware/RPG_POKET_2/SaveVersion.h').read_text()
    assert 'CURRENT_SAVE_FORMAT=29' in save and 'SAVE_RESET_EPOCH=1' in save
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
        known_psram_bytes=7638272, save29_epoch1_unchanged=True,
        candidate_firmware_bytes=None, device_heap_fps_and_latency_measured=False,
        physical_validation=False, published=False,
        note='Source and known allocation bounds only; linked target size is verified after ESP32 compilation. No physical free-heap/FPS claim.')
    (ROOT/'build').mkdir(exist_ok=True)
    (ROOT/'build/package5-scope-budget.json').write_text(json.dumps(report, indent=2)+'\n')
    print(f'PASS: package5 reviewed217 hashes exact; {protected} inherited files exact; art666/Save29/epoch1 fixed; known PSRAM7638272. No publication/device I/O.')


if __name__ == '__main__':
    main()
