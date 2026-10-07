from pathlib import Path
from PIL import Image,ImageOps
import shutil,json,hashlib
p=Path(__file__).resolve().parents[1];out=p/'assets/cartas1';out.mkdir(exist_ok=True)
source=out/'hipogrifo-original.png'
im=Image.open(source).convert('RGBA');alpha=im.getchannel('A');occupied=[]
for x in range(im.width):
 occupied.append(alpha.crop((x,0,x+1,im.height)).getextrema()[1]>=128)
runs=[];start=None
for x,v in enumerate(occupied+[False]):
 if v and start is None:start=x
 if not v and start is not None:runs.append((start,x));start=None
# Merge very small transparent seams inside a creature; retain gaps between sprites.
groups=[]
for a,b in runs:
 if groups and a-groups[-1][1]<10:groups[-1]=(groups[-1][0],b)
 else:groups.append((a,b))
assert len(groups)==4,groups
crops=[]
for a,b in groups:
 cell=im.crop((a,0,b,im.height));crops.append(cell.crop(cell.getchannel('A').getbbox()))
scale=min(76/max(c.width for c in crops),82/max(c.height for c in crops));lines=['#pragma once','#include <stdint.h>','namespace hippogriffArt {'];frames=[]
for i,c in enumerate(crops):
 c=c.resize((round(c.width*scale),round(c.height*scale)),Image.Resampling.NEAREST);frame=Image.new('RGBA',(80,86));frame.alpha_composite(c,((80-c.width)//2,84-c.height));frame.save(out/f'hipogrifo-{i}.png');frames.append(frame)
 vals=[0xf81f if a<128 else (r>>3)<<11|(g>>2)<<5|(b>>3) for r,g,b,a in frame.getdata()];lines+=['constexpr uint16_t frame'+str(i)+'[]={']+[','.join(map(str,vals[j:j+32]))+',' for j in range(0,len(vals),32)]+['};']
lines+=['constexpr const uint16_t* frames[]={frame0,frame1,frame2,frame3};','}'];(p/'firmware/RPG_POKET_2/HippogriffArt.h').write_text('\n'.join(lines))
board=Image.new('RGBA',(320,86))
for i,c in enumerate(frames):board.alpha_composite(c,(i*80,0))
board.save(out/'hipogrifo-sprites.png');(out/'proveniencia.json').write_text(json.dumps({'source':'generated original hippogriff for RPG POKET','source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'frames':4,'width':80,'height':86,'facing':'left','flash_bytes':55040,'crop_columns':groups},indent=2))
print('Four transparent left-facing sprites converted:',groups)
