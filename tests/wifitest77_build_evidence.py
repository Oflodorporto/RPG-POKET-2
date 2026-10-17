#!/usr/bin/env python3
"""Offline evidence probes; synthetic inputs never count as a target build."""
if not __debug__:
    raise RuntimeError("Qualification requires assertions; optimized Python is forbidden")
from pathlib import Path
import copy,functools,hashlib,operator,runpy,shlex,struct,sys,tempfile
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from record_wifitest77_identity import check_options,check_image,check_compile_log,FQBN,VARIANTS
# Retain all 109 old envelope/ODR/object/delta probes against their exact helper.
runpy.run_path(str(ROOT/'tests/coop_training_build_evidence.py'),run_name='__main__')
probes=0
def reject(fn):
 global probes
 try:fn()
 except (AssertionError,KeyError,ValueError):probes+=1
 else:raise AssertionError('Accepted invalid wifitest77 evidence')
for variant,(coop,radio,override) in VARIANTS.items():
 options={'fqbn':FQBN,'customBuildProperties':'build.warn_data_percentage=75'+('' if override is None else ',compiler.cpp.extra_flags='+override)}
 assert check_options(options,variant)['default_arduino_enabled']==(override is None);probes+=1
 for change in ('compiler.c.extra_flags=-Os','compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=1','compiler.cpp.extra_flags=-DRPG_ENABLE_WIFI_RADIO_TEST=1','compiler.cpp.extra_flags=-mtext-section-literals','build.extra_flags=-DRPG_ENABLE_COOP_TRAINING=0','compiler.c.elf.extra_flags=-Wl,--relax'):
  bad=copy.deepcopy(options);bad['customBuildProperties']+=','+change;reject(lambda:check_options(bad,variant))
 bad=copy.deepcopy(options);bad['fqbn']=FQBN.replace('PSRAM=opi','PSRAM=disabled');reject(lambda:check_options(bad,variant))
 bad=copy.deepcopy(options);bad['customBuildProperties']+=',build.warn_data_percentage=75';reject(lambda:check_options(bad,variant))
 bad=copy.deepcopy(options);bad['unrelated']='RPG_ENABLE_COOP_TRAINING=0';reject(lambda:check_options(bad,variant))
 for other in set(VARIANTS)-{variant}:reject(lambda other=other:check_options(options,other))
def image(payload):
 head=bytearray(24);head[0]=0xe9;head[1]=1;head[12]=9;head[23]=1
 data=head+struct.pack('<II',0x3c000000,len(payload))+payload
 data+=bytes(15-len(data)%16)+bytes([functools.reduce(operator.xor,payload,0xef)])
 data+=hashlib.sha256(data).digest();return data
valid=image(b'2026.10.17-wifitest77\0fixture');assert check_image(valid)['wifitest77_identity_present'];probes+=1
for data in (image(b'2026.10.14-shop77\0fixture'),image(b'2026.10.14-pvp77\0fixture'),valid+b'junk',valid[:-1]):reject(lambda data=data:check_image(data))
for offset in (0,1,12,23,33,-1):
 bad=bytearray(valid);bad[offset]^=0xff;reject(lambda:check_image(bad))
with tempfile.TemporaryDirectory() as d:
 root=Path(d);flags=root/'3.3.12/flags/cpp_flags';flags.parent.mkdir(parents=True);flags.write_text('-mlongcalls -ffunction-sections -fdata-sections\n')
 for variant,(coop,radio,override) in VARIANTS.items():
  log='\n'.join(shlex.join([str(root/'esp-x32/2601/bin/xtensa-esp32s3-elf-g++'),* ([] if override is None else [override]),'-c','@'+str(flags),'-Os','-DARDUINO=10607','-DARDUINO_ARCH_ESP32',str(root/name),'-o',str(root/(name+'.o'))]) for name in ('RPG_POKET_2.ino.cpp','CoopRuntime.cpp','UpdateInstall.cpp'))
  assert len(check_compile_log(log,variant))==3;probes+=1
  for bad in (log+log,log.replace('-DARDUINO=10607',''),log.replace('-DARDUINO_ARCH_ESP32',''),log.replace('/esp-x32/2601/','/esp-x32/other/'),log.replace('-Os','-O0'),log.replace(' -c ',' -E '),log.replace('CoopRuntime.cpp.o','wrong.cpp.o')):reject(lambda bad=bad:check_compile_log(bad,variant))
  for other in set(VARIANTS)-{variant}:reject(lambda other=other:check_compile_log(log,other))
