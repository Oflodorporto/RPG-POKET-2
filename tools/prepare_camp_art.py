"""Convert approved original sprite sheets/GIFs for flash; no card writes."""
from pathlib import Path
from PIL import Image,ImageOps,ImageDraw
import json,hashlib
root=Path(__file__).resolve().parents[1];base=root/'assets/camp1';base.mkdir(exist_ok=True)
source=Path('C:/Users/Rodolfo/OneDrive/Área de Trabalho/rpg_poket_imagens conceito/Animações ataques')
import shutil
for n in ['bola de fogo.gif','raio 3.gif']:shutil.copy2(source/n,base/n)
headers=['#pragma once','#include <stdint.h>','namespace campArt {'];entries=[]
def pack(im,name,size,crop=True):
 im=im.convert('RGBA')
 if crop:
  box=im.getchannel('A').getbbox();assert box,name;im=ImageOps.contain(im.crop(box),(size[0]-2,size[1]-2),Image.Resampling.NEAREST)
  out=Image.new('RGBA',size);out.alpha_composite(im,((size[0]-im.width)//2,size[1]-im.height))
 else:out=im.resize(size,Image.Resampling.NEAREST)
 out.save(base/(name+'.png'));vals=[0xf81f if a<128 else ((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b,a in out.getdata()]
 headers.append('constexpr uint16_t '+name+'[]={');headers.extend(','.join(map(str,vals[i:i+32]))+',' for i in range(0,len(vals),32));headers.append('};')
 entries.append(dict(name=name,width=size[0],height=size[1],sha256=hashlib.sha256((base/(name+'.png')).read_bytes()).hexdigest(),bytes=len(vals)*2))
 return out
im=Image.open(base/'heroes-original.png');rows=[(0,293),(293,574),(574,850),(850,1116)]
for cls in range(4):
 for state in range(2):
  for pose in range(2):
   a,b=rows[state*2+pose];pack(im.crop((cls*256,a,(cls+1)*256,b)),f'hero_{cls}_{state}_{pose}',(56,64))
headers.append('constexpr const uint16_t* heroes[4][2][2]={'+','.join('{'+','.join('{hero_%d_%d_0,hero_%d_%d_1}'%(c,s,c,s) for s in range(2))+'}' for c in range(4))+'};')
im=Image.open(base/'props-original.png');names=['fire_0','fire_1','fire_2','fire_3','sleepingbag','bedroll','meat','kit']
for i,n in enumerate(names):
 x=i%4;y=i//4;pack(im.crop((round(x*im.width/4),round(y*im.height/2),round((x+1)*im.width/4),round((y+1)*im.height/2))),n,(48,48))
headers.append('constexpr const uint16_t* fires[]={fire_0,fire_1,fire_2,fire_3};')
for n,label,size in [('bola de fogo.gif','fireball',(40,40)),('raio 3.gif','lightning',(80,112))]:
 gif=Image.open(base/n);frames=[]
 # Black is a transparent key in the supplied effect GIFs. Keep the full common
 # frame canvas, so animation does not jump as individual shapes change.
 choices=list(range(gif.n_frames))
 if label=='lightning':
  choices=[]
  for i in range(gif.n_frames):
   gif.seek(i)
   if gif.convert('L').point(lambda x:255 if x>48 else 0).histogram()[255]>10000:choices.append(i)
 for i in range(12):
  gif.seek(choices[round(i*(len(choices)-1)/11)]);frame=gif.convert('RGBA');pixels=[(r,g,b,0 if max(r,g,b)<24 else a) for r,g,b,a in frame.getdata()];frame.putdata(pixels);frames.append(frame)
 bounds=[im.getchannel('A').getbbox() for im in frames];bounds=[b for b in bounds if b];box=(min(b[0] for b in bounds),min(b[1] for b in bounds),max(b[2] for b in bounds),max(b[3] for b in bounds))
 for i,im in enumerate(frames):pack(im.crop(box),f'{label}_{i}',size,False)
 headers.append('constexpr const uint16_t* '+label+'[]={'+','.join(f'{label}_{i}' for i in range(12))+'};')
headers.append('}');(root/'firmware/RPG_POKET_2/CampArt.h').write_text('\n'.join(headers))
(base/'manifest.json').write_text(json.dumps({'count':len(entries),'flash_bytes':sum(e['bytes'] for e in entries),'assets':entries,'sources':'Built-in imagegen originals plus user-provided bola de fogo.gif and raio 3.gif'},indent=2))
preview=Image.new('RGB',(640,420),'#121823')
for i,n in enumerate(['hero_0_0_0','hero_0_1_1','hero_1_1_1','hero_2_1_1','hero_3_1_1','sleepingbag','fire_1','fireball_6','lightning_6']):
 im=Image.open(base/(n+'.png'));im=ImageOps.contain(im,(110,125),Image.Resampling.NEAREST);preview.paste(im,((i%5)*125,(i//5)*180),im)
preview.save(base/'previa-assets.png');print('Packed',len(entries),'assets;',sum(e['bytes'] for e in entries),'flash bytes')
