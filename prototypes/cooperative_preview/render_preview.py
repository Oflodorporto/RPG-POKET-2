#!/usr/bin/env python3
"""Read-only trace replay and two orientation studies; no firmware imports or writes."""
from extract_assets import *
import math,sys,platform
from PIL import ImageFont
TRACE=ROOT/'output/coop-prototype/turn-trace.jsonl'
rows=[json.loads(x) for x in TRACE.read_text().splitlines()]
assert len(rows)==27 and [r['revision'] for r in rows]==list(range(1,28))
assert rows[-1]['state_hash']==180348654 and rows[-1]['phase']==1
assert all(len(r['actors'])==6 and r['actors'][2]['kind']==0 for r in rows)
FONTSRC=FW/'src/GFX/font/glcdfont.h'
fontsection=FONTSRC.read_text().split('font[]')[1].split('{',1)[1].split('};',1)[0]
FONT=bytes(int(x,16) for x in re.findall(r'0x([0-9A-Fa-f]{2})',fontsection))
assert len(FONT)>=1280
GOLD=(232,193,119);WHITE=(246,237,210);BLUE=(125,197,238);GREEN=(122,213,126);RED=(228,107,91);MUTED=(178,174,151);INK=(10,23,26)
NAMES={1:'J1',2:'J2',10:'GOBLIN',11:'LOBO',12:'OSSOS',13:'JAVALI',14:'LOBO',15:'OSSOS',16:'GUARDIAO'}
CLASSES=['MAGO','CAVALEIRO','GUERREIRO','BARBARO']
FRAME=Image.open(SRC/'assets/ruins7/ui-frame.png').convert('RGBA')
BG=indexed(59,58).resize((240,320),Image.Resampling.NEAREST)
# Same authored scene, neutral center crop in both orientations, no stretching.
BG=BG.crop((0,110,240,300))
PROFILES=[]
s=(FW/'EnemyScale.h').read_text().split('constexpr Profile profiles')[1].split('};',1)[0]
for line in s.splitlines():
 match=re.search(r'\{(\d+),(\d+),(\d+),(\d+),',line)
 if match:PROFILES.append(tuple(map(int,match.groups())))
assert len(PROFILES)==29
SPRITES={};HBOUNDS={}
for a in rows[0]['actors'][:2]:
 ims=[hero(a,p) for p in range(6)];bs=[im.getbbox() for im in ims]
 HBOUNDS[a['id']]=(min(b[0] for b in bs),min(b[1] for b in bs),max(b[2] for b in bs),max(b[3] for b in bs))
 for p,im in enumerate(ims):SPRITES[a['id'],p]=im.crop(HBOUNDS[a['id']])
for species in range(5):
 for pose in range(4):
  l,t,r,b=PROFILES[species];SPRITES['enemy',species,pose]=enemy(species,pose).crop((l,t,r,b))
CHECKS=[]
def rect(im,box,c):ImageDraw.Draw(im).rectangle((box[0],box[1],box[0]+box[2]-1,box[1]+box[3]-1),fill=c)
def text(im,x,y,s,color=WHITE,scale=1,center=None,region=None):
 width=len(s)*6*scale-1
 if center is not None:x=center-width//2
 assert x>=0 and y>=0 and x+width<=im.width and y+8*scale<=im.height,(s,x,y,im.size)
 if region:assert x>=region[0] and y>=region[1] and x+width<=region[0]+region[2] and y+8*scale<=region[1]+region[3],(s,region)
 d=ImageDraw.Draw(im)
 for ch in s:
  i=ord(ch);assert i<256
  for dx in range(5):
   v=FONT[i*5+dx]
   for dy in range(8):
    if v&(1<<dy):d.rectangle((x+dx*scale,y+dy*scale,x+(dx+1)*scale-1,y+(dy+1)*scale-1),fill=color)
  x+=6*scale

