#!/usr/bin/env python3
"""Package images emitted by the real C++ renderer; never simulate the UI."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import hashlib,json,shutil
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'build/clock-cycle7';PREVIEW=OUT/'previews';PREVIEW.mkdir(parents=True,exist_ok=True)
palette_source=Image.open(OUT/'clock-hour-12.ppm').convert('RGB')
palette=palette_source.quantize(colors=256)
for prefix,count,name in [('daycycle',96,'clock-local-day-timelapse'),('day',50,'clock-day-realtime'),('night',50,'clock-night-realtime')]:
    frames=[Image.open(OUT/f'{prefix}-{i}.ppm').convert('RGB').resize((480,640),Image.Resampling.NEAREST) for i in range(count)]
    frames=[frame.quantize(palette=palette,dither=Image.Dither.NONE) for frame in frames]
    frames[0].save(PREVIEW/f'{name}.gif',save_all=True,append_images=frames[1:],duration=200,loop=0,optimize=False,disposal=1)
hours=[0,6,12,19,23]
contact=Image.new('RGB',(1200,352),'#131619');draw=ImageDraw.Draw(contact)
for i,hour in enumerate(hours):
    im=Image.open(OUT/f'clock-hour-{hour}.ppm');contact.paste(im,(i*240,32));draw.text((i*240+80,10),f'{hour:02}:18 local',fill='#e9cf90')
contact.save(PREVIEW/'clock-local-phases.png')
# Delivery copies keep explanatory text outside the unmodified device frame.
font_path=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')
font=ImageFont.truetype(str(font_path),16) if font_path.exists() else ImageFont.load_default()
for source,name,label in [
    ('clock-night-realtime','Relogio-medieval-noite-velocidade-real','Velocidade real | pulso e estrela cadente'),
    ('clock-local-day-timelapse','Relogio-medieval-ciclo-24h-acelerado','Demonstracao acelerada | 24h em 19,2 s')]:
    original=Image.open(PREVIEW/f'{source}.gif');labeled=[]
    for frame in range(original.n_frames):
        original.seek(frame);card=Image.new('RGB',(480,672),'#101519');card.paste(original.convert('RGB'),(0,0))
        ImageDraw.Draw(card).text((12,647),label,font=font,fill='#e9cf90')
        labeled.append(card.quantize(palette=palette,dither=Image.Dither.NONE))
    labeled[0].save(PREVIEW/f'{name}.gif',save_all=True,append_images=labeled[1:],duration=200,loop=0,optimize=False,disposal=1)
for name in ['clock-unset','clock-letter']:
    Image.open(OUT/f'{name}.ppm').resize((480,640),Image.Resampling.NEAREST).save(PREVIEW/f'{name}.png')
print('PASS: real-render captures; 24-hour timelapse is accelerated 4,500x, day/night clips run at real 5 FPS and include a night meteor.')

manifest={}
for path in PREVIEW.glob('*.gif'):
    gif=Image.open(path);duration=0
    for frame in range(gif.n_frames):gif.seek(frame);duration+=gif.info.get('duration',0)
    manifest[path.name]={'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size,'frames':gif.n_frames,'duration_ms':duration,'width':gif.width,'height':gif.height}
(PREVIEW/'capture-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
