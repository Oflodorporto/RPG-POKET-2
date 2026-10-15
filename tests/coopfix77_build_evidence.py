#!/usr/bin/env python3
"""Offline evidence probes; synthetic inputs never count as a target build."""
from pathlib import Path
import copy,functools,hashlib,operator,runpy,shlex,struct,sys,tempfile
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from record_coopfix77_identity import check_options,check_image,check_compile_log,FQBN
# Retain all 109 old envelope/ODR/object/delta probes against their exact helper.
runpy.run_path(str(ROOT/'tests/coop_training_build_evidence.py'),run_name='__main__')
probes=0
def reject(fn):
 global probes
 try:fn()
 except (AssertionError,KeyError,ValueError):probes+=1
 else:raise AssertionError('Accepted invalid coopfix77 evidence')
for flag in (0,1):
 options={'fqbn':FQBN,'customBuildProperties':'build.warn_data_percentage=75'+('' if flag else ',compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=0')}
 assert check_options(options,flag)['default_arduino_enabled']==bool(flag);probes+=1
 for change in ('compiler.c.extra_flags=-Os','compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=1','compiler.cpp.extra_flags=-mtext-section-literals','build.extra_flags=-DRPG_ENABLE_COOP_TRAINING=0','compiler.c.elf.extra_flags=-Wl,--relax'):
  bad=copy.deepcopy(options);bad['customBuildProperties']+=','+change;reject(lambda:check_options(bad,flag))
 bad=copy.deepcopy(options);bad['fqbn']=FQBN.replace('PSRAM=opi','PSRAM=disabled');reject(lambda:check_options(bad,flag))
 bad=copy.deepcopy(options);bad['customBuildProperties']+=',build.warn_data_percentage=75';reject(lambda:check_options(bad,flag))
 bad=copy.deepcopy(options);bad['unrelated']='RPG_ENABLE_COOP_TRAINING=0';reject(lambda:check_options(bad,flag))
 reject(lambda:check_options(options,1-flag))
def image(payload):
 head=bytearray(24);head[0]=0xe9;head[1]=1;head[12]=9;head[23]=1
 data=head+struct.pack('<II',0x3c000000,len(payload))+payload
 data+=bytes(15-len(data)%16)+bytes([functools.reduce(operator.xor,payload,0xef)])
 data+=hashlib.sha256(data).digest();return data
valid=image(b'2026.10.15-coopfix77\0fixture');assert check_image(valid)['coopfix77_identity_present'];probes+=1
for data in (image(b'2026.10.14-shop77\0fixture'),image(b'2026.10.14-pvp77\0fixture'),valid+b'junk',valid[:-1]):reject(lambda data=data:check_image(data))
for offset in (0,1,12,23,33,-1):
 bad=bytearray(valid);bad[offset]^=0xff;reject(lambda:check_image(bad))
with tempfile.TemporaryDirectory() as d:
 root=Path(d);flags=root/'3.3.12/flags/cpp_flags';flags.parent.mkdir(parents=True);flags.write_text('-mlongcalls -ffunction-sections -fdata-sections\n')
 for flag in (0,1):
  log='\n'.join(shlex.join([str(root/'esp-x32/2601/bin/xtensa-esp32s3-elf-g++'),* ([] if flag else ['-DRPG_ENABLE_COOP_TRAINING=0']),'-c','@'+str(flags),'-Os','-DARDUINO=10607','-DARDUINO_ARCH_ESP32',str(root/name),'-o',str(root/(name+'.o'))]) for name in ('RPG_POKET_2.ino.cpp','CoopRuntime.cpp','UpdateInstall.cpp'))
  assert len(check_compile_log(log,flag))==3;probes+=1
  for bad in (log+log,log.replace('-DARDUINO=10607',''),log.replace('-DARDUINO_ARCH_ESP32',''),log.replace('/esp-x32/2601/','/esp-x32/other/'),log.replace('-Os','-O0'),log.replace(' -c ',' -E '),log.replace('CoopRuntime.cpp.o','wrong.cpp.o')):reject(lambda bad=bad:check_compile_log(bad,flag))
  reject(lambda:check_compile_log(log,1-flag))
print(f'PASS: {probes} coopfix77 default-ON/forced-OFF, exact compiler properties/commands, image identity/envelope probes; plus all109 inherited build-evidence probes. Synthetic only.')
