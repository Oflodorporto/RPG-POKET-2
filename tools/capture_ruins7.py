"""Preserve actual 240x320 native framebuffer pixels, with exact nearest 3x review."""
from pathlib import Path
from PIL import Image
p=Path('build/ruins7');assert p.exists()
for file in sorted(p.glob('ruins7-*.ppm')):
 image=Image.open(file);assert image.size==(240,320)
 image.save(file.with_suffix('.png'))
 image.resize((720,960),Image.Resampling.NEAREST).save(file.with_name(file.stem+'-3x.png'))
for period in ['day','night']:
 composite=Image.new('RGB',(1456,960),(9,15,27))
 for n in range(2):
  original=Image.open(p/f'ruins7-{n+1}-{period}.ppm');panel=original.resize((720,960),Image.Resampling.NEAREST)
  composite.paste(panel,(n*736,0));assert composite.crop((n*736,0,n*736+720,960)).tobytes()==panel.tobytes()
 composite.save(p/('ruins7-two-pages-medieval-render.png' if period=='day' else 'ruins7-two-pages-night-render.png'))
print('PASS: actual 240x320 captures and exact 3x two-page day/night review; no synthesized UI previews')
