#!/usr/bin/env python3
"""RPG POKET / VFX 01. Original deterministic code-native pixel effects.
Requires Python 3.10+ and Pillow 9+. No downloaded artwork or image model output.
Run: python3 source/generate_effects.py [output_directory]
"""
from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import math, random, json, hashlib, sys, shutil
ROOT=Path(__file__).resolve().parents[1]
OUT=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else ROOT/'pack'
N=8; S=64; TAU=math.tau
PALETTES={
 'campfire':['#572428','#983324','#c74c26','#ef792b','#fca743','#ffd26a','#fff0b3'],
 'slash':['#345069','#577c99','#82bdd1','#b7dee0','#e6f3dc','#fff9d7'],
 'healing':['#173f38','#27664a','#3a9560','#63c886','#a3e5a3','#e0f6c8'],
 'fire_burst':['#52253d','#913244','#c64c36','#ea7834','#ffac45','#ffd881','#fff1c2'],
 'frost':['#293960','#385a84','#487fa7','#66b6c7','#a1dfdf','#e1f5e9'],
 'lightning':['#3e3065','#54498f','#6774b9','#84b8dc','#bbdeeb','#f0f5da'],
 'arcane':['#362b63','#5a408c','#8357af','#aa79cc','#d6ace4','#f4deef'],
 'guard':['#23435f','#37738b','#5aabb3','#91d4cf','#d5e9c1','#f7e4a0']}
TITLES={'campfire':'FOGUEIRA','slash':'CORTE / IMPACTO','healing':'CURA','fire_burst':'EXPLOSAO DE FOGO','frost':'GELO','lightning':'RAIO','arcane':'ENERGIA ARCANA','guard':'ESCUDO / GUARDA'}
NOTES={
 'campfire':'Chamas e brasas em loop, com lenha fixa. Recurso visual de acampamento.',
 'slash':'Arco de corte e faiscas de contato. Uso visual em ataques fisicos.',
 'healing':'Pulso verde de recuperacao. Referencias: Impor as maos, Segundo folego e Vigor.',
 'fire_burst':'Explosao compacta. Referencias: Maos flamejantes, Raios abrasadores e Bola de fogo.',
 'frost':'Cristais e particulas frias. Reserva visual futura; nao define magia ou mecanica atual.',
 'lightning':'Descarga ramificada, sem estroboscopio. Referencia visual: Raio e arte de raio existente.',
 'arcane':'Nucleo e arcos violetas. Referencia visual: Misseis magicos; sem trajetoria ou dano embutidos.',
 'guard':'Contorno de escudo e pulso turquesa/dourado. Referencias: Escudo arcano, Barreira, Escudo e Aparar.'}
DURS={'campfire':[120]*8,'slash':[80,80,90,100,110,110,120,130], 'healing':[110]*8,'fire_burst':[80,80,90,100,100,110,120,140], 'frost':[100]*8,'lightning':[90,90,100,110,110,120,130,150], 'arcane':[100]*8,'guard':[110]*8}

def rgba(c,a=255):
 if isinstance(c,tuple):return c[:3]+(max(0,min(255,round(a))),)
 return tuple(bytes.fromhex(c.lstrip('#')))+(max(0,min(255,round(a))),)
def pal(name,i,a=255):return rgba(PALETTES[name][i],a)
def layer():return Image.new('RGBA',(S,S))
def pt(x,y):return (round(x),round(y))
def polygon(d,ps,c):d.polygon([pt(*p) for p in ps],fill=c)
def line(d,ps,c,w=1):d.line([pt(*p) for p in ps],fill=c,width=w,joint='curve')
def ellipse(d,b,c,outline=None,width=1):d.ellipse(tuple(round(v) for v in b),fill=c,outline=outline,width=width)
def glow(im,x,y,r,c,power=1,sy=1):
 lay=layer();d=ImageDraw.Draw(lay)
 for rad,alpha in [(r,10),(r*.78,12),(r*.56,17),(r*.36,22)]:
  ellipse(d,(x-rad,y-rad*sy,x+rad,y+rad*sy),rgba(c,alpha*power))
 im.alpha_composite(lay)
