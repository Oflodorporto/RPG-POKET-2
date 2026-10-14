#!/usr/bin/env python3
"""Offline positive/negative fixtures for evidence validation; not target proof."""
from pathlib import Path
import copy
import sys
import tempfile
import hashlib
import struct
import functools
import operator
sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from record_coop_training_build import parse_memory,check_options,check_image,parse_symbols,inspect_shared_linkage,inspect_updater_linkage,UPDATER_STATE,compare_budgets,validate_evidence_directory,FQBN
checks=0
def reject(fn):
    global checks
    try:fn()
    except (AssertionError,KeyError,ValueError):checks+=1
    else:raise AssertionError('Accepted invalid evidence fixture')
log='Sketch uses 1,900,000 bytes (60%) of program storage space. Maximum is 3145728 bytes.\nGlobal variables use 85,000 bytes (25%) of dynamic memory, leaving 242680 bytes for local variables. Maximum is 327680 bytes.\n'
m=parse_memory(log);assert m['linker_static_ram_bytes']==85000 and m['arduino_program_bytes']==1900000;checks+=1
for bad in ('',log+log,log.replace('3145728','1000000'),log.replace('85,000','500000')):reject(lambda bad=bad:parse_memory(bad))
for flag in (0,1):
    options={'fqbn':FQBN,'customBuildProperties':'compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING='+str(flag)}
    assert check_options(options,flag)['training_define']==flag;checks+=1
    bad=copy.deepcopy(options);bad['fqbn']='esp32:esp32:esp32';reject(lambda:check_options(bad,flag))
    reject(lambda:check_options({'fqbn':FQBN,'customBuildProperties':''},flag))
    bad=copy.deepcopy(options);bad['customBuildProperties']+=' -DRPG_ENABLE_COOP_TRAINING='+str(1-flag);reject(lambda:check_options(bad,flag))
    bad=copy.deepcopy(options);bad['customBuildProperties']=bad['customBuildProperties'].replace('compiler.cpp','build');reject(lambda:check_options(bad,flag))
    bad=copy.deepcopy(options);bad['customBuildProperties']+=' -DRPG_ENABLE_COOP_TRAINING=2';reject(lambda:check_options(bad,flag))
    reject(lambda:check_options({'fqbn':FQBN,'compiler.cpp.extra_flags':'','unrelated':'-DRPG_ENABLE_COOP_TRAINING='+str(flag)},flag))
    bad=copy.deepcopy(options);bad['customBuildProperties']+=',compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING='+str(flag);reject(lambda:check_options(bad,flag))
    bad=copy.deepcopy(options);bad['unrelated']='-DRPG_ENABLE_COOP_TRAINING='+str(flag);reject(lambda:check_options(bad,flag))
    reordered=copy.deepcopy(options);parts=FQBN.split(':',3);reordered['fqbn']=':'.join(parts[:3])+':'+','.join(reversed(parts[3].split(',')))
    assert check_options(reordered,flag)['training_define']==flag;checks+=1
header=bytearray(24);header[0]=0xe9;header[1]=1;header[12]=9;header[23]=1
payload=b'2026.10.14-pvp77\0fixture'
image=header+struct.pack('<II',0x3c000000,len(payload))+payload
image+=bytes((15-len(image)%16))+bytes([functools.reduce(operator.xor,payload,0xef)])
image+=hashlib.sha256(image).digest();assert check_image(bytes(image))['chip_id']==9;checks+=1
for offset,value in ((0,0),(1,0),(12,0),(23,0),(28,255),(33,255),(-33,0),(-1,image[-1]^1)):
    bad=bytearray(image);bad[offset]=value;reject(lambda bad=bad:check_image(bytes(bad)))
