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
BASELINE = '8aa3d649fc2bae4e3050d79e02ee4b141971e490'
BASELINE_TREE = 'e0a9131d1c78205b6d463b13c049c3fe9e079f63'
FROZEN_TREE = '28c3d96d8f8c13aa74239637bb8612899fbe47f8'
INVENTORY_SHA = '167d07567c7ec443886d2ff49707aed78b1ca852ad5a174e812307eaf18a0465'
PACKAGING = {'tests/package9_qualification_config.py', '.github/workflows/package9-check.yml', 'tools/check_package9_scope.py', 'tools/check_package9_release_gate.py', 'tools/test_package9_release.py', 'docs/PACKAGE9_RELEASE_QUALIFICATION.md', 'tests/fixtures/package9-published-baselines.json', 'docs/RELEASE_PACKAGE9_NOTAS.txt', 'tools/release_package9_manifest.py', 'firmware/RPG_POKET_2/ReleaseVersion.h', 'PACKAGE9_STAGING_ONLY.txt', '.github/workflows/release.yml', 'tests/package9_release_verifier.py', 'tools/verify_package9_release.py', 'tools/record_package9_identity.py', 'tests/package9_release_gate.py', 'tools/test_package9_local.py', 'tests/fixtures/package9-approved-sources.json', 'tests/package9_release.cpp', 'tools/test_package9.sh'}
GENERATED = {'release/firmware.bin', 'release/artes.pak', 'release/manifest.json', 'release/SHA256SUMS'}
CONTRACT = {'version': '2026.10.14-clock77', 'build': 2026101403, 'package': '7.7 - Pulso do relogio', 'generation': '7.7', 'save_format': 30, 'reads_save_formats': [29, 30], 'save_reset_epoch': 1, 'art_assets': 666, 'art_payload_bytes': 7470912, 'art_total_bytes': 7470928, 'art_crc32': '9b4bcfc0', 'art_sha256': '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56', 'known_psram_bytes': 7638272, 'ota_partition_bytes': 3145728, 'new_atlas_flash_bytes': 0, 'new_psram_allocations': 0}


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def assert_generated_inventory(root, tracked):
    assert not (set(tracked) & GENERATED), 'Generated release outputs must never be tracked: '+repr(sorted(set(tracked) & GENERATED))
    for name in GENERATED:
        path = root/name
        assert not path.parent.is_symlink(), 'Generated output directory is a symlink: '+name
        if path.exists() or path.is_symlink():
            assert path.is_file() and not path.is_symlink(), 'Generated output must be a regular non-symlink file: '+name


def production_inventory(root, expected):
    production = set()
    for path in (root/'firmware').rglob('*'):
        name = str(path.relative_to(root))
        assert not path.is_symlink(), 'Production symlink outside frozen inventory: '+name
        if path.is_file():
            production.add(name)
    assert production <= expected, 'Ignored production input outside frozen inventory: '+repr(sorted(production-expected))
    return production


def main():
    assert git('rev-parse', BASELINE+'^{tree}').decode().strip() == BASELINE_TREE
    frozen = json.loads((ROOT/'tests/fixtures/package9-approved-sources.json').read_text())
    assert frozen['schema'] == 1 and frozen['baseline_commit'] == BASELINE and frozen['baseline_tree'] == BASELINE_TREE
    assert frozen['frozen_feature_tree'] == FROZEN_TREE
    assert frozen['approved_clock_production_patch_sha256'] == 'eac634fdb033f77a124b44f2d4c06dc7f5c4de099ac3d7928cac4e4478d3ac7c'
    assert frozen['preserved_story_commit'] == '9ca94294daf185ac80603e3ba409f0201eef72fb'
    assert set(frozen['packaging_exceptions']) == PACKAGING and frozen['contract'] == CONTRACT
    entries = frozen['frozen_files']
    assert len(entries) == 913 and len({entry['path'] for entry in entries}) == 913
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
    production = production_inventory(ROOT, expected)
    header = (ROOT/'firmware/RPG_POKET_2/ReleaseVersion.h').read_text()
    assert header == '#pragma once\nconstexpr const char* FW_VERSION="2026.10.14-clock77";\nconstexpr uint32_t FW_BUILD=2026101403u;\nconstexpr const char* FW_PACKAGE="7.7 - Pulso do relogio";\n', 'Only exact release identity is allowed in packaging header'
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
    # Compare allocator calls over the entire production tree against published story77.
    alloc = re.compile(r'\b(?:malloc|calloc|realloc|ps_malloc|ps_calloc|heap_caps_malloc|heap_caps_calloc|heap_caps_realloc)\s*\(')
    before = sum(len(alloc.findall(git('show',BASELINE+':'+name).decode())) for name in git('ls-tree','-r','--name-only',BASELINE,'firmware').decode().splitlines())
    after = sum(len(alloc.findall((ROOT/name).read_text())) for name in production)
    assert after == before, 'Unexpected new production allocator call'
    report = dict(schema=1, source_commit=git('rev-parse','HEAD').decode().strip(), source_tree=git('rev-parse','HEAD^{tree}').decode().strip(), frozen_feature_tree=FROZEN_TREE, frozen_inventory_sha256=INVENTORY_SHA, frozen_files=len(entries), packaging_exceptions=sorted(PACKAGING), contract=CONTRACT, new_art_payload_bytes=0, new_atlas_flash_bytes=0, new_psram_allocations=0, framebuffer_bytes=153600, decode_workspace_bytes=13760, known_psram_bytes=7638272, candidate_firmware_bytes=None, target_compile_verified=False, physical_validation=False, published=False, note='Static known allocation bound, not measured free heap/FPS/touch latency. Target link and real-byte OTA are later gates.')
    (ROOT/'build').mkdir(exist_ok=True)
    (ROOT/'build/package9-scope-budget.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: exact 913-file story+approved-clock inventory with bounded packaging exceptions; Save30/epoch1, art666, unchanged procedural/OTA stores; atlas unchanged, +0 flash, zero added allocation calls, known PSRAM7638272.')


if __name__ == '__main__':
    main()
