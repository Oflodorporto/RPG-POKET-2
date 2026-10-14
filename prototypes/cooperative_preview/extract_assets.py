from pathlib import Path
from PIL import Image,ImageDraw
import re,struct,zlib,hashlib,json
ROOT=Path(__file__).resolve().parents[3]
SRC=ROOT/'clock9-release-work'
FW=SRC/'firmware/RPG_POKET_2'
OUT=ROOT/'output/coop-preview'
meta=[tuple(map(int,x.split(','))) for x in re.findall(r'^\{([0-9,]+)\},?$',(FW/'AssetCatalog.h').read_text(),re.M)]
pack=(SRC/'cartao/RPGPOKET/artes.pak').read_bytes(); magic,n,crc,count=struct.unpack('<4sIII',pack[:16]); data=pack[16:]
assert magic==b'PKA1' and n==len(data) and crc==zlib.crc32(data) and count==len(meta)==666
USED=set()
def raw(i):
 USED.add(i);o,w,h,_,_,_,e=meta[i]; return data[o:o+w*h*e]
def rgb(c):return ((c>>11)*255//31,((c>>5)&63)*255//63,(c&31)*255//31)
def direct(i):
 _,w,h,*_=meta[i]; a=struct.unpack('<'+'H'*(w*h),raw(i));im=Image.new('RGBA',(w,h)); im.putdata([(*rgb(c),0 if c==0xf81f else 255) for c in a]);return im

def indexed(i,p):
 _,w,h,*_=meta[i];pal=struct.unpack('<'+'H'*(len(raw(p))//2),raw(p));im=Image.new('RGB',(w,h));im.putdata([rgb(pal[x]) for x in raw(i)]);return im

def enemy(species,pose=0):
 i=92+species*4+pose;p=600+species;pal=struct.unpack('<16H',raw(p)); pixels=[]
 for v in raw(i):
  for j in [v>>4,v&15]:pixels.append((*rgb(pal[j]),0 if pal[j]==0xf81f else 255))
 im=Image.new('RGBA',(80,86));im.putdata(pixels);return im
if __name__=='__main__':
 canvas=Image.new('RGB',(1000,650),'#182322');d=ImageDraw.Draw(canvas)
 for j,i in enumerate([57,59,61,63]):
  bg=indexed(i,i-1).resize((240,320),Image.Resampling.NEAREST);canvas.paste(bg,(j*250,0)); d.text((j*250+8,8),str(i),fill='red')
 for j,i in enumerate([64,65,66,67,68,69,78]):
  im=direct(i);canvas.paste(im,(j*140,340),im);d.text((j*140+8,468),str(i),fill='white')
 for j in range(5):
  im=enemy(j);canvas.paste(im,(j*140,495),im);d.text((j*140,590),str(j),fill='white')
 canvas.save(OUT/'assets-contact.png')
 print('Verified PKA1 CRC',hex(crc),len(meta),'assets')

# Fixed race/class/pose mapping, copied by parsing the production source.
race_text=(FW/'RaceArt.h').read_text()
def table(name):
 s=re.search(name+r'\[4\]\[4\]\[7\]=(.*?);',race_text).group(1)
 return json.loads(s.replace('{','[').replace('}',']'))
RACES=table('raceAsset');MASKS=table('raceMask')
OUTFIT=[0,0x001f,0xf800,0x07e0,0xffe0,0x781f,0xef7d,0x4208]
def hero(a,pose=0):
 i=RACES[a['race']][a['class']][pose];m=MASKS[a['race']][a['class']][pose]
 _,w,h,*_=meta[i];pix=struct.unpack('<'+'H'*(w*h),raw(i));mask=raw(m);pixels=[]
 for c,k in zip(pix,mask):
  sel=a['shirt'] if k==1 else a['trousers'] if k==2 else 0
  if sel and c!=0xf81f:
   color=OUTFIT[sel%8];light=max((c>>11)*255//31,((c>>5)&63)*255//63,(c&31)*255//31)
   c=(((color>>11)*light//255)<<11)|((((color>>5)&63)*light//255)<<5)|((color&31)*light//255)
  pixels.append((*rgb(c),0 if c==0xf81f else 255))
 im=Image.new('RGBA',(w,h));im.putdata(pixels);return im
