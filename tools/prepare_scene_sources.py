#!/usr/bin/env python3
"""Authoring-only conversion; CI consumes the checked exports using stdlib alone."""
import base64,gzip,hashlib,json,struct
from pathlib import Path
from PIL import Image,__version__
root=Path(__file__).resolve().parents[1]/'assets/scenes1'
m=json.loads((root/'provenance.json').read_text())
for r in m['scenes']:
    source=(root/r['source_file']).read_bytes()
    assert hashlib.sha256(source).hexdigest()==r['source_sha256']
    image=Image.open(root/r['source_file']).convert('RGB');assert image.size==(240,320)
    q=image.quantize(colors=256,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
    colors=q.getpalette();palette=struct.pack('<256H',*[((colors[i]>>3)<<11)|((colors[i+1]>>2)<<5)|(colors[i+2]>>3) for i in range(0,768,3)])
    plane=q.tobytes();data=palette+plane;reference=b''.join(palette[v*2:v*2+2] for v in plane)
    name=r['name']+'.idx8.gz.b64';encoded=base64.b64encode(gzip.compress(data,mtime=0))+b'\n';(root/name).write_bytes(encoded)
    r.update(export_file=name,export_sha256=hashlib.sha256(encoded).hexdigest(),decoded_sha256=hashlib.sha256(data).hexdigest(),reference_rgb565_sha256=hashlib.sha256(reference).hexdigest(),bytes=len(data))
m['authoring_conversion']='Pillow '+__version__+' MEDIANCUT256, no dithering; RGB565 truncation'
(root/'provenance.json').write_text(json.dumps(m,ensure_ascii=False,indent=2)+'\n')
