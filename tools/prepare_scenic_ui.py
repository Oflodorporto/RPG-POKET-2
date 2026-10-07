"""Convert generated paired artwork to firmware-native indexed backgrounds."""
from pathlib import Path
from PIL import Image
import hashlib,json
root=Path(__file__).resolve().parents[1]
out=root/'assets/abrigo-menu1'
source=out/'conceito-original.png'
import gzip
for asset in (source,out/'conceito-mapa-ruinas.png'):
 if not asset.exists():asset.write_bytes(gzip.decompress(Path(str(asset)+'.gz').read_bytes()))
im=Image.open(source).convert('RGB')
# White gutter is excluded; no painted text or controls in the backgrounds.
panels={'shelter':(0,0,578,788),'menu':(596,0,1172,788),'map':(0,0,754,1024),'ruins':(784,0,1536,1024)}
def array(v):return ',\n'.join(','.join(str(x) for x in v[i:i+40]) for i in range(0,len(v),40))
header='#pragma once\n#include <stdint.h>\nnamespace scenicArt {\n'
for name,box in panels.items():
 if name in ('map','ruins'):im=Image.open(out/'conceito-mapa-ruinas.png').convert('RGB')
 image=im.crop(box).resize((240,320),Image.Resampling.LANCZOS)
 indexed=image.quantize(colors=256,dither=Image.Dither.NONE)
 indexed.convert('RGB').save(out/(name+'-240x320.png'))
 p=indexed.getpalette();palette=[((p[i*3]>>3)<<11)|((p[i*3+1]>>2)<<5)|(p[i*3+2]>>3) for i in range(256)]
 header+='inline constexpr uint16_t '+name+'Palette[256]={'+array(palette)+'};\n'
 header+='inline constexpr uint8_t '+name+'Pixels[240*320]={'+array(indexed.tobytes())+'};\n'
header+='}\n'
(root/'firmware/RPG_POKET_2/ScenicArt.h').write_text(header)
(out/'proveniencia.json').write_text(json.dumps({'source':'Exact user supplied concepts, deterministic crop/resize/RGB565 indexed conversion; generated alternative retained but unused','prompt':'Preserve original artwork, text, buttons, colors and proportions; clickable regions use the same button rectangles; gameplay status is live.','original_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'map_ruins_sha256':hashlib.sha256((out/'conceito-mapa-ruinas.png').read_bytes()).hexdigest(),'crop_boxes':panels,'dimensions':[240,320],'palette_colors':256,'flash_bytes':309248,'sd_changes':False},indent=2))
print('Four exact concept panels, 309248 bytes in flash; no SD pack changes')
