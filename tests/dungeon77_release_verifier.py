#!/usr/bin/env python3
"""Negative contract probes for the final verifier; no network/publication/device I/O."""
from pathlib import Path
import hashlib
import json
import shutil
import struct
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
synthetic=sys.argv[1:]==['--synthetic']
assert not sys.argv[1:] or synthetic
with tempfile.TemporaryDirectory(prefix='release77-verifier-') as temp:
    temp=Path(temp)
    folder=temp/'firmware/RPG_POKET_2';folder.mkdir(parents=True)
    for name in ('ReleaseVersion.h','AssetCatalog.h','EnemyArt.h'):
        shutil.copyfile(ROOT/'firmware/RPG_POKET_2'/name,folder/name)
    dest=temp/'release';dest.mkdir()
    if synthetic:
        fw=bytearray(128);fw[0]=0xe9;struct.pack_into('<H',fw,12,9)
        version=b'2026.10.13-dungeon77\0';fw[32:32+len(version)]=version
        art=(ROOT/'cartao/RPGPOKET/artes.pak').read_bytes()
        base='https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v2026.10.13-dungeon77/'
        m={'schema':2,'version':'2026.10.13-dungeon77','build':2026101304,'package_name':'7.7 - Dungeon e interface de combate','board':'Waveshare-29667','partition':'app3M_fat9M_16MB','installation':'confirmed-device-ota','save_format':7,'writes_save_format':28,'reads_save_formats':list(range(1,29)),'minimum_psram_bytes':6828800}
        for k,n,b in [('firmware','firmware.bin',fw),('art','artes.pak',art)]:
            (dest/n).write_bytes(b);m[k]={'url':base+n,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
        m['art'].update(assets=636,payload_crc32='0b84d93f')
        (dest/'manifest.json').write_text(json.dumps(m))
    else:
        for n in ('firmware.bin','artes.pak','manifest.json'):shutil.copyfile(ROOT/'release'/n,dest/n)
    original={p.name:p.read_bytes() for p in dest.iterdir()}
    originals={p.name:p.read_bytes() for p in folder.iterdir()}
    def reset():
        for n,b in original.items():(dest/n).write_bytes(b)
        for n,b in originals.items():(folder/n).write_bytes(b)
    def run(ok,label):
        p=subprocess.run([sys.executable,str(ROOT/'tools/verify_dungeon77_release.py')],cwd=temp,capture_output=True,text=True)
        assert (p.returncode==0)==ok,(label,p.stdout,p.stderr)
    def manifest(edit):
        m=json.loads((dest/'manifest.json').read_text());edit(m);(dest/'manifest.json').write_text(json.dumps(m))
    run(True,'paired final baseline');cases=1
    for label,edit in [
      ('dev identity',lambda m:m.update(version='2026.10.13-dungeon77-dev')),
      ('stale build',lambda m:m.update(build=2026101301)),
      ('old count',lambda m:m['art'].update(assets=488)),
      ('old CRC',lambda m:m['art'].update(payload_crc32='53dcc3a5')),
      ('omitted workspace',lambda m:m.update(minimum_psram_bytes=6815040)),
      ('missing save28',lambda m:m.update(reads_save_formats=list(range(1,28)))),
      ('wrong download',lambda m:m['firmware'].update(url=m['firmware']['url'].replace('dungeon77','fatigue76'))),
      ('wrong package',lambda m:m.update(package_name='7.6'))]:
        reset();manifest(edit);run(False,label);cases+=1
    for name in ('firmware.bin','artes.pak'):
        reset();b=bytearray((dest/name).read_bytes());b[-1]^=1;(dest/name).write_bytes(b);run(False,name+' corrupted');cases+=1
    for label,edit in [('wrong chip',lambda b:struct.pack_into('<H',b,12,0)),('stale compiled identity',lambda b:b.__setitem__(slice(None),b.replace(b'2026.10.13-dungeon77\0',b'2026.10.12-stale777\0')))]:
        reset();b=bytearray((dest/'firmware.bin').read_bytes());edit(b);(dest/'firmware.bin').write_bytes(b)
        manifest(lambda m:m['firmware'].update(bytes=len(b),sha256=hashlib.sha256(b).hexdigest()));run(False,label);cases+=1
    reset();p=folder/'AssetCatalog.h';p.write_text(p.read_text().replace('ART_COUNT=636','ART_COUNT=488'));run(False,'source catalog mismatch');cases+=1
    reset();old=(ROOT/'build/enemy_transition/old.pak').read_bytes();(dest/'artes.pak').write_bytes(old)
    manifest(lambda m:m['art'].update(bytes=len(old),sha256=hashlib.sha256(old).hexdigest(),assets=488,payload_crc32='53dcc3a5'));run(False,'self-consistent old pack mixed with new firmware');cases+=1
    print(f'PASS: {cases} final release verifier probes; positive pair and stale identity/catalog/hash/save/PSRAM/mixed-pack rejection. '+('Synthetic ESP32 header fixture only; not a firmware compilation.' if synthetic else 'Prepared final firmware/art pair used.'))