def star(d,x,y,r,c):
 x,y=pt(x,y);r=max(1,round(r))
 if r==1:d.point((x,y),fill=c);return
 polygon(d,[(x,y-r),(x+1,y-1),(x+r,y),(x+1,y+1),(x,y+r),(x-1,y+1),(x-r,y),(x-1,y-1)],c)
def ring(d,cx,cy,rx,ry,c,w=1,start=0,end=TAU):
 pts=[(cx+rx*math.cos(start+(end-start)*i/80),cy+ry*math.sin(start+(end-start)*i/80)) for i in range(81)]
 line(d,pts,c,w)
def beam(d,points,colors,widths,alpha=1):
 for c,w in zip(colors,widths):line(d,points,rgba(c,255*alpha),w)
def fade(im,a):
 if a<.999:im.putalpha(im.getchannel('A').point(lambda v:round(v*a)))
 return im

def campfire(i):
 im=layer();t=i/N*TAU;glow(im,32,39,21,'#d17435',.85,.8);d=ImageDraw.Draw(im)
 # Dark stone bed, logs, exposed end grain. Stable prop geometry is intentional.
 for x,y,w,h in [(17,48,8,4),(24,51,8,3),(35,51,10,3),(43,47,7,4)]:
  ellipse(d,(x,y,x+w,y+h),rgba('#2e343b'));line(d,[(x+2,y),(x+w-2,y)],rgba('#62636a'))
 polygon(d,[(17,45),(20,42),(47,48),(45,53),(20,49)],rgba('#382b2b'))
 polygon(d,[(19,44),(43,49),(44,51),(20,47)],rgba('#83513b'))
 line(d,[(22,45),(39,49)],rgba('#bd7947'));ellipse(d,(42,47,48,53),rgba('#6c4936'));ellipse(d,(44,49,46,51),rgba('#c3935a'))
 polygon(d,[(20,49),(22,53),(46,45),(43,41)],rgba('#3c2a2b'))
 polygon(d,[(21,48),(43,42),(44,44),(23,51)],rgba('#9f663e'))
 line(d,[(25,49),(39,45)],rgba('#d99548'))
 # Interlocking flame tongues with coherent 960 ms oscillation.
 def tongue(cx,base,w,h,phase):
  bend=2.3*math.sin(t+phase);tipx=cx+2.5*math.sin(t+phase+.65);tipy=base-h-2*math.sin(t+phase)
  outer=[(cx-w,base-2),(cx-w-1,base-8),(cx-w+3,base-16),(cx-w+2,base-22),(cx-2,base-16),(tipx,tipy),(cx+3+bend,base-h*.56),(cx+6,base-h*.47),(cx+w,base-7),(cx+w-1,base-2)]
  polygon(d,outer,pal('campfire',1));inner=[(cx-w+2,base-3),(cx-w+1,base-10),(cx-2,base-15),(tipx,tipy+5),(cx+3+bend,base-h*.48),(cx+w-2,base-9),(cx+w-2,base-3)]
  polygon(d,inner,pal('campfire',3));polygon(d,[(cx-w+4,base-3),(cx-3,base-11),(cx+1+bend,base-20),(cx+2,base-11),(cx+w-4,base-5)],pal('campfire',5))
  polygon(d,[(cx-3,base-3),(cx-2,base-9),(cx+1,base-14),(cx+2,base-5)],pal('campfire',6))
 tongue(25,46,6,26,.7);tongue(38,46,7,29,2.7);tongue(32,48,9,35,0)
 for j in range(4):
  q=(i/8+j*.26)%1;x=23+j*5+math.sin(t+j)*2;y=31-q*25
  # Fade across wrap instead of popping bright embers.
  a=math.sin(math.pi*q)*200
  if a>25:line(d,[(x,y),(x,y+1)],pal('campfire',4,a))
 return im

