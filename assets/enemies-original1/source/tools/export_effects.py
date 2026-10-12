#!/usr/bin/env python3
"""Small original code-native contact VFX, separate from character art."""
from pathlib import Path
from PIL import Image,ImageDraw
import struct,json,hashlib,sys
from export_enemies import draw_fx
ROOT=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).parent/'exports/effects';ROOT.mkdir(parents=True,exist_ok=True)
PAL=[0xf81f,0xdef5,0x7e38,0xad8c]+[0]*12
# These RGB565 values are the quantized pale contact, cold magical and dust tones.
from export_enemies import from565,rgb565
records=[]
for kind in ['slash','bite','charge','claw','magic','pulse','crush']:
 p=ROOT/kind;p.mkdir(exist_ok=True);frames=[]
 for i in range(8):
  im=Image.new('RGBA',(32,32));d=ImageDraw.Draw(im);draw_fx(d,kind,16,16,i/7);rgba=[];idx=[]
  for r,g,b,a in im.getdata():
   if not a:rgba.append((0,0,0,0));idx.append(0)
   else:
    n=2 if kind in ['magic','pulse'] else 3 if kind=='charge' else 1;rgba.append((*from565(PAL[n]),255));idx.append(n)
  im.putdata(rgba);im.save(p/f'{i:02d}.png');frames.append(im);(p/f'{i:02d}.rgb565le').write_bytes(struct.pack('<1024H',*[PAL[x] for x in idx]));(p/f'{i:02d}.idx4').write_bytes(bytes((idx[j]<<4)|idx[j+1] for j in range(0,1024,2)))
 (p/'palette.rgb565le').write_bytes(struct.pack('<16H',*PAL));frames[0].save(p/'effect_preview.png',save_all=True,append_images=frames[1:],duration=40,loop=0,blend=0,disposal=0)
 (p/'manifest.json').write_text(json.dumps(dict(kind=kind,size=[32,32],frame_count=8,duration_ms=320,source='Original procedural pixel VFX; no imagegen required for existing code-native visual grammar',transparency='0xF81F / index0',note='Timing scaled to contact interval of the actual action; does not add damage, element or mechanic.',palette_rgb565=PAL),indent=2))
 records.append(dict(kind=kind,frames=8,index_bytes=4096,palette_bytes=32,rgb565_bytes=16384))
(ROOT/'manifest.json').write_text(json.dumps({'effects':records,'total_indexed_bytes':sum(x['index_bytes']+x['palette_bytes'] for x in records),'runtime_integrated':False},indent=2));print('7 effect families, 56 frames, 28896 bytes indexed with palettes')
