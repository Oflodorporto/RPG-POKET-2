#!/usr/bin/env python3
"""Verify the complete frozen integration inventory, packaging scope and budgets."""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import zlib

ROOT = Path(__file__).resolve().parents[1]
BASELINE = '3f9d3221b99e6b4ba55dc270a2a1d0a9155ea27b'
BASELINE_TREE = '34a32a69aa98ac7e8ba807a04ca065b438148dec'
FROZEN_TREE = 'e877f7c08a0f0333e398dfec08f37279825e2c2f'
INVENTORY_SHA = 'fc025ebadd2798357365f59549a14bbfaf9d620557da07def9d1ecc14d76efac'
PACKAGING = {
 '.github/workflows/package7-check.yml', '.github/workflows/release.yml',
 'firmware/RPG_POKET_2/ReleaseVersion.h', 'README.md', 'PACKAGE7_STAGING_ONLY.txt',
 'docs/PACKAGE7_RELEASE_QUALIFICATION.md', 'docs/RELEASE_PACKAGE7_NOTAS.txt',
 'tests/fixtures/package7-approved-sources.json', 'tests/fixtures/package7-published-baselines.json',
 'tests/package7_release.cpp', 'tests/package7_release_gate.py', 'tests/package7_release_verifier.py',
 'tests/package7_qualification_config.py', 'tools/check_package7_release_gate.py',
 'tools/check_package7_scope.py', 'tools/record_package7_identity.py',
 'tools/release_package7_manifest.py', 'tools/test_package7.sh',
 'tools/test_package7_release.py', 'tools/verify_package7_release.py',
}
GENERATED = {'release/firmware.bin', 'release/artes.pak', 'release/manifest.json', 'release/SHA256SUMS'}
CONTRACT = {'version': '2026.10.14-immersion77', 'build': 2026101401, 'package': '7.7 - Imersao nas Ruinas', 'generation': '7.7', 'save_format': 30, 'reads_save_formats': [29, 30], 'save_reset_epoch': 1, 'art_assets': 666, 'art_payload_bytes': 7470912, 'art_total_bytes': 7470928, 'art_crc32': '9b4bcfc0', 'art_sha256': '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56', 'known_psram_bytes': 7638272, 'ota_partition_bytes': 3145728, 'new_atlas_flash_bytes': 18144, 'new_psram_allocations': 0}


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def assert_generated_inventory(root, tracked):
    assert not (set(tracked) & GENERATED), 'Generated release outputs must never be tracked: '+repr(sorted(set(tracked) & GENERATED))
    for name in GENERATED:
        path = root/name
        assert not path.parent.is_symlink(), 'Generated output directory is a symlink: '+name
        if path.exists() or path.is_symlink():
            assert path.is_file() and not path.is_symlink(), 'Generated output must be a regular non-symlink file: '+name