def slash(i):
 im=layer();t=i/7;d=ImageDraw.Draw(im);energy=[.25,.6,1,1,.8,.56,.3,.09][i]
 glow(im,32,31,23,'#84b8d5',energy,.9)
 d=ImageDraw.Draw(im)
 head=min(1,.20+t*1.08);tail=max(0,head-(.76 if i<5 else .76-.035*(i-5)))
 def curve(q):return (10*(1-q)**2+2*18*(1-q)*q+54*q*q,48*(1-q)**2+2*10*(1-q)*q+15*q*q)
 for off,col,wid in [(1,0,1.0),(0,2,.76),(0,4,.36)]:
  a=[];b=[]
  for j in range(40):
   q=tail+(head-tail)*j/39;x,y=curve(q);dx=16*(1-q)+72*q;dy=-76*(1-q)+10*q;L=math.hypot(dx,dy);nx=-dy/L;ny=dx/L
   w=(math.sin(math.pi*j/39)**.7)*4.5*wid
   a.append((x+nx*w,y+ny*w+off));b.append((x-nx*w,y-ny*w+off))
  polygon(d,a+b[::-1],pal('slash',col,255*energy))
 if 1<=i<=5:
  r=[0,2,5,6,4,2,0,0][i];star(d,32,29,r,pal('slash',5,240*energy))
 for j,ang in enumerate([-.5,.25,1.2,2.3,3.9]):
  r=5+t*23;x=32+math.cos(ang)*r;y=30+math.sin(ang)*r*.7
  if 2<=i<=6:line(d,[(x,y),(x+math.cos(ang)*(3-i*.3),y+math.sin(ang)*(3-i*.3))],pal('slash',3,230*energy),1)
 return im

def healing(i):
 im=layer();t=i/7;e=[.16,.44,.77,1,.95,.7,.38,.12][i];glow(im,32,34,24,'#6ab576',e);d=ImageDraw.Draw(im)
 r=10+14*t;ring(d,32,46,r,r*.24,pal('healing',2,180*e),2);ring(d,32,45,r-2,r*.22,pal('healing',4,210*e))
 for j in range(3):
  pts=[]
  for k in range(28):
   q=k/27;x=32+math.sin(q*4.4+j*2.1+t*.8)*(10+q*4);y=44-q*(12+26*t)
   pts.append((x,y))
  line(d,pts,pal('healing',1,180*e),3);line(d,pts,pal('healing',3,235*e));
 for j in range(7):
  q=(j*.13+t*.64)%1;x=18+j*4.7+math.sin(j*2+t)*2;y=49-q*37
  aa=e*math.sin(math.pi*q)
  star(d,x,y,2+(j%3==0),pal('healing',5,240*aa))
 # A single central bud opens to a four-point healing glint, then fades.
 star(d,32,32-8*t,2+4*e,pal('healing',2,170*e));star(d,32,32-8*t,1+2*e,pal('healing',5,250*e))
 return im

def fire_burst(i):
 im=layer();t=i/7;e=[.18,.65,1,1,.83,.55,.29,.08][i];r=[4,9,16,20,23,24,23,22][i]
 glow(im,32,33,min(27,r+7),'#e36c39',e);d=ImageDraw.Draw(im)
 def contour(rad,phase,yoff=0):
  return [(32+math.cos(a)*(rad*(1+.13*math.sin(5*a+phase)+.09*math.sin(9*a-.4))),33+yoff+math.sin(a)*(rad*(1+.1*math.sin(7*a+.8)))) for a in [j*TAU/72 for j in range(72)]]
 # Ring dissipates after expansion; the center opens gradually, with no flash frame.
 if i<=4:
  polygon(d,contour(r,0),pal('fire_burst',1,250*e))
  polygon(d,contour(r*.82,.6),pal('fire_burst',3,255*e))
  polygon(d,contour(r*.59,.3,-1),pal('fire_burst',4,255*e))
  polygon(d,contour(r*.3,0,-2),pal('fire_burst',6,255*e))
 else:
  for j in range(10):
   a=j*TAU/10+.2;rad=r*(.83+.1*math.sin(j));x=32+math.cos(a)*rad;y=33+math.sin(a)*rad
   size=(8-i)*1.1
   polygon(d,[(x-size,y+size),(x-size,y-size/2),(x,y-size*2),(x+size*.7,y-size*.3),(x+size,y+size)],pal('fire_burst',2,245*e))
   star(d,x,y,max(1,size*.55),pal('fire_burst',4,230*e))
 for j in range(8):
  a=j*TAU/8+.35;rad=min(28,r+5);x=32+math.cos(a)*rad;y=33+math.sin(a)*rad
  if i>0:line(d,[(x,y),(x+math.cos(a)*1.5,y+math.sin(a)*1.5)],pal('fire_burst',5,210*e))
 return im

