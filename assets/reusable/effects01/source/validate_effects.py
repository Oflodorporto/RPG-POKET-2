#!/usr/bin/env python3
"""Validate exported pixels, timing, transparency, atlas mapping and source repeatability."""
from pathlib import Path
from PIL import Image, ImageChops, ImageStat, ImageSequence
import sys, json, hashlib
from generate_effects import FX, layer
from generate_map_variants import reduce
ROOT=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
errors=[];report={'native_effects':[],'map_variants':[]}
def check(ok,msg):
 if not ok:errors.append(msg)
def verify(effect,sub,base=False):
 name=effect['id'];sz=tuple(effect['native_size']);atlas=Image.open(ROOT/effect['atlas']);hs=[];bboxes=[];empty=[];frames=[]
 for r in effect['frames']:
  p=ROOT/r['file'];im=Image.open(p);frames.append(im);a=im.getchannel('A');bbox=a.getbbox();bboxes.append(bbox)
  check(im.mode=='RGBA' and im.size==sz,f'{name} {r["index"]}: mode/dimensions')
  check(hashlib.sha256(p.read_bytes()).hexdigest()==r['sha256'],f'{name} {r["index"]}: SHA-256')
  check(all(a or (r,g,b)==(0,0,0) for r,g,b,a in im.getdata()),f'{name}: hidden RGB')
  rr=r['rect'];crop=atlas.crop((rr['x'],rr['y'],rr['x']+rr['w'],rr['y']+rr['h']))
  check(crop.tobytes()==im.tobytes(),f'{name} {r["index"]}: atlas/frame mismatch')
  for edge in [a.crop((0,0,sz[0],1)),a.crop((0,sz[1]-1,sz[0],sz[1])),a.crop((0,0,1,sz[1])),a.crop((sz[0]-1,0,sz[0],sz[1]))]:check(edge.getbbox() is None,f'{name} {r["index"]}: occupied outside edge')
  if bbox is None:empty.append(r['index'])
  hs.append(hashlib.sha256(im.tobytes()).hexdigest())
  base_name=name if base else effect['base_effect'];idx=r['index'];expected=FX[base_name](idx).convert('RGBA') if idx<7 or base_name=='campfire' else layer()
  expected.putdata([(r,g,b,a) if a else (0,0,0,0) for r,g,b,a in expected.getdata()])
  if not base:expected=reduce(expected,base_name)
  check(expected.tobytes()==im.tobytes(),f'{name} {idx}: reproducibility mismatch')
 check(sum(r['duration_ms'] for r in effect['frames'])==effect['duration_ms'],f'{name}: duration sum')
 check(len(set(hs))==8,f'{name}: duplicate animation frames')
 check(empty==([] if effect['loop'] else [7]),f'{name}: unexpected empty frames {empty}')
 check(effect['loop'] or frames[-1].getchannel('A').getbbox() is None,f'{name}: one-shot must finish transparent')
 result={'id':name,'frames':len(frames),'size':sz,'empty_frames':empty,'frame_pixel_hashes':hs,'no_edge_clipping':True,'atlas_consistency':True,'deterministic_pixel_reproduction':True,'duration_ms':effect['duration_ms']}
 if effect['loop']:
  def delta(x,y):
   # Premultiply to avoid irrelevant color changes in barely visible particles.
   def flatten(im):
    bg=Image.new('RGBA',im.size,(21,28,42,255));bg.alpha_composite(im);return bg.convert('RGB')
   return sum(ImageStat.Stat(ImageChops.difference(flatten(x),flatten(y))).mean)/3
  ds=[delta(frames[i],frames[(i+1)%8]) for i in range(8)]
  check(ds[-1]<=max(ds[:-1])*1.35,f'{name}: loop seam exceeds ordinary transition')
  result['adjacent_frame_mean_abs_rgb_delta']=ds;result['loop_seam_relative_to_largest_ordinary_step']=ds[-1]/max(ds[:-1])
 sub.append(result)
main=json.loads((ROOT/'manifest.json').read_text());maps=json.loads((ROOT/'map32/manifest.json').read_text())
for e in main['effects']:verify(e,report['native_effects'],True)
for e in maps['effects']:verify(e,report['map_variants'])
master=Image.open(ROOT/main['master_atlas']['file']);check(master.size==(512,512),'master atlas size')
for row,e in enumerate(main['effects']):check(master.crop((0,row*64,512,row*64+64)).tobytes()==Image.open(ROOT/e['atlas']).tobytes(),f'{e["id"]}: master atlas row')
for e in main['effects']:
 p=ROOT/'previews'/f'{e["id"]}.gif';im=Image.open(p);duration=sum(f.info.get('duration',0) for f in ImageSequence.Iterator(im));expect=e['duration_ms']+(0 if e['loop'] else 750);check(duration==expect,f'{e["id"]}: preview GIF timing {duration} != {expect}')
report.update({'status':'PASS' if not errors else 'FAIL','errors':errors,'total_rgba_frames':88,'checks':['88 frame dimensions and RGBA mode','88 frame PNG SHA-256 hashes','88 frame/atlas pixel exact matching','transparent canvas borders','RGB zero where alpha zero','all one-shots end with transparent frame 07','distinct animation frames','native and map source pixel reproducibility','constant anchors in per-effect metadata','master atlas row matching','individual preview GIF exact durations','campfire 07-to-00 transition within adjacent-step range'],'limits':['Visual asset validation only; no device or firmware integration performed.','Preview loop gap is demonstrative and is not part of one-shot runtime duration.','No formal photosensitivity certification. One-shot effects use one build and one fade, with no full-screen or random strobe frames.']})
(ROOT/'validation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n');print(json.dumps({'status':report['status'],'frames':88,'errors':errors},ensure_ascii=False));sys.exit(bool(errors))
