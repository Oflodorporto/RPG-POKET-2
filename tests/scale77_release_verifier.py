#!/usr/bin/env python3
"""Negative probes for scale77 publication verification; no device/network I/O."""
from pathlib import Path
import hashlib
import json
import shutil
import struct
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from test_scale77_release import synthetic_fixture
from verify_scale77_release import VERSION
synthetic = sys.argv[1:] == ['--synthetic']
assert not sys.argv[1:] or synthetic
with tempfile.TemporaryDirectory(prefix='scale77-verifier-') as temp:
    temp = Path(temp)
    headers = temp/'firmware/RPG_POKET_2'
    headers.mkdir(parents=True)
    for name in ('ReleaseVersion.h','SaveVersion.h','AssetCatalog.h','EnemyArt.h'):
        shutil.copyfile(ROOT/'firmware/RPG_POKET_2'/name,headers/name)
    folder = temp/'release'
    folder.mkdir()
    if synthetic:
        synthetic_fixture(folder)
    else:
        for name in ('firmware.bin','artes.pak','manifest.json'):
            shutil.copyfile(ROOT/'release'/name,folder/name)
    files = {p:p.read_bytes() for p in [*folder.iterdir(),*headers.iterdir()]}
    def reset():
        for p,data in files.items():
            p.write_bytes(data)
    def manifest(edit):
        m = json.loads((folder/'manifest.json').read_text())
        edit(m)
        (folder/'manifest.json').write_text(json.dumps(m))
    def run(expected,label):
        result = subprocess.run([sys.executable,str(ROOT/'tools/verify_scale77_release.py')],cwd=temp,capture_output=True,text=True)
        assert (result.returncode == 0) == expected,(label,result.stdout,result.stderr)
    run(True,'approved exact pair')
    probes = 1
    edits = [
        ('recycled scenes identity',lambda m:m.update(version='2026.10.13-scenes77')),
        ('recycled scenes package',lambda m:m.update(package_name='7.7 - Cenarios e viagem diaria')),
        ('dev identity',lambda m:m.update(version=VERSION+'-dev')),
        ('old build',lambda m:m.update(build=2026101306)),
        ('old writes28',lambda m:m.update(writes_save_format=28)),
        ('false compatibility',lambda m:m.update(reads_save_formats=list(range(1,30)))),
        ('no current29',lambda m:m.update(reads_save_formats=[28])),
        ('false OTA-only upgrade',lambda m:m.update(installation='confirmed-device-ota')),
        ('full-chip erase',lambda m:m.update(usb_erase_flash='all')),
        ('wrong USB generation',lambda m:m.update(usb_required_before_save_format=28)),
        ('new repeated reset epoch',lambda m:m.update(save_reset_epoch=2)),
        ('only one reset slot',lambda m:m.update(reset_game_slots_once=[1])),
        ('omitted preservation',lambda m:m.update(preserves=[])),
        ('wrong count',lambda m:m['art'].update(assets=636)),
        ('wrong CRC',lambda m:m['art'].update(payload_crc32='0b84d93f')),
        ('omitted decode memory',lambda m:m.update(minimum_psram_bytes=7624512)),
        ('wrong package',lambda m:m.update(package_name='7.7 - Bolsa e controles de toque')),
        ('wrong firmware URL',lambda m:m['firmware'].update(url=m['firmware']['url'].replace('scale77','touch77'))),
        ('wrong art URL',lambda m:m['art'].update(url=m['art']['url'].replace('scale77','touch77'))),
        ('wrong board',lambda m:m.update(board='ESP32')),
        ('wrong partition',lambda m:m.update(partition='default')),
    ]
    for label,edit in edits:
        reset();manifest(edit);run(False,label);probes+=1
    for name in ('firmware.bin','artes.pak'):
        reset();data=bytearray((folder/name).read_bytes());data[-1]^=1;(folder/name).write_bytes(data);run(False,name+' corrupted');probes+=1
    for label,edit in [
        ('wrong chip',lambda b:struct.pack_into('<H',b,12,0)),
        ('stale scenes identity',lambda b:b.__setitem__(slice(None),b.replace(VERSION.encode()+b'\0',b'2026.10.13-scenes77\0'))),
        ('wrong compiled identity',lambda b:b.__setitem__(slice(None),b.replace(VERSION.encode()+b'\0',b'2026.10.13-stale777\0'))),
        ('partition overflow',lambda b:b.extend(bytes(0x300001-len(b)))),
    ]:
        reset();data=bytearray((folder/'firmware.bin').read_bytes());edit(data);(folder/'firmware.bin').write_bytes(data)
        manifest(lambda m:m['firmware'].update(bytes=len(data),sha256=hashlib.sha256(data).hexdigest()));run(False,label);probes+=1
    for name,old,new in [('AssetCatalog.h','ART_COUNT=666','ART_COUNT=636'),('SaveVersion.h','CURRENT_SAVE_FORMAT=29','CURRENT_SAVE_FORMAT=28'),('SaveVersion.h','SAVE_RESET_EPOCH=1','SAVE_RESET_EPOCH=2')]:
        reset();p=headers/name;text=p.read_text();assert old in text;p.write_text(text.replace(old,new));run(False,'source '+old);probes+=1
    reset();art=(folder/'artes.pak').read_bytes();old=bytearray(art[:6661456]);struct.pack_into('<III',old,4,6661440,0x0b84d93f,636);(folder/'artes.pak').write_bytes(old)
    manifest(lambda m:m['art'].update(bytes=len(old),sha256=hashlib.sha256(old).hexdigest(),assets=636,payload_crc32='0b84d93f'));run(False,'self-consistent old pack with scale firmware');probes+=1
    print(f'PASS: {probes} scale77 verifier probes; exact pair, identity/hash/pack/Save29/USB/reset/PSRAM/partition/source mismatches. '+('Synthetic firmware fixture only.' if synthetic else 'Prepared firmware/art bytes tested.'))
