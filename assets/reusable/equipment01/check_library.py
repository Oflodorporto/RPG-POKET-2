"""Validate compact native equipment, atlas coordinates and alpha contract."""
from pathlib import Path
from PIL import Image
import hashlib
import json
root=Path(__file__).resolve().parent
manifest=json.loads((root/'manifest.json').read_text())
count=0
for record in [export for asset in manifest['assets'] for export in asset['exports'].values()]+manifest['scene_props']+manifest['map_markers']:
    path=root/record['file'];im=Image.open(path)
    assert im.mode=='RGBA' and im.size==(record['width'],record['height'])
    assert set(im.getchannel('A').getdata())=={0,255}
    if 'sha256' in record:assert hashlib.sha256(path.read_bytes()).hexdigest()==record['sha256']
    if 'visible_bounds' in record:assert list(im.getchannel('A').getbbox())==record['visible_bounds']
    if 'anchor' in record:assert 0<=record['anchor'][0]<im.width and 0<=record['anchor'][1]<im.height
    count+=1
assert count==64 and len(manifest['assets'])==24
for size in (32,48):
    path=root/'atlases'/f'equipment_camp_{size}'
    atlas=Image.open(path.with_suffix('.png'));coords=json.loads(path.with_suffix('.json').read_text())
    assert len(coords['frames'])==24
    for name,info in coords['frames'].items():
        r=info['frame'];crop=atlas.crop((r['x'],r['y'],r['x']+r['w'],r['y']+r['h']))
        im=Image.open(root/f'png{size}'/f'{name}.png')
        assert crop.tobytes()==im.tobytes()
print('PASS: 64 native equipment PNGs, binary alpha, dimensions/anchors, icon hashes, and both24-cell atlas coordinate maps')
