#!/usr/bin/env python3
"""Offline evidence probes; synthetic inputs never count as a target build."""
from pathlib import Path
import copy,functools,hashlib,operator,runpy,shlex,struct,sys,tempfile
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from record_coopdiag77_identity import check_options,check_image,check_compile_log,FQBN
# Retain all 109 old envelope/ODR/object/delta probes against their exact helper.
runpy.run_path(str(ROOT/'tests/coop_training_build_evidence.py'),run_name='__main__')
probes=0
def reject(fn):
 global probes
 try:fn()
 except (AssertionError,KeyError,ValueError):probes+=1
 else:raise AssertionError('Accepted invalid coopdiag77 evidence')
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
valid=image(b'2026.10.15-coopdiag77\0fixture');assert check_image(valid)['coopdiag77_identity_present'];probes+=1
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
print(f'PASS: {probes} coopdiag77 default-ON/forced-OFF, exact compiler properties/commands, image identity/envelope probes; plus all109 inherited build-evidence probes. Synthetic only.')

# New diagnostics evidence is required in addition to every inherited target
# check. These fabricated files only exercise fail-closed validators.
from record_coopdiag77_identity import inspect_diagnostics,DIAGNOSTIC_OBJECT_LIMITS,verify_radio_evidence
symbols='\n'.join(f'00001000 {limit:08x} B {name}' for name,limit in DIAGNOSTIC_OBJECT_LIMITS.items())
assert len(inspect_diagnostics(symbols,1))==4 and inspect_diagnostics('',0)=={};probes+=2
reject(lambda:inspect_diagnostics(symbols,0));reject(lambda:inspect_diagnostics('',1))
for name,limit in DIAGNOSTIC_OBJECT_LIMITS.items():
 reject(lambda name=name:inspect_diagnostics('\n'.join(line for line in symbols.splitlines() if not line.endswith(name)),1))
 reject(lambda name=name,limit=limit:inspect_diagnostics(symbols+'\n'+f'00002000 {limit:08x} B {name}',1))
 reject(lambda name=name,limit=limit:inspect_diagnostics(symbols.replace(f'{limit:08x} B {name}',f'{limit+1:08x} B {name}'),1))
with tempfile.TemporaryDirectory() as temporary:
 root=Path(temporary);source=root/'probe.cpp';source.write_text('synthetic evidence probe\n');sha=hashlib.sha256(source.read_bytes()).hexdigest()
 strict={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1:strict_string_checks=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1','LSAN_OPTIONS':'exitcode=23'}
 report={'passed':True,'mode':'both','feature_enabled':1,'cases':[{'mode':'normal','passed':True},{'mode':'sanitizers','passed':True,'sanitizer_environment':strict}],'sources_sha256':{'probe.cpp':sha}}
 paths=[]
 for name in ('coop-radio-recovery','coop-radio-device','coop-radio-runtime-ui'):
  path=root/'build'/name/'results.json';path.parent.mkdir(parents=True);path.write_text(__import__('json').dumps(report));paths.append(path)
 folder=root/'build/coop-radio-render';folder.mkdir();(folder/'normal').mkdir();(folder/'sanitized').mkdir()
 names=['discovery-no-sample','discovery-strong','discovery-weak','discovery-stale','accept-with-signal','playing-with-signal','recovery-dots-1','recovery-dots-2','recovery-dots-3','recovery-no-art','diagnostics-live','diagnostics-no-sample','diagnostics-extremes','terminal-5','terminal-8','terminal-1','terminal-2','terminal-4']
 captures=[]
 for name in names:
  for directory in ('normal','sanitized'):
   for suffix in ('.ppm','-labels.tsv'):(folder/directory/(name+suffix)).write_text(name)
  (folder/(name+'.png')).write_text(name)
  digest=hashlib.sha256(name.encode()).hexdigest();captures.append({'name':name,'ppm_sha256':digest,'png_sha256':digest})
 render={'passed':True,'mode':'both','feature_enabled':1,'sanitized':True,'pixel_parity':True,'sanitizer_environment':strict,'source_sha256':{'probe.cpp':sha},'captures':captures}
 manifest=folder/'manifest.json';manifest.write_text(__import__('json').dumps(render));assert len(verify_radio_evidence(root))==4;probes+=1
 for path in paths:
  for key,value in [('passed',False),('mode','normal'),('feature_enabled',0),('cases',report['cases'][:1]),('sources_sha256',{}),('sources_sha256',{'../outside':sha}),('sources_sha256',{'probe.cpp':'0'*64}),('source_root','/wrong')]:
   bad=copy.deepcopy(report);bad[key]=value;path.write_text(__import__('json').dumps(bad));reject(lambda:verify_radio_evidence(root))
  bad=copy.deepcopy(report);bad['cases'][1]['sanitizer_environment']['ASAN_OPTIONS']='detect_leaks=0';path.write_text(__import__('json').dumps(bad));reject(lambda:verify_radio_evidence(root));path.write_text(__import__('json').dumps(report))
 for key,value in [('passed',False),('feature_enabled',0),('sanitized',False),('pixel_parity',False),('captures',captures[:-1]),('captures',captures[:-1]+[captures[0]]),('source_sha256',{})]:
  bad=copy.deepcopy(render);bad[key]=value;manifest.write_text(__import__('json').dumps(bad));reject(lambda:verify_radio_evidence(root))
 manifest.write_text(__import__('json').dumps(render))
 for path in [folder/'normal'/f'{names[0]}.ppm',folder/'sanitized'/f'{names[0]}.ppm',folder/'sanitized'/f'{names[0]}-labels.tsv',folder/f'{names[0]}.png',source]:
  original=path.read_bytes();path.write_bytes(original+b'changed');reject(lambda:verify_radio_evidence(root));path.write_bytes(original)
print(f'PASS: {probes} current identity/build/diagnostic evidence probes plus all109 historical probes; synthetic only.')
