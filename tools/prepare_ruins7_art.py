"""Deterministically convert the approved generated city into the existing flash slot.
No SD catalog changes, extra background allocation, baked labels or status.
"""
from pathlib import Path
import gzip,hashlib,json,re
from PIL import Image
root=Path(__file__).resolve().parents[1]
folder=root/'assets/ruins7'; source=folder/'ruins-city-generated.png.gz'
image=Image.open(__import__('io').BytesIO(gzip.decompress(source.read_bytes()))).convert('RGB').resize((240,320),Image.Resampling.LANCZOS)
indexed=image.quantize(colors=256,dither=Image.Dither.NONE)
p=indexed.getpalette(); palette=[((p[i*3]>>3)<<11)|((p[i*3+1]>>2)<<5)|(p[i*3+2]>>3) for i in range(256)]
def array(values):return ',\n'.join(','.join(str(x) for x in values[i:i+40]) for i in range(0,len(values),40))
path=root/'firmware/RPG_POKET_2/ScenicArt.h'; header=path.read_text()
for name,kind,size,values in [('Palette','uint16_t','256',palette),('Pixels','uint8_t','240*320',list(indexed.tobytes()))]:
 pattern=r'inline constexpr '+kind+r' ruins'+name+r'\[[^\]]+\]=\{[^}]+\};'
 replacement='inline constexpr '+kind+' ruins'+name+'['+size+']={'+array(values)+'};'
 header,n=re.subn(pattern,replacement,header);assert n==1
path.write_text(header)
indexed.convert('RGB').save(folder/'ruins-city-240x320.png')
print('PASS: replaced existing 77312-byte flash slot, no runtime allocation/SD change')
