#!/usr/bin/env python3
"""Bounded native C++ renderer capture. Python only converts PPM pixels to PNG."""
import argparse, hashlib, json, os, pathlib, subprocess, struct, zlib
from PIL import Image, ImageDraw
ROOT = pathlib.Path(__file__).resolve().parents[1]

def run(command, **kwargs):
    return subprocess.run(command, check=True, timeout=120, **kwargs)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--out', type=pathlib.Path, default=ROOT/'output/coop-training-render')
    parser.add_argument('--no-sanitize', action='store_true')
    parser.add_argument('--art',type=pathlib.Path,required=True,help='Validated full production artes.pak (666 assets)')
    args=parser.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    screens=out/'screens';screens.mkdir(exist_ok=True)
    art=args.art.resolve();data=art.read_bytes()
    assert struct.unpack('<4sIII',data[:16])==(b'PKA1',7470912,0x9b4bcfc0,666)
    assert len(data)==7470912+16 and zlib.crc32(data[16:])==0x9b4bcfc0
    flags=['-std=c++17','-UNDEBUG','-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc']
    source=str(ROOT/'tests/coop_training_render.cpp')
    binary=out/'coop-training-render'
    run(['g++',*flags,'-O2',source,'-o',str(binary)])
    with (out/'run.log').open('w') as log:
        run([str(binary),str(art),str(screens)],stdout=log)
    if not args.no_sanitize:
        san=out/'coop-training-render-san';san_screens=out/'sanitized';san_screens.mkdir(exist_ok=True)
        run(['g++',*flags,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',source,'-o',str(san)])
        env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')
        with (out/'sanitized.log').open('w') as log:
            run([str(san),str(art),str(san_screens)],stdout=log,env=env)
        for file in screens.glob('*.ppm'):
            assert file.read_bytes()==(san_screens/file.name).read_bytes(),file.name
    manifest=[]
    for file in sorted(screens.glob('*.ppm')):
        image=Image.open(file);image.load();assert image.size in [(320,240),(240,320)]
        dest=out/(file.stem+'.png');image.save(dest)
        manifest.append({'capture':dest.name,'size':image.size,'sha256':hashlib.sha256(dest.read_bytes()).hexdigest()})
    assert Image.open(out/'attack-targets-native-portrait.png').transpose(Image.Transpose.ROTATE_90).tobytes()==Image.open(out/'attack-targets.png').tobytes()
    selected=['lobby-four-hosts','host-accept','attack-targets','boss-target','ability-focus-test','pending-no-input','reconnect-no-input','ended-1','class-0-race-0','class-1-race-0','class-2-race-0','class-3-race-0']
    sheet=Image.new('RGB',(3*640,4*504),(10,23,26));d=ImageDraw.Draw(sheet)
    for i,name in enumerate(selected):
        x=(i%3)*640;y=(i//3)*504;image=Image.open(out/(name+'.png'));sheet.paste(image.resize((640,480),Image.Resampling.NEAREST),(x,y+24));d.text((x+8,y+6),name,fill='white')
    sheet.save(out/'contactsheet.png')
    (out/'manifest.json').write_text(json.dumps({'source':'actual firmware/RPG_POKET_2/Coop/CoopView.h through LandscapeCanvas into 240x320 portrait storage','art_crc':'9b4bcfc0','source_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [ROOT/'firmware/RPG_POKET_2/Coop/CoopView.h',ROOT/'firmware/RPG_POKET_2/Coop/CoopPortraitBridge.h',ROOT/'tests/coop_training_render.cpp']},'captures':manifest,'sanitized':not args.no_sanitize},indent=2)+'\n')
    print((out/'run.log').read_text().strip());print('Native captures:',out)
if __name__=='__main__':main()
