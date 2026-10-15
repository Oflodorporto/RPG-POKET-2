#!/usr/bin/env python3
"""Actual ON renderer captures for local radio diagnostics/recovery candidate."""
import argparse, hashlib, json, os, subprocess
from pathlib import Path
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
SAN_ENV={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1:strict_string_checks=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1','LSAN_OPTIONS':'exitcode=23'}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--art',required=True,type=Path);p.add_argument('--out',required=True,type=Path);p.add_argument('--mode',choices=('normal','both'),default='both');a=p.parse_args()
 if a.mode=='both' and os.environ.get('RPG_DETECT_LEAKS','1')!='1':raise ValueError('LeakSanitizer is mandatory')
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=True);report={'passed':False,'mode':a.mode,'feature_enabled':1,'captures':[],'sanitized':False,'pixel_parity':False}
 files=[*sorted((ROOT/'firmware/RPG_POKET_2/Coop').glob('*.h')),ROOT/'firmware/RPG_POKET_2/ReleaseVersion.h',ROOT/'tests/coop_radio_diagnostics_render.cpp',ROOT/'tests/coop_training_render.cpp',Path(__file__).resolve()]
 report['source_sha256']={str(f.relative_to(ROOT)):hashlib.sha256(f.read_bytes()).hexdigest() for f in files};manifest=out/'manifest.json'
 manifest.write_text(json.dumps(report,indent=2)+'\n')
 for mode in (('normal','sanitized') if a.mode=='both' else ('normal',)):
  captures=out/mode;captures.mkdir(exist_ok=True);binary=out/('renderer-'+mode)
  flags=['-std=c++17','-UNDEBUG','-DRPG_ENABLE_COOP_TRAINING=1','-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc']
  flags+=['-O2'] if mode=='normal' else ['-O1','-g','-fno-omit-frame-pointer','-no-pie','-fsanitize=address,undefined']
  env=dict(os.environ);env.update(SAN_ENV if mode=='sanitized' else {})
  with (out/(mode+'.log')).open('w') as log:
   for cmd in (['g++',*flags,str(ROOT/'tests/coop_radio_diagnostics_render.cpp'),'-o',str(binary)],[str(binary),str(a.art.resolve()),str(captures)]):
    log.write('+ '+' '.join(cmd)+'\n');log.flush();subprocess.run(cmd,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=240)
  if mode=='sanitized':
   for f in (out/'normal').iterdir():
    if f.read_bytes()!=(captures/f.name).read_bytes():raise ValueError('Renderer parity mismatch: '+f.name)
 for f in sorted((out/'normal').glob('*.ppm')):
  im=Image.open(f);im.load();assert im.size==(320,240);png=out/(f.stem+'.png');im.save(png)
  report['captures'].append({'name':f.stem,'ppm_sha256':hashlib.sha256(f.read_bytes()).hexdigest(),'png_sha256':hashlib.sha256(png.read_bytes()).hexdigest()})
 if len(report['captures'])!=18:raise ValueError('Unexpected capture set')
 selected=['discovery-no-sample','accept-with-signal','recovery-dots-1','recovery-dots-3','diagnostics-live','diagnostics-extremes']
 sheet=Image.new('RGB',(960,760),(10,23,26));draw=ImageDraw.Draw(sheet)
 for i,name in enumerate(selected):
  x=(i%3)*320;y=(i//3)*380;sheet.paste(Image.open(out/(name+'.png')),(x,y+25));draw.text((x+5,y+6),name,fill='white')
 sheet.save(out/'contactsheet.png')
 report.update(passed=True,sanitized=a.mode=='both',pixel_parity=a.mode=='both',sanitizer_environment=SAN_ENV if a.mode=='both' else None)
 manifest.write_text(json.dumps(report,indent=2)+'\n');print((out/'normal.log').read_text());print('PASS actual diagnostic renderer: '+str(out))
if __name__=='__main__':main()
