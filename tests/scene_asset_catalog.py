#!/usr/bin/env python3
from pathlib import Path
import base64,gzip,hashlib,json,struct,sys,unittest,zlib
sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import prepare_enemy_art as old
import prepare_scene_art as art
class CatalogTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):cls.old=old.build();cls.new=art.build(cls.old)
 def test_determinism(self):self.assertEqual(self.new,art.build(self.old));art.verify_generated(self.new)
 def test_baseline_unchanged(self):
  n=len(self.old.pack)-16;self.assertEqual(self.new.pack[16:16+n],self.old.pack[16:]);self.assertEqual(self.new.metadata[:636],self.old.metadata);self.assertEqual(self.new.fallback[:len(self.old.fallback)],self.old.fallback)
 def test_envelope_and_budget(self):
  p=self.new.pack;r=self.new.report;self.assertEqual(struct.unpack('<4sIII',p[:16]),(b'PKA1',r['payload_bytes'],zlib.crc32(p[16:]),r['assets']));self.assertEqual(hashlib.sha256(p).hexdigest(),r['pack_sha256']);self.assertLessEqual(r['known_psram_bytes'],8*1024*1024-512*1024)
 def test_dense_bounds(self):
  end=0
  for offset,w,h,fb,tw,th,elem in self.new.metadata:self.assertEqual(end,offset);end+=w*h*elem;self.assertLessEqual(fb+tw*th*elem,len(self.new.fallback));self.assertTrue(0<tw<256 and 0<th<256)
  self.assertEqual(end,len(self.new.pack)-16)
 def test_all_pixels_reference(self):
  for record in self.new.report['scenes']:
   a=self.new.metadata[record['plane']];p=self.new.metadata[record['palette']];plane=self.new.pack[16+a[0]:16+a[0]+76800];palette=self.new.pack[16+p[0]:16+p[0]+512];pixels=b''.join(palette[v*2:v*2+2] for v in plane);self.assertEqual(hashlib.sha256(pixels).hexdigest(),record['reference_rgb565_sha256']);self.assertEqual(a[1:3],(240,320));self.assertEqual(p[1:3],(128,2))
 def test_fallback_indices_palette(self):
  for r in self.new.report['scenes']:
   a=self.new.metadata[r['plane']];p=self.new.metadata[r['palette']];self.assertEqual(self.new.fallback[p[3]:p[3]+512],self.new.pack[16+p[0]:16+p[0]+512]);self.assertEqual(len(self.new.fallback[a[3]:a[3]+256]),256)
if __name__=='__main__':unittest.main()
