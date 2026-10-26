#!/usr/bin/env python3
"""Build a contact sheet from the actual quantized PNG exports (optional Pillow)."""
import argparse
import json
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont
from fetch_public_api_icons import BASE


def font(size, bold=False):
    name='DejaVuSans-Bold.ttf' if bold else 'DejaVuSans.ttf'
    return ImageFont.truetype('/usr/share/fonts/truetype/dejavu/'+name,size)


def generate(output):
    lock=json.loads((BASE/'source-lock.json').read_text())
    bg='#101720'; panel='#1a2531'; ink='#eadfc8'; muted='#9caab7'; line='#344353'
    page=Image.new('RGB',(1040,950),bg); d=ImageDraw.Draw(page)
    d.text((32,22),'RPG POKET  /  ÍCONES OFFLINE',font=font(29,True),fill=ink)
    d.text((32,67),'16 símbolos • equipamento, acampamento e magia • proposta isolada',font=font(17),fill=muted)
    d.text((32,94),'Pixels reais do export indexed4/RGB565. 24 px e 32 px ao lado da ampliação 3×.',font=font(15),fill=muted)
    for index,row in enumerate(lock['icons']):
        x=24+(index%4)*254;y=137+(index//4)*177
        d.rounded_rectangle((x,y,x+242,y+165),radius=10,fill=panel,outline=line)
        d.text((x+12,y+10),row['label_pt_br'],font=font(17,True),fill=ink)
        icon32=Image.open(BASE/f"generated/{row['slug']}-32.png").convert('RGBA')
        icon24=Image.open(BASE/f"generated/{row['slug']}-24.png").convert('RGBA')
        big=icon32.resize((96,96),Image.Resampling.NEAREST)
        page.paste(big,(x+9,y+39),big)
        page.paste(icon24,(x+131,y+66),icon24);page.paste(icon32,(x+187,y+60),icon32)
        d.text((x+127,y+105),'24 px',font=font(11),fill=muted);d.text((x+183,y+105),'32 px',font=font(11),fill=muted)
        d.text((x+12,y+143),row['author']+' · CC BY 3.0',font=font(11),fill=muted)
    d.text((32,865),'12.832 bytes de arte + até 2.048 bytes de buffer reutilizável. Nenhuma rede durante o jogo.',font=font(16),fill=ink)
    d.text((32,895),'Game-icons.net / Iconify. Cores, contornos e conversão adaptados para o protótipo.',font=font(14),fill=muted)
    d.text((32,919),'A proposta não foi aplicada às telas atuais nem publicada. Crédito completo acompanha os arquivos.',font=font(13),fill=muted)
    output.mkdir(parents=True,exist_ok=True)
    page.save(output/'contact-sheet.png')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=BASE/'preview')
    generate(parser.parse_args().output)