def frost(i):
 im=layer();t=i/7;e=[.18,.5,.82,1,1,.7,.36,.1][i];growth=[.1,.35,.72,1,1,1,1,1][i]
 glow(im,32,39,25,'#70bfd3',e,.8);d=ImageDraw.Draw(im);ring(d,32,49,11+13*t,3+3*t,pal('frost',2,180*e));
 for j,(x,dx,h,w) in enumerate([(18,-6,19,4),(26,-3,30,5),(32,2,40,6),(39,6,30,5),(46,7,20,4)]):
  top=(x+dx*growth,48-h*growth);left=(x-w,47);right=(x+w,48);ridge=(x+dx*.5,47-h*growth*.7)
  polygon(d,[left,top,right,(x,52)],pal('frost',0,230*e));polygon(d,[(left[0]+1,46),top,ridge,(x,50)],pal('frost',2,255*e));polygon(d,[top,(right[0]-1,47),(x,50),ridge],pal('frost',4,255*e));line(d,[top,ridge,(x,49)],pal('frost',5,250*e))
 for j in range(7):
  a=j*TAU/7+.1;r=14+t*12;x=32+math.cos(a)*r;y=32+math.sin(a)*r*.7
  star(d,x,y,2,pal('frost',5,220*e))
 return im

def lightning(i):
 im=layer();t=i/7;e=[.18,.56,1,1,.91,.64,.33,.1][i];glow(im,32,33,22,'#8799e1',e);d=ImageDraw.Draw(im)
 p=[(31,7),(27,16),(36,20),(29,30),(37,33),(30,44),(33,52)]
 # Fixed path, one reveal and one decay; avoids frame-random strobing.
 reveal=[2,4,7,7,7,7,7,7][i];beam(d,p[:reveal],[PALETTES['lightning'][0],PALETTES['lightning'][2],PALETTES['lightning'][4],PALETTES['lightning'][5]],[7,5,3,1],e)
 if i>=2:
  beam(d,[(35,20),(44,19),(41,28),(48,33)],[PALETTES['lightning'][1],PALETTES['lightning'][3],PALETTES['lightning'][5]],[3,2,1],e*.8)
  beam(d,[(29,30),(20,29),(23,37),(16,41)],[PALETTES['lightning'][1],PALETTES['lightning'][3]],[3,1],e*.75)
 if i>=2:ring(d,33,53,6+(i-2)*3,2+(i-2)*.7,pal('lightning',3,200*e));star(d,33,51,2+3*e,pal('lightning',5,240*e))
 return im