print(f'PASS: {probes} wifitest77 default-ON/forced-OFF, exact compiler properties/commands, image identity/envelope probes; plus all109 inherited build-evidence probes. Synthetic only.')

# New diagnostics evidence is required in addition to every inherited target
# check. These fabricated files only exercise fail-closed validators.
from record_wifitest77_identity import inspect_diagnostics,DIAGNOSTIC_OBJECT_LIMITS,verify_radio_evidence
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

from record_wifitest77_identity import RADIO_OBJECT_LIMITS,inspect_wifi_symbols,inspect_training_symbols
wifi_symbols='\n'.join(f'00002000 {limit:08x} B {name}' for name,limit in RADIO_OBJECT_LIMITS.items())
assert inspect_wifi_symbols(wifi_symbols,1)==RADIO_OBJECT_LIMITS and inspect_wifi_symbols('',0)=={};probes+=2
reject(lambda:inspect_wifi_symbols(wifi_symbols,0));reject(lambda:inspect_wifi_symbols('',1))
for name,limit in RADIO_OBJECT_LIMITS.items():
 reject(lambda name=name:inspect_wifi_symbols('\n'.join(line for line in wifi_symbols.splitlines() if not line.endswith(name)),1))
 reject(lambda name=name,limit=limit:inspect_wifi_symbols(wifi_symbols+'\n'+f'00003000 {limit:08x} B {name}',1))
 reject(lambda name=name,limit=limit:inspect_wifi_symbols(wifi_symbols.replace(f'{limit:08x} B {name}',f'{limit+1:08x} B {name}'),1))
assert inspect_training_symbols('00001000 00000001 B coopradio::owner',0)=={};probes+=1
for name in ('coopSession','coopDevice','coopInput','coopradio::queueBytes','coopradio::diagnostic','coopplatform::device','coopradio::owner.extra'):
 reject(lambda name=name:inspect_training_symbols('00001000 00000001 B '+name,0))
print(f'PASS: {probes} wifitest77 three-variant/default identity, inherited diagnostics, radio singleton/budget and narrowly shared-owner probes; synthetic only.')

# Report fixtures prove rejection behavior only, never replace a completed suite
# or actual-byte OTA report in the source identity recorder.
import json
from record_wifitest77_identity import (verify_wifi_suite_evidence,verify_ota_evidence,
    RADIO_SUITES,RADIO_TEST_SOURCES,OTA_GATE_SOURCES,RADIO_CAPTURES)
