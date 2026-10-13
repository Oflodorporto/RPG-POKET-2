"""Validate the lean source library without requiring presentation GIFs."""
from pathlib import Path
from PIL import Image
import hashlib,json
root=Path(__file__).resolve().parent
count=0
for manifest in (root/'manifest.json',root/'map32/manifest.json'):
 data=json.loads(manifest.read_text())
 for effect in data['effects']:
  atlas=Image.open(root/effect['atlas'])
  for frame in effect['frames']:
   path=root/frame['file']; image=Image.open(path);rect=frame['rect']
   assert image.mode=='RGBA' and list(image.size)==effect['native_size']
   assert hashlib.sha256(path.read_bytes()).hexdigest()==frame['sha256']
   assert atlas.crop((rect['x'],rect['y'],rect['x']+rect['w'],rect['y']+rect['h'])).tobytes()==image.tobytes()
   count+=1
assert count==88
print('PASS: 88 reusable frames, native dimensions, RGBA, SHA-256 and atlas mapping; no runtime integration')
