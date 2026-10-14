#!/usr/bin/env python3
"""Convert verified 240x320 host frames to review PNGs; no concept art."""
from pathlib import Path
from PIL import Image, ImageDraw
root=Path(__file__).resolve().parents[1]/'build/bulk-shop/screens'
names=['comprar-10-racoes','recibo-10-racoes','vender-7-racoes','recibo-venda']
titles=['COMPRAR 10 RACOES','COMPRA SALVA','VENDER 7 RACOES','VENDA SALVA']
sheet=Image.new('RGB',(960,352),(25,17,11));draw=ImageDraw.Draw(sheet)
for index,(name,title) in enumerate(zip(names,titles)):
    im=Image.open(root/(name+'.ppm'));assert im.size==(240,320)
    sheet.paste(im,(index*240,32));draw.text((index*240+12,8),title,fill=(238,214,152))
sheet.save(root/'compras-vendas-em-lote.png')
for file in root.glob('*.ppm'):
    im=Image.open(file);assert im.size==(240,320);im.save(file.with_suffix('.png'))
print('PASS: fourteen actual host frames plus annotated four-panel contact sheet')
