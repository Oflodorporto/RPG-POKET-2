from pathlib import Path
import json,hashlib,struct,sys
import numpy as np
from PIL import Image
root=Path(sys.argv[1] if len(sys.argv)>1 else Path(__file__).parent/'exports');manifest=json.loads((root/'roster_manifest.json').read_text());errors=[];checks=[];idlehash={}
for e in manifest['enemies']:
 p=root/'enemies'/f"{e['id']:02d}_{e['slug']}";pal=np.frombuffer((p/'palette.rgb565le').read_bytes(),dtype='<u2');pngs=[];hashes=[]
 try:
  assert len(pal)==16 and pal[0]==0xf81f
  for pose in ['idle','anticipation','attack','hurt']:
   f=Image.open(p/(pose+'.png'));a=np.asarray(f);assert f.size==(80,86) and f.mode=='RGBA';assert set(np.unique(a[:,:,3]))<={0,255};assert f.getbbox();assert not a[0,:,3].any() and not a[-1,:,3].any() and not a[:,0,3].any() and not a[:,-1,3].any()
   vals=np.frombuffer((p/(pose+'.rgb565le')).read_bytes(),dtype='<u2').reshape(86,80);packed=np.frombuffer((p/(pose+'.idx4')).read_bytes(),dtype=np.uint8);idx=np.stack([packed>>4,packed&15],axis=1).ravel().reshape(86,80);assert np.array_equal(pal[idx],vals);assert np.array_equal(vals==0xf81f,a[:,:,3]==0)
   # Check quantization decode preserves exported RGB exactly.
   rgb=np.stack([(vals>>11)*255//31,((vals>>5)&63)*255//63,(vals&31)*255//31],axis=-1).astype(np.uint8);assert np.array_equal(rgb[a[:,:,3]>0],a[:,:,:3][a[:,:,3]>0]);hashes.append(hashlib.sha256(a.tobytes()).hexdigest());pngs.append(a)
  assert len(set(hashes))==4,'Repeated keypose';idlehash[e['id']]=hashes[0];gif=Image.open(p/'animacao_preview.gif');assert gif.n_frames>=6;apng=Image.open(p/'ataque_preview.png');assert getattr(apng,'n_frames',0)>=3;assert 480<=sum(x['duration_ms'] for x in e['clips']['attack'])<=1200;assert not e['runtime_integrated'];assert not e['defeat_drawn']
  checks.append({'id':e['id'],'name':e['name'],'passed':True,'gif_frames':gif.n_frames,'apng_frames':apng.n_frames,'four_unique_poses':True,'index4_roundtrip_exact':True,'rgb565_roundtrip_exact':True,'transparency_binary':True,'no_canvas_edge_touch':True})
 except Exception as x:errors.append({'id':e['id'],'name':e['name'],'error':str(x)})
if len(set(idlehash.values()))!=len(idlehash):errors.append({'error':'At least two enemies share identical idle art.'})
for group in [[10,11,12,13],[14,15,16,17]]:
 vals=[idlehash[i] for i in group if i in idlehash]
 if len(vals)!=len(set(vals)):errors.append({'error':'Identical variant idle', 'group':group})
r={'count':len(checks),'expected_full_roster':29,'full_roster_complete':len(checks)==29,'checks':checks,'errors':errors,'browser_visual_test':'Not run: local Chromium process rejected socket creation. JavaScript logic tested separately.','hardware_test':'Not run. No firmware integration.'};(root/'validation.json').write_text(json.dumps(r,ensure_ascii=False,indent=2));print(json.dumps({'passed':len(checks),'errors':errors},ensure_ascii=False));sys.exit(bool(errors))