from test_wifitest77_release import HISTORY
with tempfile.TemporaryDirectory(prefix='wifitest77-report-probes-') as temporary:
 root=Path(temporary)
 names=set(RADIO_TEST_SOURCES.values())|set(OTA_GATE_SOURCES)|{'tools/test_wifi_radio_test.py'}
 for name in names:
  path=root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text('synthetic source '+name)
 hashes={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in names}
 strict={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1','LSAN_OPTIONS':'exitcode=23'}
 radio_paths=[]
 for suite in RADIO_SUITES:
  for mode in ('normal','sanitizers'):
   folder=root/'build/wifi-radio-test'/suite/mode;folder.mkdir(parents=True)
   logs={}
   for kind in ('compile','run'):
    path=folder/(kind+'.log');argv=['g++','-UNDEBUG',*(['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie','-O1'] if mode=='sanitizers' else ['-O2'])];path.write_text('argv: '+json.dumps(argv)+'\n' if kind=='compile' else 'passed fixture\n')
    logs[kind]={'path':str(path.relative_to(root)),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
   report={'suite':suite,'passed':True,'sanitizer':mode=='sanitizers','compile_exit_code':0,'run_exit_code':0,'source_error':None,'capture_error':None,'sources_changed_during_execution':[],'sources_sha256':hashes,
           'source_sha256':hashlib.sha256(json.dumps(hashes,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
           'logs':logs,'sanitizer_environment':strict if mode=='sanitizers' else {}}
   if suite in RADIO_CAPTURES:
    report['captures']={}
    for name in RADIO_CAPTURES[suite]:
     path=folder/name;path.write_bytes(b'P6\n240 320\n255\n'+bytes(240*320*3))
     report['captures'][name]={'path':str(path.relative_to(root)),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
   path=folder/'results.json';path.write_text(json.dumps(report));radio_paths.append((path,report))
 assert len(verify_wifi_suite_evidence(root))==8;probes+=1
 for path,report in radio_paths:
  for key,value in [('suite','wrong'),('passed',False),('compile_exit_code',1),('run_exit_code',23),('compile_exit_code',False),('run_exit_code',0.0),('source_error','invalid'),('capture_error','invalid'),('sources_changed_during_execution',['probe']),('sanitizer',not report['sanitizer']),('source_sha256','0'*64),('sources_sha256',{}),('logs',{})]:
   bad=copy.deepcopy(report);bad[key]=value;path.write_text(json.dumps(bad));reject(lambda:verify_wifi_suite_evidence(root))
  if report['sanitizer']:
   bad=copy.deepcopy(report);bad['sanitizer_environment']['ASAN_OPTIONS']='detect_leaks=0';path.write_text(json.dumps(bad));reject(lambda:verify_wifi_suite_evidence(root))
  else:
   bad=copy.deepcopy(report);bad['sanitizer_environment']={'ASAN_OPTIONS':'detect_leaks=0'};path.write_text(json.dumps(bad));reject(lambda:verify_wifi_suite_evidence(root))
  if report['suite'] in RADIO_CAPTURES:
   bad=copy.deepcopy(report);bad['captures'].pop(next(iter(bad['captures'])));path.write_text(json.dumps(bad));reject(lambda:verify_wifi_suite_evidence(root))
  path.write_text(json.dumps(report))
 for path,report in radio_paths:
  if report['suite'] not in RADIO_CAPTURES:continue
  name=next(iter(report['captures']));capture=root/report['captures'][name]['path'];data=capture.read_bytes()
  capture.write_bytes(data[:-1]);reject(lambda:verify_wifi_suite_evidence(root));capture.write_bytes(data)
  bad=copy.deepcopy(report);capture.write_bytes(data[:-1]+b'X');bad['captures'][name]['sha256']=hashlib.sha256(capture.read_bytes()).hexdigest();path.write_text(json.dumps(bad));reject(lambda:verify_wifi_suite_evidence(root));capture.write_bytes(data);path.write_text(json.dumps(report))
 path,report=radio_paths[0]
 for kind in ('compile','run'):
  log=root/report['logs'][kind]['path'];data=log.read_bytes();log.write_bytes(data+b'changed');reject(lambda:verify_wifi_suite_evidence(root));log.write_bytes(data)
 for invalid in ('../escape','/tmp/outside'):
  bad=copy.deepcopy(report);bad['logs']['compile']['path']=invalid;path.write_text(json.dumps(bad));reject(lambda:verify_wifi_suite_evidence(root))
 path.write_text(json.dumps(report))
 compile_log=root/report['logs']['compile']['path'];original=compile_log.read_bytes()
 for args in (['g++','-UNDEBUG','-O0'],['g++','-O2'],['g++','-UNDEBUG','-DNDEBUG'],['g++','-UNDEBUG','-DNDEBUG=1'],['g++','-UNDEBUG','-UNDEBUG'],['g++','-UNDEBUG','-fsanitize=address,undefined']):
  compile_log.write_text('argv: '+json.dumps(args)+'\n');bad=copy.deepcopy(report);bad['logs']['compile']['sha256']=hashlib.sha256(compile_log.read_bytes()).hexdigest();path.write_text(json.dumps(bad));reject(lambda:verify_wifi_suite_evidence(root))
 compile_log.write_bytes(original);path.write_text(json.dumps(report))
 source=root/RADIO_TEST_SOURCES['model'];data=source.read_bytes();source.write_bytes(data+b'changed');reject(lambda:verify_wifi_suite_evidence(root));source.write_bytes(data)
 # Actual-byte report validator uses fake triples solely within this isolated unit test.
 (root/'release').mkdir()
 inputs={}
 for name in ('firmware.bin','artes.pak','manifest.json'):
  path=root/'release'/name;path.write_text('synthetic '+name);inputs[name]={'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
 sha='a'*40;tree='b'*40;clients={}
 for label,commit in [*( (label,commit) for label,commit,_,_ in HISTORY),('wifitest77',sha)]:
  headers={'fixture.h':'c'*64};clients[label]={'source_commit':commit,'headers_sha256':headers,'closure_sha256':hashlib.sha256(json.dumps(headers,sort_keys=True).encode()).hexdigest(),'split_installer':label in ('coop77','coopfix77','coopdiag77','wifitest77')}
 sources={name:hashes[name] for name in OTA_GATE_SOURCES}
 folder=root/'build/wifitest77-ota';folder.mkdir()
 paths=[]
 for mode in ('normal','sanitizers'):
  report={'schema':1,'passed':True,'synthetic':False,'mode':mode,'source_commit':sha,'source_tree':tree,'inputs':inputs,'historical_clients':13,'clients':clients,'sources_sha256':sources,
          'sanitizer_environment':{**strict,'RPG_DETECT_LEAKS':'1'} if mode=='sanitizers' else {},
          'save29_to30':True,'save30_to30':True,'all_three_slots':True,'real_sha256':True,'historical_headers_unchanged':True,'no_load_or_rollback_writes':True,'no_new_reset':True,'physical_validation':False,'published':False}
  path=folder/('release-'+mode+'.json');path.write_text(json.dumps(report));paths.append((path,report))
 assert set(verify_ota_evidence(root,sha,tree))=={'normal','sanitizers'};probes+=1
 for path,report in paths:
  for key,value in [('passed',False),('synthetic',True),('source_commit','c'*40),('source_tree','d'*40),('historical_clients',12),('clients',{}),('inputs',{}),('sources_sha256',{}),('mode','wrong'),('save29_to30',False),('save30_to30',False),('no_new_reset',False),('physical_validation',True)]:
   bad=copy.deepcopy(report);bad[key]=value;path.write_text(json.dumps(bad));reject(lambda:verify_ota_evidence(root,sha,tree))
  for label in report['clients']:
   bad=copy.deepcopy(report);bad['clients'].pop(label);path.write_text(json.dumps(bad));reject(lambda:verify_ota_evidence(root,sha,tree))
   bad=copy.deepcopy(report);bad['clients'][label]['closure_sha256']='0'*64;path.write_text(json.dumps(bad));reject(lambda:verify_ota_evidence(root,sha,tree))
  if report['mode']=='sanitizers':
   bad=copy.deepcopy(report);bad['sanitizer_environment']['ASAN_OPTIONS']='detect_leaks=0';path.write_text(json.dumps(bad));reject(lambda:verify_ota_evidence(root,sha,tree))
  path.write_text(json.dumps(report))
 for name in ('firmware.bin','artes.pak','manifest.json'):
  path=root/'release'/name;data=path.read_bytes();path.write_bytes(data+b'changed');reject(lambda:verify_ota_evidence(root,sha,tree));path.write_bytes(data)
print(f'PASS: {probes} wifitest77 evidence refusal probes plus all109 inherited probes. All report fixtures are synthetic validator tests only.')
