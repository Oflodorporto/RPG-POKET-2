#!/usr/bin/env python3
"""Technical export of original generated enemy masters; never edits a repository.
PNG conversion, alpha-component extraction, a shared per-enemy palette and fixed root
are deterministic. New raster character art is supplied by image_gen, not this script.
"""
from pathlib import Path
import argparse, json, hashlib, math, struct, base64, io
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy import ndimage
POSES=['idle','anticipation','attack','hurt']
W,H=80,86
BG=(16,25,47); GOLD=(199,163,91); WHITE=(238,229,212)
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def rgb565(rgb):
 r,g,b=map(int,rgb);return ((r>>3)<<11)|((g>>2)<<5)|(b>>3)
def from565(v):return ((v>>11)*255//31,((v>>5)&63)*255//63,(v&31)*255//31)
def separate(master, boxes=None):
 im=Image.open(master).convert('RGBA'); a=np.asarray(im); mask=a[:,:,3]>=128
 if not mask.any() or mask.mean()>.90:raise ValueError('No usable transparent background: '+str(master))
 lbl,n=ndimage.label(mask,np.ones((3,3),bool)); sizes=np.bincount(lbl.ravel()); sizes[0]=0
 ids=list(np.argsort(sizes)[-4:]); slices=ndimage.find_objects(lbl)
 if len(ids)!=4 or min(sizes[ids])<100:raise ValueError('Fewer than 4 clear poses: '+str(master))
 ids=sorted(ids,key=lambda i:ndimage.center_of_mass(mask,lbl,i)[0]);ids=sorted(ids[:2],key=lambda i:ndimage.center_of_mass(mask,lbl,i)[1])+sorted(ids[2:],key=lambda i:ndimage.center_of_mass(mask,lbl,i)[1])
 centers=[np.array(ndimage.center_of_mass(mask,lbl,i)) for i in ids]; assignments={i:i for i in ids}; dropped=[]
 for i in range(1,n+1):
  if i in ids:continue
  cy,cx=ndimage.center_of_mass(mask,lbl,i); d=[np.linalg.norm((np.array([cy,cx])-v)/np.array(mask.shape)) for v in centers];j=int(np.argmin(d))
  if d[j]<.30:assignments[i]=ids[j]
  else:dropped.append(int(sizes[i]))
 frames=[];meta=[]
 for j,i in enumerate(ids):
  select=np.isin(lbl,[k for k,v in assignments.items() if v==i]);ys,xs=np.where(select)
  box=boxes[j] if boxes else [int(xs.min()),int(ys.min()),int(xs.max()+1),int(ys.max()+1)]
  l,t,r,b=box;arr=a[t:b,l:r].copy();local=select[t:b,l:r];arr[:,:,3]=np.where(local,255,0)
  # Root approximation uses the horizontal midpoint of the lowest 5% of visible pixels.
  by=max(0,arr.shape[0]-max(2,round(arr.shape[0]*.05)));fy,fx=np.where(local[by:]);anchor=[float((fx.min()+fx.max())/2+l),float(b-1)]
  frames.append(Image.fromarray(arr));meta.append(dict(box=box,anchor=anchor,opaque_pixels=int(local.sum()),source_edge_touch=l==0 or t==0 or r==im.width or b==im.height,core_center=[float(centers[j][1]),float(centers[j][0])]))
 return frames,meta,dict(size=list(im.size),alpha_zero=int((a[:,:,3]==0).sum()),alpha_soft=int(((a[:,:,3]>0)&(a[:,:,3]<255)).sum()),major_components=[int(sizes[i]) for i in ids],discarded_components=dropped)
def export_enemy(e,out):
 folder=out/'enemies'/f'{e["id"]:02d}_{e["slug"]}';folder.mkdir(parents=True,exist_ok=True)
 crops,meta,sourceqa=separate(e['master'],e.get('boxes'))
 anchor=e.get('target_anchor',[48,82]);mode=e.get('anchor_mode','ground');anchors=e.get('anchors',[m['core_center'] if mode=='center' else m['anchor'] for m in meta]);limits=[]
 for im,m,p in zip(crops,meta,anchors):
  l,t,r,b=m['box'];ax,ay=p[0]-l,p[1]-t
  for available,span in [(anchor[0]-2,ax),(W-3-anchor[0],im.width-1-ax),(anchor[1]-2,ay),(H-3-anchor[1],im.height-1-ay)]:
   if span>0:limits.append(available/span)
 scale=min(limits);native=[]
 for im,m,p in zip(crops,meta,anchors):
  nw,nh=max(1,round(im.width*scale)),max(1,round(im.height*scale));im2=im.resize((nw,nh),Image.Resampling.NEAREST);l,t,_,_=m['box'];ax=(p[0]-l)*scale;ay=(p[1]-t)*scale;x,y=round(anchor[0]-ax),round(anchor[1]-ay)
  assert x>=0 and y>=0 and x+nw<=W and y+nh<=H,(e['name'],x,y,nw,nh)
  fr=Image.new('RGBA',(W,H));fr.alpha_composite(im2,(x,y));native.append(fr);m['export_placement']=[x,y,nw,nh];m['anchor']=p
 # Palette is learned from opaque pixels across every pose, never from transparent RGB.
 pixels=np.concatenate([np.asarray(f)[:,:,:3][np.asarray(f)[:,:,3]>0] for f in native]);sample=Image.fromarray(pixels.reshape(1,-1,3),'RGB');q=sample.quantize(colors=int(e.get('palette_colors',16))-1,method=Image.Quantize.MEDIANCUT)
 raw=q.getpalette();pal565=[]
 for k in range(len(q.getcolors())):
  v=rgb565(raw[k*3:k*3+3]);v=0xf81e if v==0xf81f else v
  if v not in pal565:pal565.append(v)
 palette=np.array([from565(v) for v in pal565],dtype=np.int32);palall=[0xf81f]+pal565+[0]*(15-len(pal565));frames=[];indexframes=[]
 for j,fr in enumerate(native):
  a=np.asarray(fr);opaque=a[:,:,3]>0;rgb=a[:,:,:3].astype(np.int32);dist=((rgb[:,:,None,:]-palette[None,None,:,:])**2).sum(axis=3);idx=np.where(opaque,dist.argmin(axis=2)+1,0).astype(np.uint8);rgba=np.zeros((H,W,4),np.uint8);rgba[opaque,:3]=palette[idx[opaque]-1];rgba[opaque,3]=255;f=Image.fromarray(rgba);f.save(folder/(POSES[j]+'.png'));frames.append(f);indexframes.append(idx)
  vals=np.array(palall,np.uint16)[idx];(folder/(POSES[j]+'.rgb565le')).write_bytes(vals.astype('<u2').tobytes());flat=idx.ravel();(folder/(POSES[j]+'.idx4')).write_bytes(((flat[::2]<<4)|flat[1::2]).tobytes())
 sheet=Image.new('RGBA',(W*4,H))
 for j,f in enumerate(frames):sheet.alpha_composite(f,(j*W,0))
 sheet.save(folder/'sprites_4_poses.png');(folder/'palette.rgb565le').write_bytes(struct.pack('<16H',*palall))
 # Reference timing; engine must continue to own authoritative action duration/results.
 total={'slash':800,'bite':720,'charge':960,'claw':800,'magic':1000,'pulse':1000,'crush':960}.get(e['attack_kind'],800)
 durations=[round(total*.2),round(total*.3),round(total*.15),total-round(total*.65)]
 clips={'attack':[{'pose':'anticipation','duration_ms':durations[0]},{'pose':'attack','duration_ms':durations[1]},{'pose':'attack','duration_ms':durations[2],'contact':True},{'pose':'idle','duration_ms':durations[3],'recovery':True}], 'hurt':[{'pose':'hurt','duration_ms':180},{'pose':'idle','duration_ms':220}], 'defeat':[{'pose':'idle','duration_ms':450,'alpha':[1,0],'note':'Fade only; no drawn defeat pose.'}]}
 result={**{k:v for k,v in e.items() if k not in ['anchors','boxes']},'schema':'rpg-poket-enemy-assets-v1','frame_size':[W,H],'pose_order':POSES,'root_anchor':anchor,'anchor_mode':mode,'ground_anchor':anchor if mode=='ground' else None,'scale_from_master':scale,'palette_rgb565':[hex(x) for x in palall],'palette_opaque_count':len(pal565),'alpha':'binary, 0 or 255; source threshold >=128','rgb565_key':'0xF81F','facing':'left','source_qa':sourceqa,'poses':meta,'clips':clips,'files':{},'effect_asset':'../../effects/'+e['attack_kind'],'runtime_integrated':False,'hardware_validated':False,'defeat_drawn':False,'provenance':{'tool':'image_gen built-in','date':'2026-10-12','master_sha256':sha(e['master']),'prompt_sha256':sha(e['prompt']),'method':'Original prompted designs; no existing commercial sprites supplied to image generation. Technical conversion only after generation.','legal_note':'No exclusive-IP or legal-clearance guarantee.'}}
 result['qa']={'four_distinct_frames':len({hashlib.sha256(np.asarray(f).tobytes()).hexdigest() for f in frames})==4,'visible_bounds':[list(f.getbbox()) for f in frames],'no_opaque_key_collision':all(x!=0xf81f for x in pal565),'rgba_binary':True,'scale_uniform':True,'frame_palette_common':True}
 make_gif(e,frames,folder,total)
 for p in sorted(folder.iterdir()):
  if p.name!='manifest.json':result['files'][p.name]={'bytes':p.stat().st_size,'sha256':sha(p)}
 (folder/'manifest.json').write_text(json.dumps(result,ensure_ascii=False,indent=2));return result,frames

def draw_fx(d,kind,x,y,t,color=(222,211,174),scale=1):
 if t<0 or t>1:return
 r=3+int(t*10);alpha=1-t
 if kind in ['slash','claw']:
  for k in range(3 if kind=='claw' else 1):d.line([(x+7+k*3,y-11),(x-1+k*3,y),(x+7+k*3,y+9)],fill=color,width=2)
 elif kind=='bite':
  d.line([(x-8,y-7),(x-4,y),(x,y-5),(x+4,y),(x+8,y-7)],fill=color,width=2);d.line([(x-6,y+6),(x-3,y+1),(x+2,y+5),(x+6,y+1)],fill=color,width=2)
 elif kind in ['magic','pulse']:d.ellipse((x-r,y-r,x+r,y+r),outline=(120,196,196),width=2)
 elif kind=='charge':
  for i in range(4):d.rectangle((x+i*4,y+10-i%2*3,x+i*4+2,y+12-i%2*3),fill=(174,145,102))
 else:
  for dx,dy in [(1,0),(-1,0),(0,1),(0,-1),(1,1),(-1,-1)]:d.line((x+dx*4,y+dy*4,x+dx*r,y+dy*r),fill=color,width=2)

def make_gif(e,frames,folder,total):
 seq=[];delays=[];step=80
 for t in range(0,total+1500,step):
  canvas=Image.new('RGB',(240,180),BG);d=ImageDraw.Draw(canvas);d.text((9,8),e['name'],fill=GOLD);d.line((12,143,228,143),fill=(48,62,76));phase='REPOUSO';pose=0;dx=0;opacity=1
  if t<total:
   q=t/total
   if q<.2:pose=1;phase='PREPARACAO';dx=round(q/.2*3)
   elif q<.65:pose=2;phase='ATAQUE';dx=-round(14*min(1,(q-.2)/.3))
   else:pose=0;phase='RECUPERACAO';dx=-round(14*(1-q)/.35)
  elif t<total+300:phase='REPOUSO'
  elif t<total+560:pose=3;phase='RECEBE DANO';dx=round(5*math.sin((t-total-300)/260*math.pi))
  elif t<total+800:phase='REPOUSO'
  elif t<total+1250:phase='DERROTA: FADE';opacity=1-(t-total-800)/450
  else:phase='REINICIO';opacity=0
  sprite=frames[pose].copy()
  if opacity<1:sprite.putalpha(sprite.getchannel('A').point(lambda a:round(a*max(0,opacity))))
  canvas.paste(sprite,(119+dx,61),sprite)
  d=ImageDraw.Draw(canvas)
  if t<total and .35<=t/total<.65:draw_fx(d,e['attack_kind'],112,106,(t/total-.35)/.3)
  if total+300<=t<total+430:draw_fx(d,'crush',164,100,(t-total-300)/130)
  d.text((9,157),phase,fill=WHITE);d.text((140,157),'PREVIA 80x86',fill=(140,154,170));seq.append(canvas);delays.append(step)
 seq[0].save(folder/'animacao_preview.gif',save_all=True,append_images=seq[1:],duration=delays,loop=0,optimize=False,disposal=2)
 # Export each phase as an APNG with clean alpha, no stage or text, and pose-level timing.
 clipframes=[frames[1],frames[2],frames[2],frames[0]];clipframes[0].save(folder/'ataque_preview.png',save_all=True,append_images=clipframes[1:],duration=[round(total*.2),round(total*.3),round(total*.15),total-round(total*.65)],loop=0,disposal=0,blend=0)

def main():
 p=argparse.ArgumentParser();p.add_argument('manifests',nargs='+');p.add_argument('--out',default=str(Path(__file__).parent/'exports'));a=p.parse_args();out=Path(a.out);out.mkdir(parents=True,exist_ok=True);entries=[]
 for m in a.manifests:
  batch=json.loads(Path(m).read_text())['enemies']
  for e in batch:
   for key in ['master','prompt']:
    if not Path(e[key]).is_absolute():e[key]=str((Path(m).parent/e[key]).resolve())
   if e['id'] in [5,19,21,28]:e['anchor_mode']='center';e['target_anchor']=[40 if e['id']==21 else 48,60]
  entries+=batch
 results=[];allframes={};errors=[]
 for e in sorted(entries,key=lambda x:x['id']):
  try:r,f=export_enemy(e,out);results.append(r);allframes[e['id']]=f;print('OK',e['id'],e['name'],r['qa']['visible_bounds'],flush=True)
  except Exception as x:errors.append({'id':e['id'],'name':e['name'],'error':str(x)});print('ERROR',errors[-1],flush=True)
 payload={'schema':'rpg-poket-enemy-assets-v1','count':len(results),'enemies':results,'errors':errors,'runtime_integrated':False};(out/'roster_manifest.json').write_text(json.dumps(payload,ensure_ascii=False,indent=2))
 if results:
  overview(results,allframes,out);html_preview(results,allframes,out)
 if errors:raise SystemExit(1)

def overview(results,allframes,out):
 cols=4;cw,ch=340,235;rows=math.ceil(len(results)/cols);im=Image.new('RGB',(cols*cw,rows*ch+70),BG);d=ImageDraw.Draw(im)
 try:font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',16);title=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',22)
 except:font=title=None
 d.text((16,12),'RPG POKET | INIMIGOS AUTORAIS',font=title,fill=GOLD);d.text((16,43),'Repouso e ataque | Previa exportada; integracao no jogo pendente',font=font,fill=WHITE)
 for j,e in enumerate(results):
  x=(j%cols)*cw;y=70+(j//cols)*ch;d.rectangle((x+5,y+5,x+cw-5,y+ch-5),outline=(54,69,85));d.text((x+12,y+13),f'{e["id"]:02d} {e["name"]}',font=font,fill=WHITE)
  for k,pose in enumerate([0,2]):im.paste(allframes[e['id']][pose].resize((160,172),Image.Resampling.NEAREST),(x+9+k*165,y+43),allframes[e['id']][pose].resize((160,172),Image.Resampling.NEAREST))
 im.save(out/'RPG_POKET_inimigos_autorais.png')

def html_preview(results,allframes,out):
 data=[]
 for e in results:
  arr=[]
  for f in allframes[e['id']]:b=io.BytesIO();f.save(b,format='PNG');arr.append('data:image/png;base64,'+base64.b64encode(b.getvalue()).decode())
  data.append({'id':e['id'],'name':e['name'],'kind':e['attack_kind'],'frames':arr,'duration':sum(x['duration_ms'] for x in e['clips']['attack'])})
 html='''<!doctype html><html lang="pt-BR"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>RPG POKET | Inimigos autorais</title><style>body{background:#10192f;color:#eee5d4;font:16px system-ui;margin:24px;max-width:1100px}h1{color:#c7a35b;font-size:24px}select,button{font:inherit;padding:10px;margin:4px;background:#23314a;color:#eee5d4;border:1px solid #c7a35b}canvas{width:480px;height:auto;aspect-ratio:3/4;max-width:90vw;image-rendering:pixelated;border:1px solid #c7a35b}small{display:block;max-width:620px;line-height:1.5}.layout{display:flex;gap:24px;flex-wrap:wrap}img{image-rendering:pixelated;width:160px;height:172px}.poses{display:grid;grid-template-columns:160px 160px;gap:14px}</style><h1>RPG POKET · Inimigos autorais</h1><p>Quatro poses desenhadas por criatura. Prévia local de animação, sem integração no firmware.</p><select id="enemy"></select><select id="mode"><option value="attack">Ataque</option><option value="hurt">Receber dano</option><option value="defeat">Derrota por fade</option><option value="idle">Repouso</option></select><button id="play">Pausar</button><div class="layout"><canvas id="c" width="240" height="320"></canvas><div><h2>Keyposes</h2><div class="poses" id="poses"></div><small>Repouso, preparação, ataque, dano. Recuperação reutiliza repouso. Derrota é fade técnico: não há desenho adicional de queda. VFX localizados são procedurais e separados. As durações aqui são referências; regras e relógio do jogo continuam autoritativos.</small></div></div><script>const DATA=__DATA__;const sel=document.querySelector('#enemy'),mode=document.querySelector('#mode'),c=document.querySelector('#c'),ctx=c.getContext('2d');let imgs=[],running=true,start=performance.now(),hold=0;for(const e of DATA){let o=document.createElement('option');o.value=e.id;o.textContent=String(e.id).padStart(2,'0')+' '+e.name;sel.append(o)}function change(){const e=DATA.find(e=>e.id==sel.value);imgs=e.frames.map(s=>{let i=new Image;i.src=s;return i});document.querySelector('#poses').innerHTML='';imgs.forEach((i,k)=>{let d=document.createElement('div');d.append(i.cloneNode());d.append(document.createElement('br'));d.append(['Repouso','Preparação','Ataque','Dano'][k]);document.querySelector('#poses').append(d)});start=performance.now()}sel.onchange=change;mode.onchange=()=>start=performance.now();document.querySelector('#play').onclick=()=>{running=!running;document.querySelector('#play').textContent=running?'Pausar':'Reproduzir';if(running)start=performance.now()-hold;else hold=performance.now()-start};change();function fx(kind,x,y,q){ctx.strokeStyle=(kind==='magic'||kind==='pulse')?'#78c4c4':'#ded3ae';ctx.lineWidth=2;ctx.beginPath();if(kind==='slash'||kind==='claw'){for(let k=0;k<(kind==='claw'?3:1);k++){ctx.moveTo(x+7+k*3,y-11);ctx.lineTo(x-1+k*3,y);ctx.lineTo(x+7+k*3,y+9)}}else if(kind==='bite'){ctx.moveTo(x-8,y-7);for(let k=0;k<5;k++)ctx.lineTo(x-8+k*4,y+(k%2?0:-7));ctx.moveTo(x-6,y+6);ctx.lineTo(x-3,y+1);ctx.lineTo(x+2,y+5);ctx.lineTo(x+6,y+1)}else if(kind==='charge'){ctx.fillStyle='#ad9166';for(let k=0;k<4;k++)ctx.fillRect(x+k*4,y+10-k%2*3,2,2)}else if(kind==='magic'||kind==='pulse'){let r=3+Math.floor(q*10),mx=kind==='magic'?x+Math.round((1-q)*32):x;ctx.arc(mx,y,r,0,Math.PI*2)}else{let r=3+Math.floor(q*10);for(const [dx,dy]of [[1,0],[-1,0],[0,1],[0,-1],[1,1],[-1,-1]]){ctx.moveTo(x+dx*4,y+dy*4);ctx.lineTo(x+dx*r,y+dy*r)}}ctx.stroke()}function tick(now){const e=DATA.find(e=>e.id==sel.value),time=running?now-start:hold;let t=time%(e.duration+500),q=t/e.duration,p=0,dx=0,a=1,label='REPOUSO';if(mode.value==='attack'&&q<1){if(q<.2){p=1;label='PREPARAÇÃO';dx=Math.round(q/.2*3)}else if(q<.65){p=2;label='ATAQUE';dx=-Math.round(14*Math.min(1,(q-.2)/.3))}else{label='RECUPERAÇÃO';dx=-Math.round(14*(1-q)/.35)}}if(mode.value==='hurt'){t=time%1000;p=t<260?3:0;label=t<260?'RECEBE DANO':'REPOUSO';dx=t<260?Math.round(5*Math.sin(t/260*Math.PI)):0}if(mode.value==='defeat'){t=time%1400;a=t<450?1-t/450:t<900?0:1;label=t<900?'DERROTA: FADE':'REPOUSO'}ctx.imageSmoothingEnabled=false;ctx.fillStyle='#10192f';ctx.fillRect(0,0,240,320);ctx.fillStyle='#c7a35b';ctx.font='bold 12px monospace';ctx.fillText(e.name,10,25);ctx.fillStyle='#334357';ctx.fillRect(10,178,220,1);ctx.globalAlpha=a;if(imgs[p]?.complete)ctx.drawImage(imgs[p],119+dx,96);ctx.globalAlpha=1;if(mode.value==='attack'&&q>=.35&&q<.65)fx(e.kind,112,141,(q-.35)/.3);if(mode.value==='hurt'&&t<130)fx('crush',164,135,t/130);ctx.fillStyle='#eee5d4';ctx.font='12px monospace';ctx.fillText(label,10,225);ctx.font='10px monospace';ctx.fillText('Canvas 240 × 320 | sprite 80 × 86',10,254);ctx.fillText('Prévia de referência',10,273);ctx.fillText('Sem validação na placa',10,291);requestAnimationFrame(tick)}requestAnimationFrame(tick);</script></html>'''
 (out/'RPG_POKET_previa_inimigos.html').write_text(html.replace('__DATA__',json.dumps(data,ensure_ascii=False)))
if __name__=='__main__':main()
