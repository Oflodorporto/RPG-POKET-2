#!/usr/bin/env python3
"""Package pixels from the firmware renderer, with labels outside device frames."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import hashlib, json, sys
root=Path(sys.argv[1]); source=root/'screens'; out=root/'previews'; out.mkdir(parents=True,exist_ok=True)
# Sanitized and optimized builds must produce exactly the same native pixels.
verified=0
for path in sorted(source.glob('*.ppm')):
    counterpart=root/'sanitized'/path.name
    assert counterpart.is_file() and counterpart.read_bytes()==path.read_bytes(), f'Capture mismatch: {path.name}'
    verified+=1
assert verified>500
font_path=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')
font=ImageFont.truetype(str(font_path),15) if font_path.exists() else ImageFont.load_default()
classes=['Mago','Paladino','Guerreiro','Barbaro']; actions=['Ataque','Ofensiva','Defesa']
def card(images, labels, title):
    sheet=Image.new('RGB',(240*len(images),358),'#101820'); draw=ImageDraw.Draw(sheet)
    draw.text((8,3),title,fill='#e9cf90',font=font)
    for i,(im,label) in enumerate(zip(images,labels)):
        sheet.paste(im,(i*240,38));draw.text((i*240+8,20),label,fill='#e7eef0',font=font)
    return sheet
for path in source.glob('*.ppm'):
    if '-frame-' not in path.name and not path.name.startswith('idle-'):
        Image.open(path).save(out/(path.stem+'.png'))
card([Image.open(source/f'perspective-{side}-start.ppm') for side in range(2)],['Placa A: Orc / Barbaro','Placa B: Elfo / Mago'],'Mesma partida: duas perspectivas').save(out/'duas-perspectivas.png')
card([Image.open(source/name) for name in ['insufficient-mana-visible.ppm','guard-persistent.ppm','unacknowledged-action.ppm']],['Mana insuficiente','Guarda apos animacao','Acao aguardando ACK'],'Interface real: recursos e confirmacao').save(out/'feedback-e-confirmacao.png')
sheet=Image.new('RGB',(960,4*348+30),'#101820');draw=ImageDraw.Draw(sheet);draw.text((8,5),'Classes e racas: pixels reais, identidade mantida',font=font,fill='#e9cf90')
for cls in range(4):
    for race in range(4):
        x,y=race*240,30+cls*348;draw.text((x+8,y+4),f'{classes[cls]} / {("Humano","Elfo","Anao","Orc")[race]}',font=font,fill='#e7eef0');sheet.paste(Image.open(source/f'matrix-class-{cls}-race-{race}.ppm'),(x,y+28))
sheet.save(out/'matriz-classes-racas.png')
for cls in range(4):
    for action in range(3):
        frames=[]
        for frame in range(23):
            images=[Image.open(source/f'class-{cls}-action-{action}-side-{side}-frame-{frame}.ppm').convert('RGB') for side in range(2)]
            frames.append(card(images,['Ator local','Mesmo ator visto pelo rival'],f'{classes[cls]}: {actions[action]} | amostra nominal 60 ms'))
        palette=frames[10].quantize(colors=256)
        indexed=[frame.quantize(palette=palette,dither=Image.Dither.NONE) for frame in frames]
        indexed[0].save(out/f'{classes[cls].lower()}-{actions[action].lower()}-duas-perspectivas.gif',save_all=True,append_images=indexed[1:],duration=60,loop=0,optimize=False,disposal=1)
# Idle intentionally has identical pixels; a separate PNG states the stable state.
Image.open(source/'idle-0.ppm').save(out/'idle-pose-neutra.png')
manifest={}
for path in sorted(out.iterdir()):
    if path.suffix not in ('.png','.gif'):continue
    image=Image.open(path);duration=0
    for frame in range(getattr(image,'n_frames',1)):
        image.seek(frame);duration+=image.info.get('duration',0)
    manifest[path.name]={'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size,'width':image.width,'height':image.height,'frames':getattr(image,'n_frames',1),'duration_ms':duration}
(root/'capture-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(f'PASS: {verified} optimized/sanitized captures byte-identical; 12 paired-perspective GIFs; nominal 60 ms sampling, not measured hardware FPS.')
