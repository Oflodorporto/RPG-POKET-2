"""Reuse authored medieval UI pixels as a small flash-only atlas (not new art).
Source: existing exact Ruins concept + NPC portrait in the immutable art pack.
Only crop/scale/palette packing. Dynamic text and status are never in this atlas.
"""
from pathlib import Path
from PIL import Image,ImageDraw
import gzip,io,re,struct
root=Path(__file__).resolve().parents[1];out=root/'assets/ruins7'
im=Image.open(io.BytesIO(gzip.decompress((root/'assets/abrigo-menu1/conceito-mapa-ruinas.png.gz').read_bytes()))).convert('RGBA').crop((784,0,1536,1024))
# Preserve the original lettering, jewel and ornaments, with transparent corners.
title=im.crop((120,62,650,196));mask=Image.new('L',title.size,0)
ImageDraw.Draw(mask).polygon([(260,0),(292,27),(501,27),(514,42),(514,62),(530,71),(514,83),(514,119),(500,134),(17,134),(6,119),(6,84),(0,72),(10,61),(10,43),(21,27),(232,27)],fill=255)
title.putalpha(mask);title=title.resize((220,56),Image.Resampling.LANCZOS)
# Same softly rounded authored frame, center reduced to its unlettered blue field.
button=im.crop((168,786,433,875)).resize((112,44),Image.Resampling.LANCZOS)
frame=Image.new('RGBA',(40,32),(0,0,0,0))
for y in range(32):
 for x in range(40):
  sx=x if x<6 else 112-(40-x) if x>=34 else 105
  sy=y if y<6 else 44-(32-y) if y>=26 else 6+(y-6)*32//20
  frame.putpixel((x,y),button.getpixel((sx,sy)))
# Rich original icon crops. The blue field matches the authored button interior.
boxes={'guardian':(364,480,489,620),'explore':(193,801,253,864),'camp':(471,800,541,863),'map':(198,911,254,971),'shop':(473,911,540,972)}
icons={name:im.crop(box).resize((24,24),Image.Resampling.LANCZOS) for name,box in boxes.items()}
# Cache Iria's actual portrait in flash to preserve the same UI without an SD card.
meta=[tuple(map(int,m.split(','))) for m in re.findall(r'\{([\d,]+)\}',(root/'firmware/RPG_POKET_2/AssetCatalog.h').read_text())]
pack=(root/'cartao/RPGPOKET/artes.pak').read_bytes()[16:]
values=struct.unpack('<256H',pack[meta[435][0]:meta[435][0]+512]);rgb=[((v>>11)*255//31,((v>>5)&63)*255//63,(v&31)*255//31) for v in values]
portrait=Image.new('RGB',(64,64));portrait.putdata([rgb[i] for i in pack[meta[436][0]:meta[436][0]+4096]])
icons['iria']=portrait.resize((24,24),Image.Resampling.LANCZOS).convert('RGBA')
# The reusable equipment bag has a real transparent background.
bag=Image.open(root/'assets/reusable/equipment01/props/backpack.png').convert('RGBA');icons['bag']=bag.resize((24,24),Image.Resampling.LANCZOS)
images={'title':title,'frame':frame,**icons};strip=Image.new('RGB',(sum(i.width*i.height for i in images.values()),1))
colors=[]
for image in images.values():colors.extend([(r,g,b) for r,g,b,a in image.getdata()])
strip.putdata(colors);quant=strip.quantize(colors=255,dither=Image.Dither.NONE);pal=quant.getpalette()[:765]+[255,0,255];palette=[((pal[i*3]>>3)<<11)|((pal[i*3+1]>>2)<<5)|(pal[i*3+2]>>3) for i in range(256)]
def arr(v):return ',\n'.join(','.join(str(x) for x in v[i:i+40]) for i in range(0,len(v),40))
header='#pragma once\n#include <stdint.h>\n// Authored Ruins UI crops, deterministic flash-only reuse. See prepare_ruins7_ui.py.\nnamespace ruins7Art {\ninline constexpr uint16_t palette[256]={'+arr(palette)+'};\n'
pos=0;idx=list(quant.tobytes())
for name,image in images.items():
 n=image.width*image.height;data=idx[pos:pos+n];pos+=n
 for i,a in enumerate(image.getchannel('A').getdata()):
  if a<128:data[i]=255
 header+=f'inline constexpr unsigned {name}W={image.width},{name}H={image.height};\ninline constexpr uint8_t {name}Pixels[{n}]={{'+arr(data)+'};\n'
 image.save(out/f'ui-{name}.png')
header+='}\n';(root/'firmware/RPG_POKET_2/Ruins7Art.h').write_text(header)
print('PASS: authored UI reused in flash:',pos+512,'bytes; no SD/PSRAM allocation')
