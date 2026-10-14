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
BASELINE = '6a23dda54114db611b818b51e4b1b942019098fa'
BASELINE_TREE = 'd75fd4455d677559d15cccaba6cd90861c6acf20'
FROZEN_TREE = '4c9e5360b6ed849fec9fdae74ef9580180aa7881'
INVENTORY_SHA = 'd34aa8047b7061c978317557b0fcb2eda9ceadc9fc6cd3f2d699485850ee36ae'
PACKAGING = {'tools/test_package11_local.py', '.github/workflows/release.yml', 'tools/check_package11_release_gate.py', 'tools/test_package11.sh', 'tests/package11_release.cpp', 'tools/check_package11_scope.py', 'docs/PACKAGE11_RELEASE_QUALIFICATION.md', 'docs/RELEASE_PACKAGE11_NOTAS.txt', 'tools/verify_package11_release.py', 'tools/release_package11_manifest.py', 'PACKAGE11_STAGING_ONLY.txt', 'firmware/RPG_POKET_2/ReleaseVersion.h', 'tests/fixtures/package11-approved-sources.json', 'tests/package11_qualification_config.py', 'tools/test_package11_release.py', '.github/workflows/package11-check.yml', 'tests/fixtures/package11-published-baselines.json', 'tests/package11_release_gate.py', 'tests/package11_release_verifier.py', 'tools/record_package11_identity.py'}
GENERATED = {'release/artes.pak', 'release/firmware.bin', 'release/SHA256SUMS', 'release/manifest.json'}
CONTRACT = {'version': '2026.10.14-shop77', 'build': 2026101405, 'package': '7.7 - Comercio em lote', 'generation': '7.7', 'save_format': 30, 'save_bytes': 160, 'reads_save_formats': [29, 30], 'save_reset_epoch': 1, 'ration_limit': 10, 'old_reader_ration_limit': 9, 'art_assets': 666, 'art_payload_bytes': 7470912, 'art_total_bytes': 7470928, 'art_crc32': '9b4bcfc0', 'art_sha256': '14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56', 'known_psram_bytes': 7638272, 'ota_partition_bytes': 3145728, 'new_atlas_flash_bytes': 0, 'new_psram_allocations': 0}

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
    frozen = json.loads((ROOT/'tests/fixtures/package11-approved-sources.json').read_text())
    assert frozen['schema'] == 1 and frozen['baseline_commit'] == BASELINE and frozen['baseline_tree'] == BASELINE_TREE
    assert frozen['approved_bulk_commit'] == '21ab532f3bfceb37c2def05c9d2ee1d247df30f9' and frozen['approved_bulk_tree'] == '161c2909dab80fc4d76d8f2c6bad93dcfc1c6ad0' and frozen['frozen_feature_tree'] == FROZEN_TREE
    assert frozen['target_integer_type_fix'] == {'schema': 1, 'failed_candidate_commit': '6797d22c75903b7d0cb376b2547b1c097b5d64b0', 'failed_run': 38607447395, 'patch_sha256': 'e2e422900b08c2fee7171f496698bbeae4a7e98c13f642fb3f5eca8dfb734518', 'purpose': 'Specify uint32_t as the common std::min type on ESP32; same bounded subtraction/division and sale quantity, no rule/save/layout change.', 'path': 'firmware/RPG_POKET_2/Trading.h', 'before_sha256': '47a62a5a14e8202e0ac13d34b4c59a3ee1208eaa393ed8323d86741af62cc1ed', 'after_sha256': '86ce098fea1d40bfab7db600d19a5bc4ab4eb462aad67a4a6bd612bd65d5ac8b', 'after_bytes': 7308}
    assert frozen['approved_bulk_archive_sha256'] == 'd62a0b5bb698694c58cf36abf8b60231bbe71fcbd8769ff1581949cb7a861883'
    assert len(frozen['bulk_changed_paths']) == 25
    assert set(frozen['packaging_exceptions']) == PACKAGING and frozen['contract'] == CONTRACT
    entries = frozen['frozen_files']
    assert len(entries) == 992 and len({entry['path'] for entry in entries}) == 992
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
    assert header == '#pragma once\nconstexpr const char* FW_VERSION="2026.10.14-shop77";\nconstexpr uint32_t FW_BUILD=2026101405u;\nconstexpr const char* FW_PACKAGE="7.7 - Comercio em lote";\n', 'Only exact release identity is allowed in packaging header'
    assert (ROOT/'README.md').read_bytes() == git('show', BASELINE+':README.md'), 'Preserve complete current lore/showcase README'
    for name in ('WorldClock.h', 'DeviceSettings.h', 'ClockCycle7.h', 'ClockView.h', 'HeroPresentation.h', 'HeroActionView.h', 'PersonalArt.h', 'Persistence.h', 'SaveVersion.h', 'ResetEpoch.h', 'AssetCatalog.h', 'AssetStore.h', 'EnemyArt.h', 'Dungeon.h', 'DungeonGeneration.h', 'UpdateDevice.h', 'ClubDuel.h', 'ClubDuel.cpp', 'ClubProtocol.h', 'ClubProtocol.cpp', 'ClubRadio.h'):
        path = 'firmware/RPG_POKET_2/'+name
        assert (ROOT/path).read_bytes() == git('show', BASELINE+':'+path), 'Unchanged Save30/epoch/procedural/OTA/PvP/clock contract: '+name
    assert 'PERSISTENT_SAVE_FORMAT=30' in (ROOT/'firmware/RPG_POKET_2/Persistence.h').read_text()
    assert 'PERSISTENT_SAVE_SIZE=160' in (ROOT/'firmware/RPG_POKET_2/Persistence.h').read_text()
    assert 'RATION_LIMIT=10' in (ROOT/'firmware/RPG_POKET_2/Rules.h').read_text()
    pack = (ROOT/'cartao/RPGPOKET/artes.pak').read_bytes()
    assert len(pack) == CONTRACT['art_total_bytes'] and hashlib.sha256(pack).hexdigest() == CONTRACT['art_sha256']
    assert struct.unpack('<4sIII', pack[:16]) == (b'PKA1',7470912,0x9b4bcfc0,666) and zlib.crc32(pack[16:]) == 0x9b4bcfc0
    atlas = (ROOT/'firmware/RPG_POKET_2/Ruins7Art.h').read_text()
    atlas_bytes = sum(len(values.replace('\n','').split(',')) * (2 if typ == 'uint16_t' else 1)
                      for typ, values in re.findall(r'inline constexpr (uint\d+_t) \w+\[[^\]]+\]=\{([^}]+)\};', atlas))
    assert atlas_bytes == 18144
    protocol = (ROOT/'firmware/RPG_POKET_2/ClubProtocol.h').read_text()
    assert 'constexpr uint8_t VERSION = 5;' in protocol and 'static_assert(sizeof(Packet) == 52' in protocol
    alloc = re.compile(r'\b(?:malloc|calloc|realloc|ps_malloc|ps_calloc|heap_caps_malloc|heap_caps_calloc|heap_caps_realloc)\s*\(')
    before = sum(len(alloc.findall(git('show',BASELINE+':'+name).decode())) for name in git('ls-tree','-r','--name-only',BASELINE,'firmware').decode().splitlines())
    after = sum(len(alloc.findall((ROOT/name).read_text())) for name in production)
    assert after == before, 'Unexpected new production allocator call'
    report = dict(schema=1, source_commit=git('rev-parse','HEAD').decode().strip(), source_tree=git('rev-parse','HEAD^{tree}').decode().strip(), frozen_feature_tree=FROZEN_TREE, frozen_inventory_sha256=INVENTORY_SHA, frozen_files=len(entries), target_integer_type_fix=frozen['target_integer_type_fix'], packaging_exceptions=sorted(PACKAGING), contract=CONTRACT, new_art_payload_bytes=0, new_atlas_flash_bytes=0, new_psram_allocations=0, framebuffer_bytes=153600, decode_workspace_bytes=13760, known_psram_bytes=7638272, candidate_firmware_bytes=None, target_compile_verified=False, physical_validation=False, published=False, pvp_protocol_version=5, pvp_packet_bytes=52, new_reset=False, downgrade_warning='Older readers reject rations=10; a mixed journal may recover an older <=9 record. Two records at10 block without writes. No firmware downgrade guarantee.', note='Static known allocation bound, not measured free heap/FPS/touch latency. Target link and real-byte OTA are later gates.')
    (ROOT/'build').mkdir(exist_ok=True)
    (ROOT/'build/package11-scope-budget.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: exact 992-file current-main+approved-bulk inventory; bounded packaging; Save30/160 bytes/read29+30/epoch1/rations10; unchanged PvP/clock/OTA/procedural/art666; zero new allocation calls, known PSRAM7638272.')


if __name__ == '__main__':
    main()