def main():
    assert git('rev-parse', BASELINE+'^{tree}').decode().strip() == BASELINE_TREE
    frozen = json.loads((ROOT/'tests/fixtures/package7-approved-sources.json').read_text())
    assert frozen['schema'] == 1 and frozen['baseline_commit'] == BASELINE and frozen['baseline_tree'] == BASELINE_TREE
    assert frozen['frozen_feature_tree'] == FROZEN_TREE
    assert frozen['frozen_source_manifest_sha256'] == '3b18e11d5c001f5f4e6a405eea7a71de25ab6fbf2de091171f25d91f40c31cb2'
    assert set(frozen['packaging_exceptions']) == PACKAGING and frozen['contract'] == CONTRACT
    entries = frozen['frozen_files']
    assert len(entries) == 871 and len({entry['path'] for entry in entries}) == 871
    assert hashlib.sha256(json.dumps(entries, sort_keys=True, separators=(',', ':')).encode()).hexdigest() == INVENTORY_SHA, 'Reviewed frozen inventory changed'
    for entry in entries:
        name = entry['path']
        assert not Path(name).is_absolute() and '..' not in Path(name).parts
        if name in PACKAGING:
            continue
        path = ROOT/name
        assert path.is_file() and not path.is_symlink(), name
        data = path.read_bytes()
        assert len(data) == entry['bytes'] and hashlib.sha256(data).hexdigest() == entry['sha256'], 'Frozen source changed: '+name
        assert bool(path.stat().st_mode & 0o111) == entry['executable'], 'Mode changed: '+name
    expected = {entry['path'] for entry in entries} | PACKAGING
    tracked = set(git('ls-files', '--cached', '-z').decode().split('\0')) - {''}
    assert_generated_inventory(ROOT, tracked)
    actual = set(git('ls-files', '--cached', '--others', '--exclude-standard', '-z').decode().split('\0')) - {''}
    assert not (actual-expected-GENERATED), 'Unexpected source files: '+repr(sorted(actual-expected-GENERATED))
    assert expected <= actual, 'Missing tracked/source inputs: '+repr(sorted(expected-actual))
    for name in PACKAGING:
        assert (ROOT/name).is_file() and not (ROOT/name).is_symlink(), name
    production = {str(path.relative_to(ROOT)) for path in (ROOT/'firmware').rglob('*') if path.is_file()}
    assert production <= expected, 'Ignored production input outside frozen inventory'
    header = (ROOT/'firmware/RPG_POKET_2/ReleaseVersion.h').read_text()
    assert re.search(r'FW_VERSION="([^"]+)"', header).group(1) == CONTRACT['version']
    assert int(re.search(r'FW_BUILD=(\d+)u', header).group(1)) == CONTRACT['build']
    assert re.search(r'FW_PACKAGE="([^"]+)"', header).group(1) == CONTRACT['package']
    for name in ('Save.h', 'Persistence.h', 'SaveVersion.h', 'ResetEpoch.h', 'AssetCatalog.h', 'AssetStore.h', 'EnemyArt.h', 'Dungeon.h', 'DungeonGeneration.h', 'UpdateDevice.h'):
        path = 'firmware/RPG_POKET_2/'+name
        assert (ROOT/path).read_bytes() == git('show', BASELINE+':'+path), 'Unchanged Save30/epoch/procedural/OTA/PSRAM contract: '+name
    pack = (ROOT/'cartao/RPGPOKET/artes.pak').read_bytes()
    assert len(pack) == CONTRACT['art_total_bytes'] and hashlib.sha256(pack).hexdigest() == CONTRACT['art_sha256']
    assert struct.unpack('<4sIII', pack[:16]) == (b'PKA1',7470912,0x9b4bcfc0,666) and zlib.crc32(pack[16:]) == 0x9b4bcfc0
    atlas = (ROOT/'firmware/RPG_POKET_2/Ruins7Art.h').read_text()
    atlas_bytes = sum(len(values.replace('\n','').split(',')) * (2 if typ == 'uint16_t' else 1)
                      for typ, values in re.findall(r'inline constexpr (uint\d+_t) \w+\[[^\]]+\]=\{([^}]+)\};', atlas))
    assert atlas_bytes == 18144
    # Compare allocator calls over the entire production tree against package6.
    alloc = re.compile(r'\b(?:malloc|calloc|realloc|ps_malloc|ps_calloc|heap_caps_malloc|heap_caps_calloc|heap_caps_realloc)\s*\(')
    before = sum(len(alloc.findall(git('show',BASELINE+':'+name).decode())) for name in git('ls-tree','-r','--name-only',BASELINE,'firmware').decode().splitlines())
    after = sum(len(alloc.findall((ROOT/name).read_text())) for name in production)
    assert after == before, 'Unexpected new production allocator call'
    report = dict(schema=1, source_commit=git('rev-parse','HEAD').decode().strip(), source_tree=git('rev-parse','HEAD^{tree}').decode().strip(), frozen_feature_tree=FROZEN_TREE, frozen_inventory_sha256=INVENTORY_SHA, frozen_files=len(entries), packaging_exceptions=sorted(PACKAGING), contract=CONTRACT, new_art_payload_bytes=0, new_atlas_flash_bytes=18144, new_psram_allocations=0, framebuffer_bytes=153600, decode_workspace_bytes=13760, known_psram_bytes=7638272, candidate_firmware_bytes=None, target_compile_verified=False, physical_validation=False, published=False, note='Static known allocation bound, not measured free heap/FPS/touch latency. Target link and real-byte OTA are later gates.')
    (ROOT/'build').mkdir(exist_ok=True)
    (ROOT/'build/package7-scope-budget.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: exact 871-file integration inventory with bounded packaging exceptions; Save30/epoch1, art666, unchanged procedural/OTA stores; atlas +18144 flash, zero added allocation calls, known PSRAM7638272.')


if __name__ == '__main__':
    main()
