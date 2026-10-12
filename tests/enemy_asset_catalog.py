#!/usr/bin/env python3
"""Compact enemy package integrity, baseline preservation and fallback bounds."""
from pathlib import Path
import gzip
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import prepare_enemy_art as art
from unpack_art import reconstruct_legacy_pack


class EnemyAssetCatalogTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.result = art.build()
        cls.old_pack = reconstruct_legacy_pack()
        cls.baseline = json.loads((art.ASSETS / 'baseline/catalog.json').read_text())
        cls.old_fallback = gzip.decompress((art.ASSETS / 'baseline/fallback.bin.gz').read_bytes())
        cls.exports, cls.enemies = art.load_exports()

    def test_deterministic_output_and_generated_files(self):
        second = art.build()
        self.assertEqual(self.result.pack, second.pack)
        self.assertEqual(self.result.catalog, second.catalog)
        self.assertEqual(self.result.descriptors, second.descriptors)
        self.assertEqual(self.result.report, second.report)
        art.verify_generated(self.result)
        rows, fallback = art.parse_catalog(art.CATALOG_PATH.read_text())
        self.assertEqual(rows, self.result.metadata)
        self.assertEqual(fallback, self.result.fallback)


    def test_self_contained_reference_extraction_and_bundle_reproduction(self):
        with tempfile.TemporaryDirectory(prefix='.references-test-', dir=art.ASSETS) as tmp:
            tmp = Path(tmp)
            self.assertEqual(art.extract_reference(tmp), 552)
            self.assertEqual(len((tmp / 'enemies.tsv').read_text().splitlines()), 30)
            self.assertEqual(len((tmp / 'effects.tsv').read_text().splitlines()), 8)
            self.assertEqual(art.repack_exports(tmp, write=False), (art.ASSETS / 'exports.tar.gz').read_bytes())
            for enemy in self.enemies:
                folder = tmp / f'exports/enemies/{enemy["id"]:02d}_{enemy["slug"]}'
                self.assertEqual(json.loads((folder / 'manifest.json').read_text()), enemy)
                self.assertEqual((folder / 'idle.rgb565le').stat().st_size, 13760)


    def test_package_identity_and_budget(self):
        packet = self.result.pack
        self.assertEqual(struct.unpack('<4sIII', packet[:16]), (b'PKA1', 6661440, 0x0b84d93f, 636))
        self.assertEqual(len(packet), 6661456)
        self.assertEqual(zlib.crc32(packet[16:]), 0x0b84d93f)
        self.assertEqual(hashlib.sha256(packet).hexdigest(), '5416f6d507601bc131751e89106332f291f8d1d09642faa264a9b61215dd43f1')
        self.assertEqual(self.result.report['replacement_bytes'], 428864)
        self.assertEqual(self.result.report['known_psram_bytes'], 6828800)
        self.assertEqual(self.result.report['theoretical_8mib_headroom_bytes'], 1559808)
        self.assertFalse(self.result.report['hardware_measured'])

    def test_exact_slot_mapping_and_bounds(self):
        expected = list(range(92, 123)) + list(range(401, 417)) + list(range(459, 464)) + list(range(480, 544))
        self.assertEqual(list(art.BODY_SLOTS), expected)
        self.assertEqual(len(set(art.BODY_SLOTS)), 116)
        end = 0
        for row in self.result.metadata:
            offset, w, h, fb, tw, th, elem = row
            self.assertEqual(offset, end)
            self.assertLessEqual(fb + tw * th * elem, len(self.result.fallback))
            end += w * h * elem
        self.assertEqual(end, 6661440)
        for slot in art.BODY_SLOTS:
            self.assertEqual(self.result.metadata[slot][1:3], (40, 86))
            self.assertEqual(self.result.metadata[slot][4:], (1, 1, 1))
        for slot in art.EFFECT_SLOTS:
            self.assertEqual(self.result.metadata[slot][1:3], (16, 32))
            self.assertEqual(self.result.metadata[slot][4:], (1, 1, 1))

    def test_all_428_non_enemy_payloads_and_fallbacks_unchanged(self):
        old, new = self.old_pack[16:], self.result.pack[16:]
        compared = 0
        for i, old_row in enumerate(self.baseline['metadata']):
            if i in art.RETIRED:
                continue
            row = self.result.metadata[i]
            size = old_row[1] * old_row[2] * old_row[6]
            fallback_size = old_row[4] * old_row[5] * old_row[6]
            self.assertEqual(new[row[0]:row[0] + size], old[old_row[0]:old_row[0] + size], i)
            self.assertEqual(self.result.fallback[row[3]:row[3] + fallback_size], self.old_fallback[old_row[3]:old_row[3] + fallback_size], i)
            self.assertEqual(row[1:3] + row[4:], tuple(old_row[1:3] + old_row[4:]), i)
            compared += 1
        self.assertEqual(compared, 428)

    def test_all_172_packed_frames_match_independent_rgb565_references(self):
        # Independent decoder: byte unpack plus explicit palette-word indexing.
        payload = self.result.pack[16:]
        references = self.result.report['reference_frames']
        self.assertEqual(len(references), 172)
        for ref in references:
            slot = ref['asset_id']
            if slot in art.BODY_SLOTS:
                palette_slot = 600 + art.BODY_SLOTS.index(slot) // 4
            else:
                palette_slot = 629 + (slot - 544) // 8
            m, pal = self.result.metadata[slot], self.result.metadata[palette_slot]
            palette = struct.unpack('<16H', payload[pal[0]:pal[0] + 32])
            words = []
            for byte in payload[m[0]:m[0] + m[1] * m[2]]:
                words.extend((palette[byte // 16], palette[byte % 16]))
            actual = struct.pack('<' + str(len(words)) + 'H', *words)
            self.assertEqual(actual, self.exports[ref['reference']], slot)
            self.assertEqual(hashlib.sha256(actual).hexdigest(), ref['sha256'])

    def test_fallback_planes_are_zero_and_palettes_are_real(self):
        for slot in art.BODY_SLOTS + art.EFFECT_SLOTS:
            row = self.result.metadata[slot]
            self.assertEqual(row[4:], (1, 1, 1))
            self.assertEqual(self.result.fallback[row[3]], 0)
        for slot in art.ENEMY_PALETTES + art.EFFECT_PALETTES:
            row = self.result.metadata[slot]
            self.assertEqual(row[1:3] + row[4:], (16, 1, 16, 1, 2))
            self.assertEqual(self.result.fallback[row[3]:row[3] + 32], self.result.pack[16 + row[0]:16 + row[0] + 32])
            self.assertEqual(self.result.fallback[row[3]:row[3] + 2], b'\x1f\xf8')

    def test_manifest_anchors_poses_and_provenance(self):
        self.assertEqual([e['id'] for e in self.enemies], list(range(29)))
        self.assertEqual([e['id'] for e in self.enemies if e['anchor_mode'] == 'center'], [5, 19, 21, 28])
        self.assertEqual(self.enemies[25]['root_anchor'], [55, 82])
        hashes = []
        for e in self.enemies:
            self.assertEqual(tuple(e['pose_order']), art.POSES)
            for pose in art.POSES:
                path = f'exports/enemies/{e["id"]:02d}_{e["slug"]}/{pose}.png'
                hashes.append(hashlib.sha256(self.exports[path]).hexdigest())
        self.assertEqual(len(set(hashes)), 116)
        self.assertEqual(self.result.report['effect_family_order'], list(art.FAMILIES))

    def test_compiled_fallback_canaries_and_constexpr_descriptors(self):
        compiler = os.environ.get('CXX', 'g++')
        if not shutil.which(compiler):
            self.skipTest(f'{compiler} is not available; C++ fallback not executed')
        source = r'''
#include "EnemyArtCatalog.h"
#include <cassert>
#include <vector>
int main() {
  constexpr unsigned guard=32;
  std::vector<uint8_t> bytes(ART_BYTES+2*guard,0xa5);
  artMemory=bytes.data()+guard;
  makeFallback();
  for(unsigned i=0;i<guard;++i) assert(bytes[i]==0xa5 && bytes[guard+ART_BYTES+i]==0xa5);
  for(unsigned i=0;i<enemyArt::ENEMY_COUNT;++i) {
    const auto& e=enemyArt::enemies[i];
    assert(e.palette==600+i);
    for(unsigned p=0;p<4;++p) {
      const auto& m=assetMeta[e.poses[p]];
      for(unsigned b=0;b<enemyArt::BODY_BYTES;++b) assert(artMemory[m.offset+b]==0);
    }
  }
  for(unsigned k=0;k<7;++k) for(unsigned f=0;f<8;++f) {
    const auto& m=assetMeta[enemyArt::effectFrames[k][f]];
    for(unsigned b=0;b<enemyArt::FX_BYTES;++b) assert(artMemory[m.offset+b]==0);
  }
  for(unsigned i=600;i<636;++i) {
    const auto& m=assetMeta[i];
    for(unsigned b=0;b<32;++b) assert(artMemory[m.offset+b]==artFallback[m.fallback+b]);
  }
  artMemory=nullptr;
}
'''
        with tempfile.TemporaryDirectory(prefix='.catalog-test-', dir=art.ASSETS) as tmp:
            tmp = Path(tmp)
            (tmp / 'test.cpp').write_text(source)
            flags = ['-std=c++17', '-O1', '-g', '-fno-omit-frame-pointer', '-fsanitize=address,undefined', '-no-pie']
            subprocess.run([compiler, *flags, '-I', str(ROOT / 'firmware/RPG_POKET_2'), str(tmp / 'test.cpp'), '-o', str(tmp / 'test')], check=True, capture_output=True)
            # The traced cloud executor cannot run LeakSanitizer; ASan/UBSan stay enabled.
            env = dict(os.environ, ASAN_OPTIONS=os.environ.get('ASAN_OPTIONS', '') + ':detect_leaks=0')
            run = subprocess.run([str(tmp / 'test')], capture_output=True, text=True, env=env)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)

    def test_no_legacy_rgb565_accessors_to_repurposed_slots(self):
        # Numeric raw reads and the two indexed legacy accessors are prohibited.
        # Typed byte access in EnemyArt.h is allowed and tested separately.
        offenders = []
        for path in (ROOT / 'firmware/RPG_POKET_2').glob('*.h'):
            if path.name == 'AssetCatalog.h':
                continue
            text = path.read_text()
            for match in re.finditer(r'reinterpret_cast\s*<\s*const\s+uint16_t\s*\*\s*>\s*\(\s*assetBytes\s*\(\s*(\d+)\s*\)', text):
                if int(match.group(1)) in art.BODY_SLOTS + art.EFFECT_SLOTS:
                    offenders.append(f'{path.name}: raw asset {match.group(1)}')
            for name in ('regionEnemyFrame', 'worldEnemyFrame'):
                if re.search(r'inline\s+const\s+uint16_t\s*\*\s*' + name + r'\s*\([^\n]*reinterpret_cast', text):
                    offenders.append(f'{path.name}: {name}')
        self.assertEqual(offenders, [], '\n'.join(offenders))


if __name__ == '__main__':
    unittest.main(verbosity=2)