def arcane(i):
 im=layer();t=i/7;e=[.18,.48,.8,1,.94,.69,.37,.11][i];r=7+13*math.sin(math.pi*t/2);glow(im,32,32,26,'#9973cf',e);d=ImageDraw.Draw(im)
 for j in range(3):
  a=j*TAU/3+t*1.25
  ring(d,32,32,r,r*.7,pal('arcane',1,220*e),3,a,a+1.35)
  ring(d,32,31,r,r*.7,pal('arcane',3,245*e),1,a,a+1.35)
  x=32+math.cos(a+1.35)*r;y=31+math.sin(a+1.35)*r*.7;star(d,x,y,3,pal('arcane',5,245*e))
 rr=3+e*5
 polygon(d,[(32,32-rr*1.6),(32+rr,32),(32,32+rr*1.6),(32-rr,32)],pal('arcane',1,255*e))
 polygon(d,[(32,32-rr*1.6),(32,32+rr*1.6),(32-rr,32)],pal('arcane',3,250*e))
 polygon(d,[(32,32-rr),(32+rr*.5,32),(32,32+rr),(32-rr*.5,32)],pal('arcane',5,250*e))
 for j in range(5):
  a=j*TAU/5+.6;r2=14+t*12;star(d,32+math.cos(a)*r2,32+math.sin(a)*r2,1+(j%2),pal('arcane',4,190*e))
 return im

def guard(i):
 im=layer();t=i/7;e=[.15,.43,.77,1,1,.73,.4,.12][i];glow(im,32,32,26,'#66adb3',e);d=ImageDraw.Draw(im)
 scale=[.58,.75,.92,1,1,1,1,1][i]
 shape=[(32,11),(50,19),(47,37),(40,47),(32,54),(24,47),(17,37),(14,19)]
 shape=[(32+(x-32)*scale,32+(y-32)*scale) for x,y in shape]
 polygon(d,shape,pal('guard',0,55*e));line(d,shape+[shape[0]],pal('guard',0,210*e),5);line(d,shape+[shape[0]],pal('guard',2,245*e),3);line(d,shape+[shape[0]],pal('guard',4,250*e),1)
 inner=[(32+(x-32)*.76,32+(y-32)*.77) for x,y in shape];line(d,inner+[inner[0]],pal('guard',1,160*e))
 line(d,[(32,20),(32,45)],pal('guard',3,160*e));line(d,[(23,28),(41,28)],pal('guard',3,160*e))
 star(d,32,28,3+e,pal('guard',5,230*e))
 for j in range(3):
  a=[-.6,2.7,4.3][j];r=17+10*t;star(d,32+math.cos(a)*r,32+math.sin(a)*r,2,pal('guard',4,210*e))
 return im

FX={'campfire':campfire,'slash':slash,'healing':healing,'fire_burst':fire_burst,'frost':frost,'lightning':lightning,'arcane':arcane,'guard':guard}

