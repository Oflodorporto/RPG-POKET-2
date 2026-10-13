#!/usr/bin/env python3
"""Reduced, palette-preserving variants for FUTURE travel-map overlays.
Run after generate_effects.py: python3 source/generate_map_variants.py OUTPUT
"""
from pathlib import Path
from PIL import Image, ImageDraw
import json, sys, hashlib, shutil
from generate_effects import PALETTES, DURS, TITLES, rgba, font, savegif
OUT=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[1]/'pack'
NAMES=['campfire','healing','arcane']

def reduce(im,name):
 # Area coverage preserves one-pixel lines that nearest-only reduction drops.
 # RGB is returned to the exact source ramp, and alpha is quantized to 17 steps.
 small=im.resize((32,32),Image.Resampling.BOX);colors=[rgba(c)[:3] for c in PALETTES[name]];pixels=[]
 for r,g,b,a in small.getdata():
  a=round(a/17)*17
  if a<17:pixels.append((0,0,0,0));continue
  c=min(colors,key=lambda c:(c[0]-r)**2+(c[1]-g)**2+(c[2]-b)**2)
  pixels.append((*c,a))
 small.putdata(pixels);return small

def main():
 records=[];imgs={}
 (OUT/'map32').mkdir(exist_ok=True)
 for name in NAMES:
  dest=OUT/'map32'/name;dest.mkdir(exist_ok=True);atlas=Image.new('RGBA',(256,32));frames=[];entries=[]
  for i in range(8):
   im=reduce(Image.open(OUT/'frames'/name/f'{i:02}.png').convert('RGBA'),name);im.save(dest/f'{i:02}.png');atlas.paste(im,(i*32,0));frames.append(im)
   entries.append({'index':i,'file':f'map32/{name}/{i:02}.png','rect':{'x':i*32,'y':0,'w':32,'h':32},'duration_ms':DURS[name][i],'alpha_bbox':im.getchannel('A').getbbox(),'sha256':hashlib.sha256((dest/f'{i:02}.png').read_bytes()).hexdigest()})
  atlas.save(dest/'atlas.png');anchor=[16,26] if name=='campfire' else [16,16]
  meta={'id':name+'_map32','base_effect':name,'native_size':[32,32],'frame_count':8,'atlas':f'map32/{name}/atlas.png','atlas_size':[256,32],'anchor_px':anchor,'duration_ms':sum(DURS[name]),'loop':name=='campfire','playback':'loop' if name=='campfire' else 'one-shot','alpha':'straight RGBA; 17-step coverage alpha; zero RGB where alpha=0','sampling':'nearest','frames':entries,'status':'future-use asset, not integrated','map_guidance':'Reserve an unobstructed 32x32 box. Keep at least 6 px from label, route or city-marker bounds. Suppress the effect if no safe placement exists; do not move map labels.'}
  (dest/'metadata.json').write_text(json.dumps(meta,ensure_ascii=False,indent=2)+'\n');records.append(meta);imgs[name]=frames
 manifest={'title':'RPG POKET — variantes de mapa 32x32','variant_count':3,'frame_count':24,'target':'future travel map use, no runtime change','effects':records}
 (OUT/'map32'/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
 sheet=Image.new('RGB',(848,300),'#121b28');d=ImageDraw.Draw(sheet);d.text((24,18),'MAPA DE VIAGEM  /  VARIANTES 32×32',font=font(20),fill='#e2cf99');d.text((24,47),'Escala 4× na prévia  ·  uso futuro  ·  nenhuma integração no jogo',font=font(12),fill='#9eadba')
 def card(n,im):
  x=24+n*276;tile=Image.new('RGBA',(64,48),'#253631');td=ImageDraw.Draw(tile)
  for yy in range(0,48,8):
   for xx in range(0,64,8):
    if (xx//8+yy//8)%2:td.rectangle((xx,yy,xx+7,yy+7),fill='#2c3e35')
  tile.alpha_composite(im,(16,8));return tile.convert('RGB').resize((256,192),Image.Resampling.NEAREST),x
 for n,name in enumerate(NAMES):
  c,x=card(n,imgs[name][3]);sheet.paste(c,(x,76));d.text((x+7,278),TITLES[name],font=font(12),fill='#e4d6b0')
 sheet.save(OUT/'map32'/'RPG_POKET_Efeitos01_mapa32.png')
 animation=[]
 for tick in range(32):
  canvas=sheet.copy();ms=tick*100
  for n,name in enumerate(NAMES):
   total=sum(DURS[name]);pos=ms%(total if name=='campfire' else total+750);idx=None;elapsed=0
   for k,dt in enumerate(DURS[name]):
    elapsed+=dt
    if pos<elapsed:idx=k;break
   frame=imgs[name][idx] if idx is not None else Image.new('RGBA',(32,32));c,x=card(n,frame);canvas.paste(c,(x,76))
  animation.append(canvas)
 savegif(animation,OUT/'map32'/'RPG_POKET_Efeitos01_mapa32.gif',[100]*32)
 
 if Path(__file__).resolve() != (OUT/'source'/'generate_map_variants.py').resolve():shutil.copy2(__file__,OUT/'source'/'generate_map_variants.py');print('Generated map variants: 3 families, 24 frames at 32x32.')
if __name__=='__main__':main()