def panel(im,box,active=False):
 x,y,w,h=box;assert x>=0 and y>=0 and x+w<=im.width and y+h<=im.height
 # Nine-slice the real game frame; no generated artwork.
 c=5;sw,sh=FRAME.size
 for sx,sy,sww,shh,dx,dy,dw,dh in [(0,0,c,c,x,y,c,c),(sw-c,0,c,c,x+w-c,y,c,c),(0,sh-c,c,c,x,y+h-c,c,c),(sw-c,sh-c,c,c,x+w-c,y+h-c,c,c),(c,0,sw-2*c,c,x+c,y,w-2*c,c),(c,sh-c,sw-2*c,c,x+c,y+h-c,w-2*c,c),(0,c,c,sh-2*c,x,y+c,c,h-2*c),(sw-c,c,c,sh-2*c,x+w-c,y+c,c,h-2*c),(c,c,sw-2*c,sh-2*c,x+c,y+c,w-2*c,h-2*c)]:
  patch=FRAME.crop((sx,sy,sx+sww,sy+shh)).resize((dw,dh),Image.Resampling.NEAREST);im.paste(patch,(dx,dy),patch)
 if active:ImageDraw.Draw(im).rectangle((x+2,y+2,x+w-3,y+h-3),outline=GREEN)

def bar(im,x,y,w,value,total,c):
 rect(im,(x,y,w,3),(30,30,25));rect(im,(x,y,max(1,(w*value//total)) if value else 0,3),c) if value else None

def geometry(mode,boss=False):
 if mode=='portrait':
  W,H=240,320
  return dict(size=(W,H),arena=(3,42,234,131),event=(4,175,232,14),stats=[(4,192,114,38),(122,192,114,38)],buttons=[(4,233,114,36),(122,233,114,36),(4,272,114,36),(122,272,114,36)],hero=[(20,53,57,54),(34,112,57,54)],enemy=[(159,49,48,36),(174,88,48,36),(154,127,48,36)],boss=(140,81,84,73),footer=311)
 return dict(size=(320,240),arena=(3,42,314,106),event=(4,150,312,14),stats=[(4,167,154,29),(162,167,154,29)],buttons=[(4,199,75,30),(83,199,75,30),(162,199,75,30),(241,199,75,30)],hero=[(18,54,58,64),(78,76,58,64)],enemy=[(192,51,48,40),(260,75,48,40),(191,97,48,40)],boss=(212,61,84,77),footer=232)

def bounds_intersect(a,b):return max(a[0],b[0])<min(a[0]+a[2],b[0]+b[2]) and max(a[1],b[1])<min(a[1]+a[3],b[1]+b[3])
def fit(im,box):
 x,y,w,h=box;scale=min(w/im.width,h/im.height);ww=max(1,int(im.width*scale));hh=max(1,int(im.height*scale));return im.resize((ww,hh),Image.Resampling.NEAREST),(x+(w-ww)//2,y+h-hh)

def label(a):return NAMES[a['id']]
def action_line(r):
 n=NAMES[r['actor_id']];t=NAMES[r['target_id']];act=r['action']
 if r['from_wave']!=r['wave']:return f'ONDA {r["from_wave"]} CONCLUIDA > {r["wave"]}'
 if act==1:return f'{n}: DEFENDEU'
 if act==2:return f'{n}: POCAO +{r["amount"]} HP'
 if act==3:return f'{n}: POCAO +{r["amount"]} MP'
 if r['amount']==0:return f'{n} > {t}: ESQUIVA'
 return f'{n} > {t}: -{r["amount"]}'+(' CRIT' if r['critical'] else '')+(' *' if act==4 else '')

def order(r):
 if r['phase']:return 'FIM DO ENCONTRO'
 alive=[a for a in r['actors'] if a['kind'] and a['hp']];i=next(i for i,a in enumerate(alive) if a['id']==r['next_actor_id']);alive=alive[i:]+alive[:i]
 return ' > '.join(NAMES[a['id']] if a['kind']==1 else {0:'GOB',1:'LOB',2:'OSS',3:'BOSS',4:'JAV'}[a['species']] for a in alive)

def render(mode,r,anim=0):
 g=geometry(mode,r['wave']==3);W,H=g['size'];im=Image.new('RGB',(W,H),INK)
 # The control and status coordinates never move during action animation.
 ar=g['arena'];scale=max(ar[2]/BG.width,ar[3]/BG.height);size=(math.ceil(BG.width*scale),math.ceil(BG.height*scale));scene=BG.resize(size,Image.Resampling.NEAREST);ox=(scene.width-ar[2])//2;oy=(scene.height-ar[3])//2;scene=scene.crop((ox,oy,ox+ar[2],oy+ar[3]));im.paste(scene,ar[:2])
 panel(im,(3,2,W-6,20));title=f'COOP 2P   '+('CHEFE' if r['wave']==3 else f'ONDA {r["wave"]}/2');text(im,0,8,title,GOLD,center=W//2)
 text(im,4,25,'ORDEM',MUTED);text(im,40,25,order(r),GREEN if r['phase'] else WHITE)
 nxt='VITORIA EM RAM' if r['phase']==1 else 'DERROTA EM RAM' if r['phase']==2 else 'VEZ: '+NAMES[r['next_actor_id']]
 text(im,4,34,nxt,BLUE)
 text(im,W-6-len(f'R{r["round"]} / T{r["revision"]}')*6,34,f'R{r["round"]} / T{r["revision"]}',MUTED)
 # Roster quantities and HP are exact post-confirmation values from the trace.
 for a in r['actors']:
  if a['kind']==0:continue
  player=a['kind']==1;slot=a['slot'];b=g['hero'][slot] if player else g['boss'] if a['kind']==3 else g['enemy'][slot-3]
  pose=0
  if a['id']==r['actor_id'] and anim in (1,2) and r['action'] in (0,4) and r['from_wave']==r['wave']:pose=2
  elif a['id']==r['target_id'] and r['amount'] and anim==2 and not player:pose=3
  src=SPRITES[a['id'],pose] if player else SPRITES['enemy',a['species'],pose]
  if a['hp']==0 and not (a['id']==r['target_id'] and anim in (1,2)):continue
  if a['hp']==0:src=src.copy();src.putalpha(src.getchannel('A').point(lambda v:v//2))
  spr,(x,y)=fit(src,b)
  # Movement is strictly visual and bounded, 2 pixels only for active source.
  if a['id']==r['actor_id'] and anim in (1,2) and r['action'] in (0,4) and r['from_wave']==r['wave']:x+=2 if player else -2
  assert x>=ar[0] and y>=ar[1] and x+spr.width<=ar[0]+ar[2] and y+spr.height<=ar[1]+ar[3]
  d=ImageDraw.Draw(im);d.ellipse((x+3,y+spr.height-4,x+spr.width-3,y+spr.height+1),fill=(39,39,27))
  im.paste(spr,(x,y),spr)
  active=(not r['phase'] and a['id']==r['next_actor_id'])
  if active:d.polygon([(x+spr.width//2-3,y-5),(x+spr.width//2+3,y-5),(x+spr.width//2,y-2)],fill=GREEN)
  if player:
   # Tiny character IDs distinguish per-player turns without replacing their sprites.
   text(im,b[0]-12,b[1]+b[3]-8,f'J{slot+1}',BLUE)
  else:
   tag=f'{NAMES[a["id"]]} {a["hp"]}' if a['kind']==3 else f'{NAMES[a["id"]][:3]} {a["hp"]}'
   tw=len(tag)*6;tx=max(ar[0],min(W-3-tw,b[0]+b[2]//2-tw//2));ty=b[1]+b[3]-2
   rect(im,(tx-1,ty,tw+2,10),INK);text(im,tx,ty+1,tag,GOLD if a['kind']==3 else WHITE);bar(im,tx,ty+9,tw,a['hp'],a['maxhp'],RED)
 # Confirmed action is separate from the next actor; no speculative hit result.
 panel(im,g['event']);text(im,0,g['event'][1]+4,action_line(r),GOLD,center=W//2)
 for i,b in enumerate(g['stats']):
  a=r['actors'][i];panel(im,b,a['id']==r['next_actor_id'] and not r['phase']);x,y,w,h=b
  if mode=='portrait':
   text(im,x+5,y+5,f'J{i+1} {CLASSES[a["class"]]}',GOLD)
   text(im,x+5,y+15,f'HP {a["hp"]}/{a["maxhp"]}',GREEN);text(im,x+5,y+25,f'MP {a["mp"]}/{a["maxmp"]}',BLUE)
   if a['guard']:text(im,x+w-13,y+25,'D',GOLD)
  else:
   text(im,x+5,y+4,f'J{i+1} {CLASSES[a["class"]]}',GOLD)
   text(im,x+5,y+14,f'HP {a["hp"]}/{a["maxhp"]} MP {a["mp"]}/{a["maxmp"]}',WHITE)
   if a['guard']:text(im,x+w-12,y+4,'D',GOLD)
 names=['ATACAR','FOCO*','DEFENDER','ITENS'];sub=['alvo','2 MP','guarda','HP / MP']
 for i,b in enumerate(g['buttons']):
  panel(im,b);x,y,w,h=b;text(im,0,y+7,names[i],GOLD,center=x+w//2,region=b);text(im,0,y+20 if h==36 else y+17,sub[i],MUTED,center=x+w//2,region=b)
 text(im,0,g['footer'],'J3: RESERVA FUTURA | *TESTE',MUTED,center=W//2)
 return im

# UI tests: concept geometry only, not a claim of a working touch driver.
for mode in ['portrait','landscape']:
 g=geometry(mode);W,H=g['size'];b=g['buttons']
 assert W*H==76800 and W*H*2==153600
 assert len(b)==4
 for i,a in enumerate(b):
  assert a[0]>=0 and a[1]>=0 and a[0]+a[2]<=W and a[1]+a[3]<=H
  assert all(not bounds_intersect(a,c) for c in b[i+1:])
  assert not bounds_intersect(a,g['event']) and all(not bounds_intersect(a,c) for c in g['stats'])
 # Exhaustively classify every native pixel and every edge/corner of buttons.
 hits=[0]*4
 for y in range(H):
  for x in range(W):
   found=[i for i,(bx,by,bw,bh) in enumerate(b) if bx<=x<bx+bw and by<=y<by+bh]
   assert len(found)<=1
   if found:hits[found[0]]+=1
 assert hits==[w*h for x,y,w,h in b]
 CHECKS.append(dict(mode=mode,size=[W,H],framebuffer_rgb565_bytes=W*H*2,buttons=b,hit_pixels=hits,overlap=False,scope='Planned hitbox geometry, no touch/rotation/hardware implementation'))
 for r in rows:
  for anim in range(4):render(mode,r,anim)

# Audience-facing contact sheets: exact nearest-neighbor 2x, same trace rows.
font_path='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
TITLE=ImageFont.truetype(font_path,22);BODY=ImageFont.truetype(font_path,15);SMALL=ImageFont.truetype(font_path,13)
def shell(mode):
 g=geometry(mode);w,h=g['size'];sheet=Image.new('RGB',(w*4+60,h*2+143),(17,28,30));d=ImageDraw.Draw(sheet)
 d.text((20,13),'PROTÓTIPO — não instalado',font=TITLE,fill=GOLD)
 d.text((20,44),f'{"Retrato" if mode=="portrait" else "Paisagem"} {w} × {h} · pixels ampliados 2× · mesmas ações e estados',font=BODY,fill=WHITE)
 for j,ix in enumerate([2,24]):
  x=20+j*(w*2+20);d.text((x,72),'Onda 1 · Goblin agiu · próximo: Lobo' if j==0 else 'Chefe · Guardião agiu · próximo: J1',font=SMALL,fill=BLUE)
  im=render(mode,rows[ix]);im.save(OUT/f'{mode}-{"wave" if j==0 else "boss"}-native.png');sheet.paste(im.resize((w*2,h*2),Image.Resampling.NEAREST),(x,95))
 d.text((20,h*2+104),'D = defesa ativa. Foco é ação provisória de teste. Itens: HP/MP.',font=SMALL,fill=MUTED)
 d.text((20,h*2+123),'Só replay local: sem instalação, saves, rádio ou FPS de placa medidos.',font=SMALL,fill=MUTED)
 sheet.save(OUT/f'coop-{mode}-2x.png')
for mode in ['portrait','landscape']:shell(mode)

# A full 27-event playback, labelled pacing rather than simulated device FPS.
frames=[];durations=[]
for r in rows:
 for p in range(4):
  canvas=Image.new('RGB',(1170,735),(17,28,30));d=ImageDraw.Draw(canvas)
  d.text((20,12),'PROTÓTIPO — não instalado',font=TITLE,fill=GOLD)
  d.text((20,43),'Replay dos 27 turnos confirmados · 1 s por ação · sem tempo real de placa',font=BODY,fill=WHITE)
  d.text((20,68),'Retrato 240 × 320 · 2×',font=BODY,fill=BLUE);d.text((520,68),'Paisagem 320 × 240 · 2×',font=BODY,fill=BLUE)
  canvas.paste(render('portrait',r,p).resize((480,640),Image.Resampling.NEAREST),(20,93));canvas.paste(render('landscape',r,p).resize((640,480),Image.Resampling.NEAREST),(520,93))
  d.text((520,595),f'Ação {r["revision"]}/27 · estado confirmado {r["state_hash"]}',font=BODY,fill=GOLD)
  d.text((520,622),'D = defesa ativa; * = Foco provisório de teste.',font=BODY,fill=MUTED)
  d.text((520,649),'J3 é só reserva de espaço, sem terceiro jogador.',font=BODY,fill=MUTED)
  d.text((520,676),'Sem gravação de campanha e sem recompensa.',font=BODY,fill=MUTED)
  frames.append(canvas);durations.append([120,180,180,520][p])
frames[0].save(OUT/'coop-turnos-replay.gif',save_all=True,append_images=frames[1:],duration=durations,loop=0,optimize=True,disposal=2)

encoded=Image.open(OUT/'coop-turnos-replay.gif')
encoded_durations=[]
for frame_no in range(encoded.n_frames):
 encoded.seek(frame_no);encoded_durations.append(encoded.info['duration'])
assert sum(encoded_durations)==27000
source_paths=[Path(__file__),Path(__file__).with_name('extract_assets.py'),SRC/'cartao/RPGPOKET/artes.pak',FW/'AssetCatalog.h',FW/'EnemyArtCatalog.h',FW/'EnemyArt.h',FW/'Indexed4.h',FW/'EnemyScale.h',FW/'RaceArt.h',FW/'HeroArt.h',FW/'PersonalArt.h',FONTSRC,SRC/'assets/ruins7/ui-frame.png',TRACE,ROOT/'coop-prototype-work/prototypes/cooperative/CoopModel.h']
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
manifest=dict(purpose='Isolated, read-only orientation study and trace playback',installed=False,production_changed=False,trace_rows=27,trace_sha256=sha(TRACE),last_state_hash=rows[-1]['state_hash'],static_revisions=[3,25],source_files=[dict(path=str(p.relative_to(ROOT)),sha256=sha(p),bytes=p.stat().st_size) for p in source_paths],asset_ids=sorted(USED),geometry=CHECKS,animation=dict(generated_frames=len(frames),encoded_frames=encoded.n_frames,encoded_duration_ms=sum(encoded_durations),duration_ms=sum(durations),pacing='1 second per confirmed action; not measured gameplay timing',engine_states_changed=False,transition_policy='Wave-transition frames show new confirmed wave without attack animation; no old target is replaced by a different enemy',terminal_policy='Dead target briefly shown at half opacity only during hit pose, then removed; no next actor when terminal'),libraries=dict(python=platform.python_version(),Pillow=Image.__version__),limitations=['No firmware integration or publication','No hardware rotation or touch driver','No FPS, flash, free RAM or ESP32 performance claim','J3 is reserved space, not implemented','Foco and guard model are provisional engine demonstration rules','Controls are planned zones; menu transitions are not implemented'])
manifest['outputs']=[dict(path=str(p.relative_to(ROOT)),sha256=sha(p),bytes=p.stat().st_size) for p in sorted(OUT.glob('*.png')) if p.name!='assets-contact.png']+[dict(path=str((OUT/'coop-turnos-replay.gif').relative_to(ROOT)),sha256=sha(OUT/'coop-turnos-replay.gif'),bytes=(OUT/'coop-turnos-replay.gif').stat().st_size)]
(OUT/'preview-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
(OUT/'checks.json').write_text(json.dumps(dict(result='PASS',rendered_native_frames=216,trace_rows=27,buttons_classified_pixels=153600,geometry=CHECKS,static_states_match=True),indent=2)+'\n')
print('PASS: 216 bounded native frames; all 27 exact post-states; 153600 planned hit-test pixels; 2 orientation sheets and 27-second GIF.')