reject(lambda:check_image(bytes(image)+b'junk'))
reject(lambda:check_image(bytes(image[:-1])))
symbols=''.join('3fc00000 %08x B %s\n'%(size,name) for name,size in {'coopSession':3900,'coopInput':440,'coopDevice':88,'coopradio::queueBytes':2016,'coopradio::queueState':80}.items())
assert parse_symbols(symbols,1)['coopSession']==3900;checks+=1
assert parse_symbols(symbols+'3fc00004 00000004 r TAG\n3fc00008 00000004 r TAG\n',1)['coopSession']==3900;checks+=1
assert parse_symbols('3fc00000 00000100 B game\n',0)=={};checks+=1
reject(lambda:parse_symbols(symbols,0))
reject(lambda:parse_symbols('',1))
reject(lambda:parse_symbols(symbols+symbols,1)) # ambiguous duplicate names must not collapse
reject(lambda:parse_symbols(symbols.replace('00000f3c','00010000'),1))
reject(lambda:parse_symbols(symbols.replace('000007e0','00000c00'),1))
shared=''.join('3fc00000 00000004 B %s\n'%name for name in ('artMemory','enemyArt::workspace','coopradio::owner','coopradio::callbackCounters'))
assert len(inspect_shared_linkage(shared,1)['shared_mutable']['artMemory'])==1;checks+=1
assert inspect_shared_linkage('',0)['shared_mutable']['artMemory']==[];checks+=1
reject(lambda:inspect_shared_linkage('',1))
for name in ('artMemory','enemyArt::workspace','coopradio::owner','coopradio::callbackCounters'):
    reject(lambda name=name:inspect_shared_linkage(shared+'3fc00000 00000004 B '+name+'\n',1))
tables='3c000000 000029a0 r assetMeta\n3c0029a0 000029a0 r assetMeta\n'
reject(lambda:inspect_shared_linkage(shared+tables,1))
fonts='3c000000 00000500 r font\n3c000500 00000500 r font\n'
assert len(inspect_shared_linkage(shared+fonts,1)['readonly_tables']['font'])==2;checks+=1
updater=''.join('3fc00000 00000004 B %s\n'%name for name in UPDATER_STATE)+'42010000 00000e8d T installUpdate(bool)\n'
assert set(inspect_updater_linkage(updater))==set(UPDATER_STATE)|{'installUpdate(bool)'};checks+=1
for name in (*UPDATER_STATE,'installUpdate(bool)'):
    lines=updater.splitlines(keepends=True)
    selected=next(line for line in lines if line.endswith(' '+name+'\n'))
    reject(lambda selected=selected:inspect_updater_linkage(updater.replace(selected,'')))
    reject(lambda selected=selected:inspect_updater_linkage(updater+selected))
    weak=selected.replace(' B ',' V ').replace(' T ',' W ')
    reject(lambda selected=selected,weak=weak:inspect_updater_linkage(updater.replace(selected,weak)))
    zero=selected.split(' ',1)[0]+' 00000000 '+selected.split(' ',2)[2]
    reject(lambda selected=selected,zero=zero:inspect_updater_linkage(updater.replace(selected,zero)))
off={'image':{'bytes':1900000},'memory':{'linker_static_ram_bytes':80000}}
on={'image':{'bytes':2000000},'memory':{'linker_static_ram_bytes':86500}}
assert compare_budgets(off,on)['added_static_ram_bytes']==6500;checks+=1
for image,ram in ((1900000,86500),(2300000,86500),(2000000,80000),(2000000,100000)):
    reject(lambda image=image,ram=ram:compare_budgets(off,{'image':{'bytes':image},'memory':{'linker_static_ram_bytes':ram}}))
with tempfile.TemporaryDirectory() as folder:
    root=Path(folder);(root/'experimental-build-evidence.json').write_text('{}');validate_evidence_directory(root);checks+=1
    reject(lambda:validate_evidence_directory(root,complete=True))
    for name in ('firmware.bin','firmware.elf','manifest.json','production-manifest.json','unreviewed.txt','SHA256SUMS'):
        p=root/name;p.write_text('fixture');reject(lambda:validate_evidence_directory(root));p.unlink()
    p=root/'toolchain.txt';p.symlink_to(root/'experimental-build-evidence.json');reject(lambda:validate_evidence_directory(root));p.unlink()
    (root/'nested').mkdir();reject(lambda:validate_evidence_directory(root))
print(f'PASS: {checks} offline build-evidence fixture checks. Synthetic inputs are not ESP32 compilation evidence.')