def background(mode='ruins'):
 # Original low-contrast environment, deliberately subordinate to effect pixels.
 im=Image.new('RGB',(112,96),'#151c2c');d=ImageDraw.Draw(im);rng=random.Random(1989)
 if mode=='forest':
  for j in range(12):
   x=j*10+rng.randrange(-3,4);w=rng.randrange(4,8);d.rectangle((x,0,x+w,63),fill=['#202737','#242c38','#28303b'][j%3]);line(d,[(x+1,0),(x+1,53)],'#323941')
  for j in range(30):
   x=rng.randrange(112);y=rng.randrange(58);d.rectangle((x,y,x+5,y+2),fill=['#243540','#2b3943','#344047'][j%3])
 else:
  for y in range(0,64,16):
   for x in range(-12 if y//16%2 else -2,112,28):
    c=['#252c3b','#292f40','#2c3241'][rng.randrange(3)];d.rectangle((x,y,x+25,y+13),fill=c);line(d,[(x+2,y+1),(x+24,y+1)],'#343c4a');line(d,[(x,y+13),(x+25,y+13)],'#1a202e')
  for j in range(7):
   x=rng.randrange(112);y=rng.randrange(50);line(d,[(x,y),(x+3,y+4),(x,y+8)],'#171e2a')
 d.rectangle((0,62,112,96),fill='#2f3033')
 for j in range(44):
  x=rng.randrange(112);y=rng.randrange(64,96);line(d,[(x,y),(x+rng.randrange(2,9),y)],['#3c3b3a','#242a31','#49433c'][j%3])
 ellipse(d,(29,64,83,78),'#222832');return im

def composite(im,bg,scale=4):
 b=bg.convert('RGBA');b.alpha_composite(im,(24,12));return b.convert('RGB').resize((112*scale,96*scale),Image.Resampling.NEAREST)
def font(size):
 for p in ['/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf','/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf']:
  if Path(p).exists():return ImageFont.truetype(p,size)
 return ImageFont.load_default()
def global_pal(frames):
 strip=Image.new('RGB',(frames[0].width,frames[0].height*len(frames)))
 for i,f in enumerate(frames):strip.paste(f,(0,i*f.height))
 p=strip.quantize(colors=192,method=Image.Quantize.MEDIANCUT)
 return [f.quantize(palette=p,dither=Image.Dither.NONE) for f in frames]
def savegif(frames,path,durations):
 fs=global_pal(frames);fs[0].save(path,save_all=True,append_images=fs[1:],duration=durations,loop=0,disposal=2,optimize=False)

def main():
 OUT.mkdir(parents=True,exist_ok=True)
 for p in ['frames','atlases','previews','metadata','source']:(OUT/p).mkdir(exist_ok=True)
 allframes={};effects=[]
 for name,func in FX.items():
  dest=OUT/'frames'/name;dest.mkdir(exist_ok=True);frames=[];records=[];atlas=Image.new('RGBA',(S*N,S))
  for i in range(N):
   im=func(i).convert('RGBA') if (i<7 or name=='campfire') else layer()
   # Canonical hidden RGB: no dark/colored fringe from transparent texels.
   im.putdata([(r,g,b,a) if a else (0,0,0,0) for r,g,b,a in im.getdata()]);im.save(dest/f'{i:02d}.png');frames.append(im);atlas.paste(im,(i*S,0))
   records.append({'index':i,'file':f'frames/{name}/{i:02d}.png','rect':{'x':i*S,'y':0,'w':S,'h':S},'duration_ms':DURS[name][i],'alpha_bbox':im.getchannel('A').getbbox(),'sha256':hashlib.sha256((dest/f'{i:02d}.png').read_bytes()).hexdigest()})
  atlas.save(OUT/'atlases'/f'{name}.png')
  anchor=[32,52] if name in ['campfire','lightning'] else [32,32]
  meta={'id':name,'name_pt':TITLES[name],'native_size':[S,S],'frame_count':N,'atlas':f'atlases/{name}.png','atlas_size':[N*S,S],'layout':'row-major, left-to-right','anchor_px':anchor,'anchor_normalized':[anchor[0]/S,anchor[1]/S],'anchor_convention':'pixel offset from top-left; draw top-left = world anchor minus anchor_px','duration_ms':sum(DURS[name]),'loop':name=='campfire','playback':'loop' if name=='campfire' else 'one-shot','alpha':'straight RGBA, unpremultiplied; RGB zero where A=0','sampling':'nearest','blend':'source-over','frames':records,'intended_use':NOTES[name],'runtime_integrated':False}
  (OUT/'metadata'/f'{name}.json').write_text(json.dumps(meta,ensure_ascii=False,indent=2)+'\n');effects.append(meta);allframes[name]=frames
  bg=background('forest' if name=='campfire' else 'ruins');previews=[composite(im,bg) for im in frames]
  if name!='campfire':previews += [composite(layer(),bg)];durations=DURS[name]+[750]
  else:durations=DURS[name]
  savegif(previews,OUT/'previews'/f'{name}.gif',durations)
 master=Image.new('RGBA',(S*N,S*len(FX)))
 for row,(name,frames) in enumerate(allframes.items()):
  for i,im in enumerate(frames):master.paste(im,(i*S,row*S))
 master.save(OUT/'atlases'/'all_effects_512x512.png')
 manifest={'title':'RPG POKET — Efeitos 01','version':'1.0.0','date':'2026-10-13','effect_count':8,'total_frame_count':64,'native_canvas':[64,64],'target_screen':[240,320],'source':'Original deterministic procedural pixel geometry. No downloaded artwork. No imagegen output.','integration':'Asset-only pack. No firmware, mechanic, damage, balance or release changes.','master_atlas':{'file':'atlases/all_effects_512x512.png','size':[512,512],'row_order':list(FX),'frame_rect':'x=frame_index*64, y=effect_row*64, width=64, height=64'},'effects':effects}
 (OUT/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
 # Compact overview: eight coherent, labelled cards, native art scaled by integer.
 W,H=1016,634;sheet=Image.new('RGB',(W,H),'#101725');d=ImageDraw.Draw(sheet)
 d.text((28,20),'RPG POKET  /  EFEITOS 01',font=font(24),fill='#ead69c');d.text((28,53),'8 efeitos  ·  64 quadros RGBA  ·  animação em pixels',font=font(14),fill='#9eafbd')
 for n,(name,frames) in enumerate(allframes.items()):
  x=24+(n%4)*248;y=91+(n//4)*250
  d.rectangle((x,y,x+231,y+231),fill='#192333',outline='#425465',width=1)
  bg=background('forest' if name=='campfire' else 'ruins');shot=composite(frames[3],bg,2);sheet.paste(shot,(x+4,y+4))
  d.text((x+11,y+200),TITLES[name],font=font(12),fill='#e7dab2');tag='LOOP  ·  960 ms' if name=='campfire' else f'1 DISPARO  ·  {sum(DURS[name])} ms'
  d.text((x+11,y+216),tag,font=font(10),fill='#92a7b9')
 d.text((28,610),'Escala 2× na prévia. Arquivos nativos: 64×64 px. Gelo reservado para uso futuro.',font=font(12),fill='#859aac')
 sheet.save(OUT/'RPG_POKET_Efeitos01_previa.png')
 # Animated overview: 12.8 s, fully honours each timing and loop/one-shot pause.
 overview=[]
 for tick in range(128):
  ms=tick*100;canvas=sheet.copy()
  for n,(name,frames) in enumerate(allframes.items()):
   x=24+(n%4)*248;y=91+(n//4)*250;total=sum(DURS[name]);cycle=total if name=='campfire' else total+750;pos=ms%cycle;idx=None;s=0
   for j,duration in enumerate(DURS[name]):
    s+=duration
    if pos<s:idx=j;break
   im=frames[idx] if idx is not None else layer();shot=composite(im,background('forest' if name=='campfire' else 'ruins'),2);canvas.paste(shot,(x+4,y+4))
  overview.append(canvas)
 savegif(overview,OUT/'RPG_POKET_Efeitos01_animados.gif',[100]*len(overview))
 # All 64 frames on a checkerboard for alpha/sequence inspection.
 contact=Image.new('RGB',(8*128+130,8*146+42),'#151d2a');dd=ImageDraw.Draw(contact)
 dd.text((12,12),'64 QUADROS / ORDEM DE LEITURA: ESQUERDA → DIREITA',font=font(14),fill='#ddd1b0')
 for row,(name,frames) in enumerate(allframes.items()):
  dd.text((8,48+row*146),TITLES[name],font=font(11),fill='#cbbd9c')
  for i,im in enumerate(frames):
   x=130+i*128;y=42+row*146;b=Image.new('RGBA',(64,64));bd=ImageDraw.Draw(b)
   for yy in range(0,64,8):
    for xx in range(0,64,8):bd.rectangle((xx,yy,xx+7,yy+7),fill='#344252' if (xx//8+yy//8)%2 else '#253342')
   b.alpha_composite(im);contact.paste(b.convert('RGB').resize((128,128),Image.Resampling.NEAREST),(x,y));dd.text((x+5,y+129),f'{i:02d} / {DURS[name][i]} ms',font=font(10),fill='#a3b2c1')
 contact.save(OUT/'previews'/'all_64_frames_checkerboard.png')
 
 if Path(__file__).resolve() != (OUT/'source'/'generate_effects.py').resolve():shutil.copy2(__file__,OUT/'source'/'generate_effects.py')
 print(f'Generated {len(effects)} effects, 64 native RGBA frames: {OUT}')
if __name__=='__main__':main()
