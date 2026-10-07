"""Reproduce the firmware portrait background from the generated original."""
from pathlib import Path
from PIL import Image
import json,hashlib
root=Path(__file__).resolve().parents[1]
out=root/'assets/titulo1';out.mkdir(parents=True,exist_ok=True)
source=out/'titulo-original.png'
im=Image.open(source).convert('RGB').resize((240,320),Image.Resampling.LANCZOS)
indexed=im.quantize(colors=256,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
indexed.convert('RGB').save(out/'titulo-240x320.png')
palette=indexed.getpalette();rgb565=[]
for i in range(256):
 r,g,b=palette[i*3:i*3+3];rgb565.append(((r>>3)<<11)|((g>>2)<<5)|(b>>3))
def array(values):return ',\n'.join(','.join(str(v) for v in values[i:i+40]) for i in range(0,len(values),40))
header='#pragma once\n#include <stdint.h>\nnamespace titleArt {\ninline constexpr uint16_t palette[256]={'+array(rgb565)+'};\ninline constexpr uint8_t pixels[240*320]={'+array(list(indexed.tobytes()))+'};\n}\n'
(root/'firmware/RPG_POKET_2/TitleArt.h').write_text(header)
(out/'proveniencia.json').write_text(json.dumps({'source':'imagegen; user reference used for style and vertical composition','original_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'dimensions':[240,320],'palette':256,'flash_bytes':77312,'animations':'code: leaves, fire tips, embers, distant hippogriff; no SD reads'},indent=2))
print('Title art: 77312 flash bytes; 240x320; reproducible indexed palette')
