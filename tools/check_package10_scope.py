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
BASELINE = '5300dada9959c85ff3302508940344d50602036b'
BASELINE_TREE = 'a8f043f4acecda081009c96b2a74c473c3d43399'
FROZEN_TREE = '615201e01c2c3103cd04e29af1aa9d9d291644f7'
INVENTORY_SHA = '9292097f70b2a170e2622ed1b2a20eb19d494fb00f435e32a7af1b963ce69051'
PACKAGING = {'tools/check_package10_scope.py', 'tests/package10_release_verifier.py', 'tools/release_package10_manifest.py', '.github/workflows/release.yml', 'tools/test_package10_local.py', 'tools/verify_package10_release.py', 'tools/test_package10.sh', 'docs/PACKAGE10_RELEASE_QUALIFICATION.md', 'tests/package10_release.cpp', 'tests/package10_release_gate.py', 'tests/fixtures/package10-published-baselines.json', 'tools/record_package10_identity.py', 'docs/RELEASE_PACKAGE10_NOTAS.txt', 'firmware/RPG_POKET_2/ReleaseVersion.h', 'tools/test_package10_release.py', '.github/workflows/package10-check.yml', 'tests/fixtures/package10-approved-sources.json', 'PACKAGE10_STAGING_ONLY.txt', 'tests/package10_qualification_config.py', 'tools/check_package10_release_gate.py'}
GENERATED = {'release/firmware.bin', 'release/artes.pak', 'release/manifest.json', 'release/SHA256SUMS'}
CONTRACT = {'version': '2026.10.14-pvp77', 'build': 2026101404, 'package': '7.7 - Duelo PvP', 'generation': '7.7', 'save_format': 30, 'reads_save_formats': [29, 30], 'save_reset_epoch': 1, 'art_assets': 666, 'art_payload_bytes': 7470912, 'art_total_bytes': 7470928, 'art_crc32': '9b4bcfc0', 'art_sha256': '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56', 'known_psram_bytes': 7638272, 'ota_partition_bytes': 3145728, 'new_atlas_flash_bytes': 0, 'new_psram_allocations': 0}


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
    frozen = json.loads((ROOT/'tests/fixtures/package10-approved-sources.json').read_text())
    assert frozen['schema'] == 1 and frozen['baseline_commit'] == BASELINE and frozen['baseline_tree'] == BASELINE_TREE
    assert frozen['frozen_feature_tree'] == FROZEN_TREE
    assert frozen['approved_pvp_patch_sha256'] == 'ac9e3725b42cf1b0de949de8cac770673e0f8bf15a88c342dfb7fde0e1675b16'
    assert frozen['approved_pvp_sources_sha256'] == 'caf2bd0c11924ee848d4618526867b4b62d6dab370ebed6ae687edce091764f9'
    assert frozen['host_query_link_fix'] == {'schema': 1, 'failed_candidate_commit': '9841c466d1755f803246727a8a9cabf5c10d8483', 'failed_run': 38575517453, 'patch_sha256': '86d2711a32e25a4b098521e9a272e088553c3390192b168c80f78184b8c17671', 'purpose': 'Move unchanged pure query bodies into the header; preserve legacy header-only real-renderer harnesses without stubs.', 'files': {'firmware/RPG_POKET_2/ClubDuel.cpp': {'bytes': 6145, 'sha256': '23d1d986ba763dfaf98b8e72c1b44d30e03ad41dd207e47a7b9ef63a9b2b1ee1'}, 'firmware/RPG_POKET_2/ClubDuel.h': {'bytes': 2758, 'sha256': '7b406221df231cec7f52f851637d34fd73d8a67c36989e87f2b899ca87d0e924'}}}
    assert frozen['preserved_clock_commit'] == '1ca079e2a93d8b99a71a03e98d69cb6c201dbb56'
    assert set(frozen['packaging_exceptions']) == PACKAGING and frozen['contract'] == CONTRACT
    entries = frozen['frozen_files']
    assert len(entries) == 953 and len({entry['path'] for entry in entries}) == 953
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
    assert header == '#pragma once\nconstexpr const char* FW_VERSION="2026.10.14-pvp77";\nconstexpr uint32_t FW_BUILD=2026101404u;\nconstexpr const char* FW_PACKAGE="7.7 - Duelo PvP";\n', 'Only exact release identity is allowed in packaging header'
    assert re.search(r'FW_VERSION="([^"]+)"', header).group(1) == CONTRACT['version']
    assert int(re.search(r'FW_BUILD=(\d+)u', header).group(1)) == CONTRACT['build']
    assert re.search(r'FW_PACKAGE="([^"]+)"', header).group(1) == CONTRACT['package']
    for name in ('Rules.h', 'RPG_POKET_2.ino', 'WorldClock.h', 'DeviceSettings.h', 'ClockCycle7.h', 'ClockView.h', 'HeroPresentation.h', 'HeroActionView.h', 'PersonalArt.h', 'Save.h', 'Persistence.h', 'SaveVersion.h', 'ResetEpoch.h', 'AssetCatalog.h', 'AssetStore.h', 'EnemyArt.h', 'Dungeon.h', 'DungeonGeneration.h', 'UpdateDevice.h'):
        path = 'firmware/RPG_POKET_2/'+name
        assert (ROOT/path).read_bytes() == git('show', BASELINE+':'+path), 'Unchanged Save30/epoch/procedural/OTA/PSRAM contract: '+name
    pack = (ROOT/'cartao/RPGPOKET/artes.pak').read_bytes()
    assert len(pack) == CONTRACT['art_total_bytes'] and hashlib.sha256(pack).hexdigest() == CONTRACT['art_sha256']
    assert struct.unpack('<4sIII', pack[:16]) == (b'PKA1',7470912,0x9b4bcfc0,666) and zlib.crc32(pack[16:]) == 0x9b4bcfc0
    atlas = (ROOT/'firmware/RPG_POKET_2/Ruins7Art.h').read_text()
    atlas_bytes = sum(len(values.replace('\n','').split(',')) * (2 if typ == 'uint16_t' else 1)
                      for typ, values in re.findall(r'inline constexpr (uint\d+_t) \w+\[[^\]]+\]=\{([^}]+)\};', atlas))
    assert atlas_bytes == 18144
    protocol = (ROOT/'firmware/RPG_POKET_2/ClubProtocol.h').read_text()
    assert 'constexpr uint8_t VERSION = 5;' in protocol and 'static_assert(sizeof(Packet) == 52' in protocol
    assert 'Atualize as duas placas' in (ROOT/'firmware/RPG_POKET_2/ClubRadio.h').read_text()
    # Compare allocator calls over the entire production tree against published clock77.
    alloc = re.compile(r'\b(?:malloc|calloc|realloc|ps_malloc|ps_calloc|heap_caps_malloc|heap_caps_calloc|heap_caps_realloc)\s*\(')
    before = sum(len(alloc.findall(git('show',BASELINE+':'+name).decode())) for name in git('ls-tree','-r','--name-only',BASELINE,'firmware').decode().splitlines())
    after = sum(len(alloc.findall((ROOT/name).read_text())) for name in production)
    assert after == before, 'Unexpected new production allocator call'
    report = dict(schema=1, source_commit=git('rev-parse','HEAD').decode().strip(), source_tree=git('rev-parse','HEAD^{tree}').decode().strip(), frozen_feature_tree=FROZEN_TREE, frozen_inventory_sha256=INVENTORY_SHA, frozen_files=len(entries), packaging_exceptions=sorted(PACKAGING), contract=CONTRACT, new_art_payload_bytes=0, new_atlas_flash_bytes=0, new_psram_allocations=0, framebuffer_bytes=153600, decode_workspace_bytes=13760, known_psram_bytes=7638272, candidate_firmware_bytes=None, target_compile_verified=False, physical_validation=False, published=False, pvp_protocol_version=5, pvp_packet_bytes=52, pvp_host_sizes={'ClubUi':640,'Session':488,'Duel':104,'Packet':52,'Peer':44}, pvp_radio_queue_added_bytes=64, note='Static known allocation bound, not measured free heap/FPS/touch latency. Target link and real-byte OTA are later gates.')
    (ROOT/'build').mkdir(exist_ok=True)
    (ROOT/'build/package10-scope-budget.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: exact 953-file clock+approved-PvP inventory with bounded packaging exceptions; Save30/epoch1, art666, unchanged procedural/OTA stores; atlas unchanged, +0 flash, zero added allocation calls, known PSRAM7638272.')


if __name__ == '__main__':
    main()
