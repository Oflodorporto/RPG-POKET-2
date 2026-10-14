#!/usr/bin/env python3
"""Record experimental compile evidence. Never copy/install/package firmware."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import subprocess
import struct
import functools
import operator

ROOT=Path(__file__).resolve().parents[1]
BASE='6a23dda54114db611b818b51e4b1b942019098fa'
CANDIDATE='b117da8bd46eb1cb3fa4e051654dc9e32c6f126e'
CANDIDATE_TREE='56a7bba85c83ba3584589502a81f3c69cde5dc01'
FQBN='esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,EraseFlash=none'
CORE='3.3.12'
OTA_BYTES=3145728
MAX_ADDED_IMAGE=262144
MAX_ADDED_STATIC_RAM=16384
OBJECT_LIMITS={'coopSession':6144,'coopInput':1024,'coopDevice':256}
EVIDENCE_NAMES={'toolchain.txt','esp32-flag-0.log','esp32-flag-1.log','sections-flag-0.txt','sections-flag-1.txt','coop-symbols-flag-0.txt','coop-symbols-flag-1.txt','experimental-build-evidence.json','NOT_FOR_INSTALL.txt'}

def git(*args):
    return subprocess.check_output(['git',*args],cwd=ROOT,text=True).strip()

def digest(path):
    data=path.read_bytes()
    return {'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}

def parse_memory(log):
    flash=re.findall(r'Sketch uses ([\d,]+) bytes .*?Maximum is ([\d,]+) bytes',log)
    ram=re.findall(r'Global variables use ([\d,]+) bytes .*?Maximum is ([\d,]+) bytes',log)
    assert len(flash)==len(ram)==1,'Missing/ambiguous Arduino memory summary'
    used,maximum=(int(v.replace(',','')) for v in flash[0])
    static,ram_max=(int(v.replace(',','')) for v in ram[0])
    assert maximum==OTA_BYTES and 0<used<=maximum
    assert 0<static<ram_max<=512*1024
    return {'arduino_program_bytes':used,'application_partition_bytes':maximum,
            'linker_static_ram_bytes':static,'arduino_static_ram_limit_bytes':ram_max}

def board_options(text):
    package,architecture,board,settings=text.split(':',3)
    pairs=[item.split('=',1) for item in settings.split(',')]
    assert all(len(p)==2 for p in pairs) and len(dict(pairs))==len(pairs),'Malformed/duplicate FQBN option'
    return package,architecture,board,dict(pairs)

def check_options(options,flag):
    assert board_options(options['fqbn'])==board_options(FQBN),'Unexpected actual FQBN'
    serialized=json.dumps(options,sort_keys=True)
    # Arduino CLI stores customBuildProperties as a comma-joined list. This
    # workflow deliberately supplies one simple, comma-free C++ flag property.
    raw=options['customBuildProperties'];assert isinstance(raw,str)
    pairs=[item.split('=',1) for item in raw.split(',')]
    assert all(len(p)==2 for p in pairs) and len(dict(pairs))==len(pairs)
    props=dict(pairs)
    assert props.pop('compiler.cpp.extra_flags')=='-DRPG_ENABLE_COOP_TRAINING='+str(flag),'Exact effective C++ define required'
    rest=dict(options);rest.pop('customBuildProperties')
    assert 'RPG_ENABLE_COOP_TRAINING' not in json.dumps([rest,props]),'Training define outside its approved property'
    return {'fqbn':options['fqbn'],'training_define':int(flag),'options_sha256':hashlib.sha256(serialized.encode()).hexdigest()}

def check_image(data):
    # Unsigned ESP image format, not a boot/OTA or secure-boot qualification.
    assert 24<=len(data)<=OTA_BYTES and data[0]==0xe9 and 1<=data[1]<=16
    assert struct.unpack_from('<H',data,12)[0]==9,'Expected ESP32-S3 image'
    assert data[23]==1,'Expected esptool appended SHA256'
    pos=24;checksum=0xef
    for _ in range(data[1]):
        assert pos+8<=len(data),'Truncated segment header'
        length=struct.unpack_from('<I',data,pos+4)[0];pos+=8
        assert pos+length<=len(data),'Truncated segment payload'
        checksum=functools.reduce(operator.xor,data[pos:pos+length],checksum);pos+=length
    footer=pos|15
    assert footer+33==len(data),'Unexpected image padding/signature/trailer'
    assert not any(data[pos:footer]) and data[footer]==checksum,'Segment checksum/padding mismatch'
    assert hashlib.sha256(data[:footer+1]).digest()==data[footer+1:],'Appended image digest mismatch'
    assert b'2026.10.14-pvp77\0' in data,'Inherited firmware identity absent'
    return {'chip_id':9,'segment_count':data[1],'segment_checksum_verified':True,'appended_sha256_verified':True,'inherited_version_present':True}

def parse_symbols(text,flag):
    symbols={}
    for line in text.splitlines():
        m=re.match(r'^\s*[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+\S\s+(.+)$',line)
        if m:
            symbols.setdefault(m[2],[]).append(int(m[1],16))
    if not flag:
        assert not any(any(token in name for token in ('coopSession','coopDevice','coopInput','coopradio::','coopplatform::')) for name in symbols),'Coop runtime present in flag0 target ELF'
        return {}
    sizes={}
    for name,limit in {**OBJECT_LIMITS,'coopradio::queueBytes':3072,'coopradio::queueState':3072}.items():
        matches=[v for k,values in symbols.items() if k==name or k.startswith(name+'.') for v in values]
        assert len(matches)==1,('Required target object absent/ambiguous',name)
        assert 0<matches[0]<=limit,('Target static object budget exceeded',name,matches)
        sizes[name]=matches[0]
    assert sizes['coopradio::queueBytes']+sizes['coopradio::queueState']<=3072
    return sizes

def inspect_shared_linkage(text,flag):
    """Enforce singleton mutable/art storage; report inherited font copies."""
    symbols={}
    for line in text.splitlines():
        m=re.match(r'^\s*[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+(\S)\s+(.+)$',line)
        if m:symbols.setdefault(m[3],[]).append({'bytes':int(m[1],16),'kind':m[2]})
    shared=('artMemory','enemyArt::workspace','coopradio::owner','coopradio::callbackCounters')
    for name in shared:
        entries=symbols.get(name,[])
        assert len(entries)<=1,('Duplicate shared mutable state',name)
        if flag and name!='enemyArt::workspace':assert len(entries)==1,('Missing shared state',name)
    tables=('assetMeta','artFallback','raceAsset','raceMask','enemyArt::enemies','enemyScale::profiles','font','outfitColors','coopview::kHeroCrop','ruins7Art::palette','ruins7Art::framePixels')
    for name in tables:
        if name!='font':assert len(symbols.get(name,[]))<=1,('Duplicate art table',name)
    return {'shared_mutable':{name:symbols.get(name,[]) for name in shared},
            'readonly_tables':{name:symbols.get(name,[]) for name in tables}}

def compare_budgets(off,on):
    image=on['image']['bytes']-off['image']['bytes']
    ram=on['memory']['linker_static_ram_bytes']-off['memory']['linker_static_ram_bytes']
    assert 0<image<=MAX_ADDED_IMAGE,('Coop image increment outside candidate cap',image)
    assert 0<ram<=MAX_ADDED_STATIC_RAM,('Coop static RAM increment outside candidate cap',ram)
    return {'added_image_bytes':image,'added_static_ram_bytes':ram,
            'added_image_cap_bytes':MAX_ADDED_IMAGE,'added_static_ram_cap_bytes':MAX_ADDED_STATIC_RAM,
            'image_partition_margin_bytes':OTA_BYTES-on['image']['bytes']}

def find_tool(folder,suffix):
    for name in ('xtensa-esp32s3-elf-'+suffix,'xtensa-esp-elf-'+suffix):
        found=list(folder.glob('**/bin/'+name))
        if found:
            assert len(found)==1,('Ambiguous installed target tool',found)
            return found[0]
    raise AssertionError('ESP32 Xtensa '+suffix+' tool missing')

def validate_evidence_directory(folder,complete=False):
    names=set()
    for path in folder.iterdir():
        assert path.is_file() and not path.is_symlink(),path
        assert path.name in EVIDENCE_NAMES,('Non-evidence artifact',path)
        names.add(path.name)
    if complete:assert names==EVIDENCE_NAMES,'Evidence file missing'

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--build-root',type=Path,required=True)
    parser.add_argument('--evidence-dir',type=Path,required=True)
    parser.add_argument('--tools-dir',type=Path,default=Path.home()/'.arduino15/packages/esp32/tools')
    args=parser.parse_args()
    sha=git('rev-parse','HEAD')
    assert git('rev-parse',CANDIDATE+'^{tree}')==CANDIDATE_TREE,'Reviewed source tree mismatch'
    assert os.environ['GITHUB_SHA']==sha
    assert os.environ['GITHUB_REF']=='refs/heads/feat/coop-training-check' and os.environ['GITHUB_EVENT_NAME']=='push'
    assert not git('status','--porcelain','--untracked-files=all'),'Source must remain exact and clean'
    build=args.build_root.resolve();out=args.evidence_dir.resolve()
    assert not build.is_relative_to(ROOT),'Compiler output must be external to the checkout'
    assert out==ROOT/'build/coop-training-evidence','Use the dedicated evidence-only staging directory'
    validate_evidence_directory(out)
    toolchain=(out/'toolchain.txt').read_text()
    assert re.search(r'^esp32:esp32\s+3\.3\.12(?:\s|$)',toolchain,re.M),'Installed core differs from 3.3.12'
    assert 'arduino-cli' in toolchain.lower()
    size=find_tool(args.tools_dir,'size');nm=find_tool(args.tools_dir,'nm')
    builds={}
    for flag in (0,1):
        folder=build/('flag-'+str(flag));elf=folder/'RPG_POKET_2.ino.elf';binary=folder/'RPG_POKET_2.ino.bin'
        assert elf.is_file() and binary.is_file() and not elf.is_symlink() and not binary.is_symlink()
        options=json.loads((folder/'build.options.json').read_text());effective=check_options(options,flag)
        image=digest(binary);image_format=check_image(binary.read_bytes())
        log=out/('esp32-flag-'+str(flag)+'.log');memory=parse_memory(log.read_text())
        sections=subprocess.check_output([str(size),'-A',str(elf)],text=True)
        symbols=subprocess.check_output([str(nm),'-S','-C','--defined-only',str(elf)],text=True)
        object_sizes=parse_symbols(symbols,flag)
        linkage=inspect_shared_linkage(symbols,flag)
        translation_units={}
        for name in ('RPG_POKET_2.ino.cpp.o','CoopRuntime.cpp.o'):
            obj=folder/'sketch'/name
            assert obj.is_file() and not obj.is_symlink(),('Expected target object absent',obj)
            obj_sections=subprocess.check_output([str(size),'-A',str(obj)],text=True)
            obj_symbols=subprocess.check_output([str(nm),'-S','-C','--defined-only',str(obj)],text=True)
            if not flag and name=='CoopRuntime.cpp.o':assert not obj_symbols.strip(),'Disabled runtime object has definitions'
            if name=='CoopRuntime.cpp.o':
                assert not re.search(r' (?:assetMeta|artFallback|font|artMemory)$|enemyArt::workspace|ruins7Art::',obj_symbols,re.M),'Art storage emitted by runtime object'
            translation_units[name]={**digest(obj),'defined_symbol_lines':len(obj_symbols.splitlines())}
            sections+='\nTRANSLATION UNIT '+name+'\n'+obj_sections
        (out/('sections-flag-'+str(flag)+'.txt')).write_text(sections)
        # Keep a small, inspectable symbol extract instead of the entire ELF.
        (out/('coop-symbols-flag-'+str(flag)+'.txt')).write_text('\n'.join(line for line in symbols.splitlines() if any(token in line for token in ('coopSession','coopInput','coopDevice','coopradio::','artMemory','enemyArt::workspace','assetMeta','artFallback','raceAsset','raceMask','enemyArt::enemies','enemyScale::profiles',' font','outfitColors','coopview::kHeroCrop','ruins7Art::palette','ruins7Art::framePixels'))) +'\n')
        builds[str(flag)]={'experimental_identity':'coop-training-qualification/'+sha+'/flag-'+str(flag),
            'feature_enabled':bool(flag),'effective_options':effective,'image':image,'image_format':image_format,'elf':digest(elf),
            'build_log':digest(log),'memory':memory,'target_object_bytes':object_sizes,'translation_units':translation_units,'linkage_audit':linkage,
            'binary_uploaded':False,'install_authorized':False}
    budget=compare_budgets(builds['0'],builds['1'])
    sources={name:digest(ROOT/name) for name in git('ls-files').splitlines()}
    identity={'schema':1,'kind':'EXPERIMENTAL_COMPILE_EVIDENCE_ONLY','source_commit':sha,'source_tree':git('rev-parse','HEAD^{tree}'),
        'base_commit':BASE,'reviewed_integration_commit':CANDIDATE,'reviewed_integration_tree':CANDIDATE_TREE,'branch':os.environ['GITHUB_REF'],
        'event':os.environ['GITHUB_EVENT_NAME'],'run_id':os.environ.get('GITHUB_RUN_ID'),'core':CORE,'fqbn':FQBN,
        'inherited_release_identity':{'version':'2026.10.14-pvp77','build':2026101404,'package':'7.7 - Duelo PvP',
            'header_unchanged':(ROOT/'firmware/RPG_POKET_2/ReleaseVersion.h').read_text()==subprocess.check_output(['git','show',BASE+':firmware/RPG_POKET_2/ReleaseVersion.h'],cwd=ROOT,text=True)},
        'experimental_builds':builds,'compiled_budget':budget,'source_inventory':sources,
        'save_format':30,'pvp_protocol_version':5,'pvp_packet_bytes':52,'existing_art_count':666,
        'existing_known_psram_bytes':7638272,'additional_explicit_psram_allocations':0,
        'native_suite':'6 optimized + 5 ASan/UBSan + 3 existing PvP, including Save30/RNG checks',
        'target_compile_verified':True,'physical_validation':False,'free_heap_measured':False,'stack_high_water_measured':False,
        'radio_touch_wifi_validated_on_hardware':False,'release_qualified':False,'published':False,'install_authorized':False,
        'note':'Both binaries retain the inherited PvP release header. They are experimental build outputs, never uploaded or offered for installation. Linker sizes are not measured heap/stack/PSRAM/FPS.'}
    assert identity['inherited_release_identity']['header_unchanged']
    (out/'experimental-build-evidence.json').write_text(json.dumps(identity,indent=2,sort_keys=True)+'\n')
    (out/'NOT_FOR_INSTALL.txt').write_text('EXPERIMENTAL COMPILE EVIDENCE ONLY\nNo installable binary is included. This is not a release qualification.\nTwo-board RF, touch, Wi-Fi restoration, heap/stack and pacing tests remain pending.\n')
    validate_evidence_directory(out,complete=True)
    print('PASS: exact source, target flag0/flag1, effective defines, inherited identity, ELF object caps, image/static RAM delta, evidence-only staging; hardware validation pending')

if __name__=='__main__':main()
