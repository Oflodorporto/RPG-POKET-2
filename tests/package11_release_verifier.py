#!/usr/bin/env python3
"""Negative probes for package11 publication verification; no device/network I/O."""
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
from test_package11_release import synthetic_fixture
from verify_package11_release import VERSION, BUILD, PACKAGE
synthetic = sys.argv[1:] == ['--synthetic']
assert not sys.argv[1:] or synthetic
with tempfile.TemporaryDirectory(prefix='package11-verifier-') as temp:
    temp = Path(temp)
    headers = temp/'firmware/RPG_POKET_2'
    headers.mkdir(parents=True)
    for name in ('ReleaseVersion.h','SaveVersion.h','AssetCatalog.h','EnemyArt.h','Rules.h'):
        shutil.copyfile(ROOT/'firmware/RPG_POKET_2'/name,headers/name)
    folder = temp/'release'
    folder.mkdir()
    if synthetic:
        synthetic_fixture(folder)
        expected_manifest = json.loads((folder/'manifest.json').read_text())
        (temp/'build').mkdir()
        (temp/'cartao/RPGPOKET').mkdir(parents=True)
        shutil.copyfile(folder/'firmware.bin', temp/'build/RPG_POKET_2.ino.bin')
        shutil.copyfile(folder/'artes.pak', temp/'cartao/RPGPOKET/artes.pak')
        subprocess.run([sys.executable, str(ROOT/'tools/release_package11_manifest.py')], cwd=temp, check=True, capture_output=True, text=True)
        assert json.loads((folder/'manifest.json').read_text()) == expected_manifest, 'Generator and verifier fixture contracts differ'
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
        result = subprocess.run([sys.executable,str(ROOT/'tools/verify_package11_release.py')],cwd=temp,capture_output=True,text=True)
        assert (result.returncode == 0) == expected,(label,result.stdout,result.stderr)
    run(True,'approved exact pair')
    probes = 1
    edits = [
        ('recycled scenes identity',lambda m:m.update(version='2026.10.13-scenes77')),
        ('recycled scenes package',lambda m:m.update(package_name='7.7 - Cenarios e viagem diaria')),
        ('dev identity',lambda m:m.update(version=VERSION+'-dev')),
        ('recycled scale identity',lambda m:m.update(version='2026.10.13-scale77')),
        ('recycled scale package',lambda m:m.update(package_name='7.7 - Escala e contraste dos inimigos')),
        ('old scenes build',lambda m:m.update(build=2026101306)),
        ('old scale build',lambda m:m.update(build=2026101307)),
        ('old camp build',lambda m:m.update(build=2026101308)),
        ('recycled camp identity',lambda m:m.update(version='2026.10.13-camp77')),
        ('recycled camp package',lambda m:m.update(package_name='7.7 - D20, acampamento e bolsa')),
        ('recycled explore identity',lambda m:m.update(version='2026.10.13-explore77')),
        ('recycled explore build',lambda m:m.update(build=2026101309)),
        ('recycled explore package',lambda m:m.update(package_name='7.7 - Dados e exploracao das Ruinas')),
        ('recycled immersion identity',lambda m:m.update(version='2026.10.14-immersion77')),
        ('recycled immersion build',lambda m:m.update(build=2026101401)),
        ('recycled immersion package',lambda m:m.update(package_name='7.7 - Imersao nas Ruinas')),
        ('recycled story identity',lambda m:m.update(version='2026.10.14-story77')),
        ('recycled story build',lambda m:m.update(build=2026101402)),
        ('recycled story package',lambda m:m.update(package_name='7.7 - Ecos de Valdaria')),
        ('recycled clock identity',lambda m:m.update(version='2026.10.14-clock77')),
        ('recycled clock build',lambda m:m.update(build=2026101403)),
        ('recycled clock package',lambda m:m.update(package_name='7.7 - Pulso do relogio')),
        ('recycled PvP identity',lambda m:m.update(version='2026.10.14-pvp77')),
        ('recycled PvP build',lambda m:m.update(build=2026101404)),
        ('recycled PvP package',lambda m:m.update(package_name='7.7 - Duelo PvP')),
        ('wrong ration cap',lambda m:m.update(ration_limit=9)),
        ('oversized ration cap',lambda m:m.update(ration_limit=11)),
        ('false historical cap',lambda m:m.update(historical_reader_ration_limit=10)),
        ('false firmware downgrade',lambda m:m.update(firmware_downgrade_supported=True)),
        ('missing ration cap',lambda m:m.pop('ration_limit')),
        ('missing historical cap',lambda m:m.pop('historical_reader_ration_limit')),
        ('missing downgrade caveat',lambda m:m.pop('firmware_downgrade_supported')),
        ('floating point schema',lambda m:m.update(schema=2.0)),
        ('floating point epoch',lambda m:m.update(save_reset_epoch=1.0)),
        ('boolean epoch',lambda m:m.update(save_reset_epoch=True)),
        ('boolean reset slot',lambda m:m.update(reset_game_slots_once=[True,2,3])),
        ('floating point read format',lambda m:m.update(reads_save_formats=[29.0,30])),
        ('floating point ration cap',lambda m:m.update(ration_limit=10.0)),
        ('floating point historical cap',lambda m:m.update(historical_reader_ration_limit=9.0)),
        ('integer downgrade flag',lambda m:m.update(firmware_downgrade_supported=0)),
        ('future build',lambda m:m.update(build=BUILD+1)),
        ('old writes28',lambda m:m.update(writes_save_format=28)),
        ('false compatibility',lambda m:m.update(reads_save_formats=list(range(1,30)))),
        ('no current30',lambda m:m.update(reads_save_formats=[29])),
        ('no historical29',lambda m:m.update(reads_save_formats=[30])),
        ('old writes29',lambda m:m.update(writes_save_format=29)),
        ('future writes31',lambda m:m.update(writes_save_format=31)),
        ('reordered formats',lambda m:m.update(reads_save_formats=[30,29])),
        ('repeated format',lambda m:m.update(reads_save_formats=[29,30,30])),
        ('wrong legacy sentinel',lambda m:m.update(save_format=29)),
        ('false OTA-only upgrade',lambda m:m.update(installation='confirmed-device-ota')),
        ('full-chip erase',lambda m:m.update(usb_erase_flash='all')),
        ('wrong USB generation',lambda m:m.update(usb_required_before_save_format=28)),
        ('new repeated reset epoch',lambda m:m.update(save_reset_epoch=2)),
        ('only one reset slot',lambda m:m.update(reset_game_slots_once=[1])),
        ('omitted preservation',lambda m:m.update(preserves=[])),
        ('wrong count',lambda m:m['art'].update(assets=636)),
        ('wrong CRC',lambda m:m['art'].update(payload_crc32='0b84d93f')),
        ('repeated reset',lambda m:m.update(save_reset_epoch=BUILD)),
        ('false reset removal',lambda m:m.update(reset_game_slots_once=[])),
        ('omitted decode memory',lambda m:m.update(minimum_psram_bytes=7624512)),
        ('wrong package',lambda m:m.update(package_name='7.7 - Bolsa e controles de toque')),
        ('wrong firmware URL',lambda m:m['firmware'].update(url=m['firmware']['url'].replace('shop77','touch77'))),
        ('wrong art URL',lambda m:m['art'].update(url=m['art']['url'].replace('shop77','touch77'))),
        ('wrong board',lambda m:m.update(board='ESP32')),
        ('wrong partition',lambda m:m.update(partition='default')),
    ]
    for label,edit in edits:
        reset();manifest(edit);run(False,label);probes+=1
    # Every numeric contract member must retain its JSON integer type,
    # including nested sizes and array members (bool is an int subclass).
    approved = json.loads(files[folder/'manifest.json'])
    def integer_paths(value, path=()):
        if type(value) is int:
            yield path
        elif type(value) is dict:
            for key, child in value.items(): yield from integer_paths(child, path+(key,))
        elif type(value) is list:
            for key, child in enumerate(value): yield from integer_paths(child, path+(key,))
    for path in integer_paths(approved):
        for replacement in (True, 1.0):
            reset()
            def wrong_type(m):
                target=m
                for key in path[:-1]: target=target[key]
                target[path[-1]]=float(target[path[-1]]) if type(replacement) is float else replacement
            manifest(wrong_type);run(False,'numeric field type '+repr(path));probes+=1
    for name in ('firmware.bin','artes.pak'):
        reset();data=bytearray((folder/name).read_bytes());data[-1]^=1;(folder/name).write_bytes(data);run(False,name+' corrupted');probes+=1
    for label,edit in [
        ('wrong chip',lambda b:struct.pack_into('<H',b,12,0)),
        ('stale scale identity',lambda b:b.__setitem__(slice(None),b.replace(VERSION.encode()+b'\0',b'2026.10.13-scale77\0'))),
        ('development identity',lambda b:b.extend(b'explore77-dev\0')),
        ('stale scenes identity',lambda b:b.__setitem__(slice(None),b.replace(VERSION.encode()+b'\0',b'2026.10.13-scenes77\0'))),
        ('stale camp identity',lambda b:b.__setitem__(slice(None),b.replace(VERSION.encode()+b'\0',b'2026.10.13-camp77\0'))),
        ('immersion development identity',lambda b:b.extend(b'immersion77-dev\0')),
        ('package6 development identity',lambda b:b.extend(b'package6-dev\0')),
        ('story development identity',lambda b:b.extend(b'story77-dev\0')),
        ('package7 development identity',lambda b:b.extend(b'package7-dev\0')),
        ('shop development identity',lambda b:b.extend(b'shop77-dev\0')),
        ('package10 development identity',lambda b:b.extend(b'package10-dev\0')),
        ('stale PvP compiled identity',lambda b:b.__setitem__(slice(None),b.replace(VERSION.encode()+b'\0',b'2026.10.14-pvp77\0'))),
        ('package11 development identity',lambda b:b.extend(b'package11-dev\0')),
        ('package8 development identity',lambda b:b.extend(b'package8-dev\0')),
        ('package9 development identity',lambda b:b.extend(b'package9-dev\0')),
        ('pvp development identity',lambda b:b.extend(b'pvp77-dev\0')),
        ('stale clock compiled identity',lambda b:b.__setitem__(slice(None),b.replace(VERSION.encode()+b'\0',b'2026.10.14-clock77\0'))),
        ('clock development identity',lambda b:b.extend(b'clock77-dev\0')),
        ('wrong compiled identity',lambda b:b.__setitem__(slice(None),b.replace(VERSION.encode()+b'\0',b'2026.10.13-stale777\0'))),
        ('partition overflow',lambda b:b.extend(bytes(0x300001-len(b)))),
    ]:
        reset();data=bytearray((folder/'firmware.bin').read_bytes());edit(data);(folder/'firmware.bin').write_bytes(data)
        manifest(lambda m:m['firmware'].update(bytes=len(data),sha256=hashlib.sha256(data).hexdigest()));run(False,label);probes+=1
    for name,old,new in [('ReleaseVersion.h',VERSION,'2026.10.13-scale77'),('ReleaseVersion.h',str(BUILD)+'u','2026101307u'),('ReleaseVersion.h',PACKAGE,'7.7 - Escala e contraste dos inimigos'),('AssetCatalog.h','ART_COUNT=666','ART_COUNT=636'),('SaveVersion.h','CURRENT_SAVE_FORMAT=30','CURRENT_SAVE_FORMAT=29'),('SaveVersion.h','SAVE_RESET_EPOCH=1','SAVE_RESET_EPOCH=2'),('Rules.h','RATION_LIMIT=10','RATION_LIMIT=9')]:
        reset();p=headers/name;text=p.read_text();assert old in text;p.write_text(text.replace(old,new));run(False,'source '+old);probes+=1
    reset();art=(folder/'artes.pak').read_bytes();old=bytearray(art[:6661456]);struct.pack_into('<III',old,4,6661440,0x0b84d93f,636);(folder/'artes.pak').write_bytes(old)
    manifest(lambda m:m['art'].update(bytes=len(old),sha256=hashlib.sha256(old).hexdigest(),assets=636,payload_crc32='0b84d93f'));run(False,'self-consistent old pack with package11 firmware');probes+=1
    print(f'PASS: {probes} package11 verifier probes; exact pair, identity/hash/pack/Save30/USB/reset/PSRAM/partition/source mismatches. '+('Synthetic firmware fixture only.' if synthetic else 'Prepared firmware/art bytes tested.'))
