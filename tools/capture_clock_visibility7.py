#!/usr/bin/env python3
"""Package real C++ clock captures; do not simulate UI or physical backlight."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import hashlib, json
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/clock-cycle7'; DEST=OUT/'visibility-preview'; DEST.mkdir(parents=True,exist_ok=True)
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',16)
small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',13)
# One palette covers the full breath, so quantization cannot flatten its light.
frames=[Image.open(OUT/f'morning-realtime-{i}.ppm').convert('RGB') for i in range(60)]
sample=Image.new('RGB',(240*6,320))
for i,j in enumerate([0,5,10,15,20,25]):sample.paste(frames[j],(i*240,0))
palette=sample.quantize(colors=256)
cards=[]
for frame in frames:
    card=Image.new('RGB',(480,688),'#101519')
    card.paste(frame.resize((480,640),Image.Resampling.NEAREST),(0,0))
    d=ImageDraw.Draw(card)
    d.text((12,646),'Velocidade real | 12 s | 5 FPS',font=font,fill='#e9cf90')
    d.text((12,669),'Previa do render; brilho fisico depende da tela.',font=small,fill='#c4c7c9')
    cards.append(card.quantize(palette=palette,dither=Image.Dither.NONE))
name='Relogio-pulso-manha-velocidade-real-12s.gif'
cards[0].save(DEST/name,save_all=True,append_images=cards[1:],duration=200,loop=0,optimize=False,disposal=1)
phases=[('08','Manha | 08:33'),('12','Meio-dia | 12:00'),('17','Tarde | 17:00'),('23','Noite | 23:00')]
contact=Image.new('RGB',(960,382),'#101519');d=ImageDraw.Draw(contact)
for i,(key,label) in enumerate(phases):
    contact.paste(Image.open(OUT/f'visibility-phase-{key}.ppm'),(i*240,31))
    d.text((i*240+12,8),label,font=font,fill='#e9cf90')
d.text((12,360),'Mesmo render de 240 x 320; cores mudam gradualmente pela hora local. Arte e backlight preservados.',font=small,fill='#c4c7c9')
contact.save(DEST/'Relogio-luz-manha-meiodia-tarde-noite.png')
comparison=Image.new('RGB',(960,680),'#101519');d=ImageDraw.Draw(comparison)
for i,(key,label) in enumerate([('low','Inicio do pulso'),('high','Pico do pulso')]):
    comparison.paste(Image.open(OUT/f'visibility-pulse-{key}.ppm').resize((480,640),Image.Resampling.NEAREST),(i*480,0))
    d.text((i*480+12,651),label+' | 08:33:13',font=font,fill='#e9cf90')
comparison.save(DEST/'Relogio-pulso-inicio-e-pico.png')
manifest={}
for path in sorted(DEST.iterdir()):
    if path.suffix not in ('.gif','.png'):continue
    im=Image.open(path);duration=0
    for f in range(getattr(im,'n_frames',1)):im.seek(f);duration+=im.info.get('duration',0)
    manifest[path.name]={'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size,'width':im.width,'height':im.height,'frames':getattr(im,'n_frames',1),'duration_ms':duration}
# GIF may merge identical adjacent captures, preserving their 200 ms timing.
assert manifest[name]['duration_ms']==12000 and 55<=manifest[name]['frames']<=60
(DEST/'capture-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('PASS: 12 s normal-speed morning GIF and real-render local-time/pulse stills; no simulated device brightness.')
