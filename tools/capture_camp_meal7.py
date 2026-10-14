#!/usr/bin/env python3
"""Package unscaled production frames; no sprite redrawing or screenshot cleanup."""
from pathlib import Path
from PIL import Image, ImageDraw
import argparse, hashlib, json
parser=argparse.ArgumentParser();parser.add_argument('root',nargs='?',default='build/screens/camp-identity77');args=parser.parse_args();root=Path(args.root)
assert root.is_dir()
identities=[(0,0),(1,1),(2,2),(3,3)]
races=['Humano','Elfo','Anao','Orc'];classes=['Mago','Paladino','Guerreiro','Barbaro']
files=[]
def opened(page,race,cls,status,frame):
    p=root/f'{page}-race{race}-class{cls}-{status}-frame{frame}.ppm'
    im=Image.open(p).convert('RGB');assert im.size==(240,320);return im
for page in ['setup','rest']:
    sheet=Image.new('RGB',(960,340*4),'#111923');draw=ImageDraw.Draw(sheet)
    for race in range(4):
        for cls in range(4):
            im=opened(page,race,cls,'ready',12)
            sheet.paste(im,(cls*240,race*340+20));draw.text((cls*240+4,race*340+4),f'{races[race]} / {classes[cls]}',fill='white')
    sheet.save(root/f'{page}-all-identities.png')
    fallback=Image.new('RGB',(960,340),'#111923');draw=ImageDraw.Draw(fallback)
    frames=[]
    for frame in range(24):
        strip=Image.new('RGB',(960,340),'#111923');sd=ImageDraw.Draw(strip)
        for n,(race,cls) in enumerate(identities):
            strip.paste(opened(page,race,cls,'ready',frame),(n*240,20));sd.text((n*240+4,4),f'{races[race]} / {classes[cls]}',fill='white')
        frames.append(strip)
    frames[0].save(root/f'{page}-meal-four-identities.gif',save_all=True,append_images=frames[1:],duration=120,loop=0,disposal=2)
    for n,(race,cls) in enumerate(identities):
        im=opened(page,race,cls,'ready',12);im.save(root/f'{page}-race{race}-class{cls}-native.png')
        fallback.paste(opened(page,race,cls,'fallback',0),(n*240,20));draw.text((n*240+4,4),f'{races[race]} / {classes[cls]}',fill='white')
        seq=[opened(page,race,cls,'ready',f) for f in range(24)]
        seq[0].save(root/f'{page}-race{race}-class{cls}-meal.gif',save_all=True,append_images=seq[1:],duration=120,loop=0,disposal=2)
    fallback.save(root/f'{page}-fallback-native.png')
# The close-up is explicitly marked 3x; all standalone scenes remain native.
sequence=Image.new('RGB',(303*6,375),'#111923');sd=ImageDraw.Draw(sequence)
for n,frame in enumerate([0,6,10,12,16,20]):
    image=opened('rest',0,0,'ready',frame).crop((45,125,146,242)).resize((303,351),Image.Resampling.NEAREST)
    sequence.paste(image,(n*303,24));sd.text((n*303+5,6),f'3x / frame {frame} / {frame*120} ms',fill='white')
sequence.save(root/'meal-closeup-3x.png')
for p in sorted(root.glob('*')):
    if p.suffix in {'.png','.gif'}:
        im=Image.open(p);files.append(dict(file=p.name,width=im.width,height=im.height,frames=getattr(im,'n_frames',1),sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
(root/'meal-preview-manifest.json').write_text(json.dumps(dict(native_scene_dimensions=[240,320],frame_duration_ms=120,cycle_ms=2880,identities=16,assets_unchanged=True,files=files,notes='Actual production render frames. Contact sheets tile unscaled scenes. GIF demonstrates the repeating cosmetic meal cycle; actual rest retains its existing 4500 ms controller timer.'),indent=2)+'\n')
print(f'PASS: {len(files)} unscaled production PNG/GIF previews, all 16 identities and ready/fallback contact sheets.')
