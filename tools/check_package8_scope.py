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
BASELINE = '979822bb48ad10180d182df2232e2dcded4f9d66'
BASELINE_TREE = 'a1e76917fd848b5f2dbc77ad81b18a34c7654257'
FROZEN_TREE = '6419dba3e5430d8986cef26ab023c64f40a73860'
INVENTORY_SHA = 'a28e015c4d174b2110b363d45561f7bc12c60ade279d2d4db07a92a194140183'
PACKAGING = {'PACKAGE8_STAGING_ONLY.txt', 'tests/fixtures/package8-published-baselines.json', 'tests/package8_qualification_config.py', 'docs/PACKAGE8_RELEASE_QUALIFICATION.md', 'tools/check_package8_release_gate.py', 'tests/package8_release_gate.py', 'tests/fixtures/package8-approved-sources.json', 'firmware/RPG_POKET_2/ReleaseVersion.h', 'tools/test_package8.sh', 'tests/package8_release_verifier.py', 'docs/RELEASE_PACKAGE8_NOTAS.txt', 'tools/test_package8_release.py', 'tools/test_package8_local.py', '.github/workflows/package8-check.yml', 'tests/package8_release.cpp', 'tools/release_package8_manifest.py', '.github/workflows/release.yml', 'tools/check_package8_scope.py', 'tools/verify_package8_release.py', 'tools/record_package8_identity.py'}
GENERATED = {'release/firmware.bin', 'release/artes.pak', 'release/manifest.json', 'release/SHA256SUMS'}
CONTRACT = {'version': '2026.10.14-story77', 'build': 2026101402, 'package': '7.7 - Ecos de Valdaria', 'generation': '7.7', 'save_format': 30, 'reads_save_formats': [29, 30], 'save_reset_epoch': 1, 'art_assets': 666, 'art_payload_bytes': 7470912, 'art_total_bytes': 7470928, 'art_crc32': '9b4bcfc0', 'art_sha256': '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56', 'known_psram_bytes': 7638272, 'ota_partition_bytes': 3145728, 'new_atlas_flash_bytes': 0, 'new_psram_allocations': 0}


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
    frozen = json.loads((ROOT/'tests/fixtures/package8-approved-sources.json').read_text())
    assert frozen['schema'] == 1 and frozen['baseline_commit'] == BASELINE and frozen['baseline_tree'] == BASELINE_TREE
    assert frozen['frozen_feature_tree'] == FROZEN_TREE
    assert frozen['approved_narrative_commit'] == '4f4eb428ad48914e2cf987ad8983e11cf0dd5d38'
    assert frozen['approved_narrative_tree'] == '372133e00714548c1081fdee98fdbdad9eb8b6b7'
    assert set(frozen['packaging_exceptions']) == PACKAGING and frozen['contract'] == CONTRACT
    entries = frozen['frozen_files']
    assert len(entries) == 893 and len({entry['path'] for entry in entries}) == 893
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
    # Compare allocator calls over the entire production tree against published package7.
    alloc = re.compile(r'\b(?:malloc|calloc|realloc|ps_malloc|ps_calloc|heap_caps_malloc|heap_caps_calloc|heap_caps_realloc)\s*\(')
    before = sum(len(alloc.findall(git('show',BASELINE+':'+name).decode())) for name in git('ls-tree','-r','--name-only',BASELINE,'firmware').decode().splitlines())
    after = sum(len(alloc.findall((ROOT/name).read_text())) for name in production)
    assert after == before, 'Unexpected new production allocator call'
    report = dict(schema=1, source_commit=git('rev-parse','HEAD').decode().strip(), source_tree=git('rev-parse','HEAD^{tree}').decode().strip(), frozen_feature_tree=FROZEN_TREE, frozen_inventory_sha256=INVENTORY_SHA, frozen_files=len(entries), packaging_exceptions=sorted(PACKAGING), contract=CONTRACT, new_art_payload_bytes=0, new_atlas_flash_bytes=0, new_psram_allocations=0, framebuffer_bytes=153600, decode_workspace_bytes=13760, known_psram_bytes=7638272, candidate_firmware_bytes=None, target_compile_verified=False, physical_validation=False, published=False, note='Static known allocation bound, not measured free heap/FPS/touch latency. Target link and real-byte OTA are later gates.')
    (ROOT/'build').mkdir(exist_ok=True)
    (ROOT/'build/package8-scope-budget.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: exact 893-file integration inventory with bounded packaging exceptions; Save30/epoch1, art666, unchanged procedural/OTA stores; atlas unchanged, +0 flash, zero added allocation calls, known PSRAM7638272.')


if __name__ == '__main__':
    main()
