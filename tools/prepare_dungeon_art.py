"""Pack original imagegen atlases; never writes to a card or changes existing art."""
from pathlib import Path
from PIL import Image,ImageOps,ImageDraw
import numpy as np
import json,hashlib,struct
root=Path(__file__).resolve().parents[1];base=root/'assets/dungeon1';base.mkdir(parents=True,exist_ok=True)
entries=[];headers=['#pragma once','#include <stdint.h>','namespace dungeonArt {'];preview=Image.new('RGB',(640,620),'#121823');draw=ImageDraw.Draw(preview)
def save(im,name,size,transparent=True):
    if transparent:
        bounds=im.getchannel('A').getbbox();assert bounds,name
        im=im.crop(bounds);im=ImageOps.contain(im,(size[0]-4,size[1]-4),Image.Resampling.NEAREST)
        out=Image.new('RGBA',size);out.alpha_composite(im,((size[0]-im.width)//2,size[1]-im.height-2))
    else:out=im.convert('RGBA').resize(size,Image.Resampling.NEAREST)
    out.save(base/(name+'.png'))
    vals=[0xf81f if a<128 else ((r>>3)<<11)|((g>>2)<<5)|(b>>3) for r,g,b,a in out.getdata()]
    headers.append('constexpr uint16_t '+name+'[]={');headers.extend(','.join(map(str,vals[i:i+24]))+',' for i in range(0,len(vals),24));headers.append('};')
    entries.append(dict(name=name,width=size[0],height=size[1],transparent=transparent,sha256=hashlib.sha256((base/(name+'.png')).read_bytes()).hexdigest(),bytes_rgb565=len(vals)*2))
    return out
def cell(im,col,row,cols,rows):return im.crop((round(col*im.width/cols),round(row*im.height/rows),round((col+1)*im.width/cols),round((row+1)*im.height/rows)))
im=Image.open(base/'enemies-original.png').convert('RGBA');enemy=['skeleton','spectre','warden','archon']
# Generated enemy rows are uneven and two poses touch. Extract alpha components
# instead of slicing uniform cells, so neighboring heads/weapons cannot leak.
data=np.array(im);alpha=data[:,:,3];seen=alpha<=100;parts=[];h,w=alpha.shape
for y,x in zip(*np.where(~seen)):
    if seen[y,x]:continue
    stack=[(int(y),int(x))];seen[y,x]=True;pts=[]
    while stack:
        py,px=stack.pop();pts.append((py,px))
        for dy,dx in ((0,1),(0,-1),(1,0),(-1,0),(1,1),(1,-1),(-1,1),(-1,-1)):
            ny,nx=py+dy,px+dx
            if 0<=ny<h and 0<=nx<w and not seen[ny,nx]:seen[ny,nx]=True;stack.append((ny,nx))
    if len(pts)>3000:
        if max(y for y,x in pts)-min(y for y,x in pts)>h*.45:
            parts.extend([[p for p in pts if p[0]<1020],[p for p in pts if p[0]>=1020]])
        else:parts.append(pts)
assert len(parts)==16,len(parts)
parts.sort(key=lambda pts:(min(3,int(np.mean([p[0] for p in pts])/360)),np.mean([p[1] for p in pts])))
enemy_frames=[]
for pts in parts:
    ys,xs=zip(*pts);box=(min(xs),min(ys),max(xs)+1,max(ys)+1);crop=data[box[1]:box[3],box[0]:box[2]].copy();mask=np.zeros(crop.shape[:2],dtype=bool)
    for y,x in pts:mask[y-box[1],x-box[0]]=True
    crop[~mask,3]=0;enemy_frames.append(Image.fromarray(crop))
for row,name in enumerate(enemy):
    for col in range(4):
        out=save(enemy_frames[row*4+col],f'{name}_{col}',(48,64));p=out.resize((96,128),Image.Resampling.NEAREST);preview.paste(p,(col*100,row*132),p)
headers.append('constexpr const uint16_t* enemies[4][4]={'+','.join('{'+','.join(f'{n}_{i}' for i in range(4))+'}' for n in enemy)+'};')
props=['coins','gold','chest','chest_open','crystal','seal','life','mana','torch','stairs_up','stairs_down','relic'];im=Image.open(base/'props-original.png').convert('RGBA')
for i,name in enumerate(props):
    out=save(cell(im,i%4,i//4,4,3),name,(32,32));p=out.resize((64,64),Image.Resampling.NEAREST);preview.paste(p,(408+(i%3)*76,(i//3)*100+25),p);draw.text((408+(i%3)*76,(i//3)*100+90),name,fill='white')
headers.append('constexpr const uint16_t* props[]={'+','.join(props)+'};')
for floor in range(2):
    im=Image.open(base/f'floor{floor+1}-original.png').convert('RGBA')
    for i,name in enumerate(['wall','floor','ceiling','door']):save(cell(im,i%2,i//2,2,2),f'f{floor}_{name}',(32,32),False)
headers.append('constexpr const uint16_t* textures[2][4]={{f0_wall,f0_floor,f0_ceiling,f0_door},{f1_wall,f1_floor,f1_ceiling,f1_door}};')
im=Image.open(base/'weapons-original.png').convert('RGBA')
for cls in range(4):
    for pose in range(2):save(cell(im,cls,pose,4,2),f'weapon_{cls}_{pose}',(64,52))
headers.append('constexpr const uint16_t* weapons[4][2]={'+','.join('{weapon_%d_0,weapon_%d_1}'%(c,c) for c in range(4))+'};')
headers.append('}')
(root/'firmware/RPG_POKET_2/DungeonArt.h').write_text('\n'.join(headers),encoding='utf8')
(base/'manifest.json').write_text(json.dumps({'assets':entries,'count':len(entries),'bytes_rgb565':sum(e['bytes_rgb565'] for e in entries),'source':'Built-in imagegen; original atlases preserved; RGB565 transparent key 0xf81f'},indent=2),encoding='utf8')
preview.save(base/'previa-assets.png');print('PASS:',len(entries),'assets packed',sum(e['bytes_rgb565'] for e in entries),'bytes RGB565')
