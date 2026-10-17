#!/usr/bin/env python3
"""Record exact CI source identity only after aggregate, target build and byte gates."""
if not __debug__:
    raise RuntimeError("Qualification requires assertions; optimized Python is forbidden")
from pathlib import Path
import argparse
import functools
import operator
import re
import shlex
import struct
import hashlib
import json
import os
import subprocess
from verify_wifitest77_release import verify
from test_wifitest77_release import published_lineage
from check_wifitest77_scope import assert_generated_inventory
from record_coop_training_build import (parse_memory,board_options,parse_symbols,inspect_shared_linkage,inspect_updater_linkage,compare_budgets,find_tool,digest,FQBN,OTA_BYTES,UPDATER_STATE)


def git(*args):
    return subprocess.check_output(['git',*args],text=True).strip()


def assert_generated_only(status):
    allowed = {'release/firmware.bin','release/artes.pak','release/manifest.json','release/SHA256SUMS'}
    assert all(line.startswith('?? ') and line[3:] in allowed for line in status.splitlines()), 'Unexpected source change: '+status


VARIANTS = {
    'coop1-radio1': (1,1,None),
    'coop1-radio0': (1,0,'-DRPG_ENABLE_WIFI_RADIO_TEST=0'),
    'coop0-radio1': (0,1,'-DRPG_ENABLE_COOP_TRAINING=0'),
}

def check_options(options,variant):
    assert variant in VARIANTS, 'Unapproved target variant'
    coop,radio,override=VARIANTS[variant]
    assert board_options(options['fqbn'])==board_options(FQBN),'Unexpected FQBN'
    raw=options['customBuildProperties'];assert isinstance(raw,str)
    pairs=[item.split('=',1) for item in raw.split(',') if item]
    assert all(len(p)==2 for p in pairs) and len(dict(pairs))==len(pairs),'Malformed/duplicate properties'
    props=dict(pairs)
    if override is None:
        assert 'compiler.cpp.extra_flags' not in props,'Default build must use Arduino defaults without an ON override'
    else:
        assert props.pop('compiler.cpp.extra_flags',None)==override,'Exact single forced-OFF flag required'
    assert set(props)<={'build.warn_data_percentage'},'Unexpected build/compiler/linker custom property'
    if 'build.warn_data_percentage' in props:assert props['build.warn_data_percentage']=='75'
    rest=dict(options);rest.pop('customBuildProperties')
    for define in ('RPG_ENABLE_COOP_TRAINING','RPG_ENABLE_WIFI_RADIO_TEST'):
        assert define not in json.dumps([rest,props]),'Define outside approved property'
    return dict(fqbn=options['fqbn'],coop_enabled=coop,radio_enabled=radio,
                default_arduino_enabled=override is None,explicit_override=override,
                options_sha256=hashlib.sha256(json.dumps(options,sort_keys=True).encode()).hexdigest())

def check_image(data):
    assert 24<=len(data)<=OTA_BYTES and data[0]==0xe9 and 1<=data[1]<=16
    assert struct.unpack_from('<H',data,12)[0]==9 and data[23]==1,'Expected ESP32-S3 image with digest'
    pos=24;checksum=0xef
    for _ in range(data[1]):
        assert pos+8<=len(data);length=struct.unpack_from('<I',data,pos+4)[0];pos+=8
        assert pos+length<=len(data);checksum=functools.reduce(operator.xor,data[pos:pos+length],checksum);pos+=length
    footer=pos|15
    assert footer+33==len(data) and not any(data[pos:footer]) and data[footer]==checksum,'Invalid ESP image envelope'
    assert hashlib.sha256(data[:footer+1]).digest()==data[footer+1:],'Invalid appended SHA256'
    assert b'2026.10.17-wifitest77\0' in data,'Wrong compiled release identity'
    return dict(chip_id=9,segment_count=data[1],segment_checksum_verified=True,appended_sha256_verified=True,wifitest77_identity_present=True)

def check_compile_log(log,variant):
    assert variant in VARIANTS
    override=VARIANTS[variant][2]
    units={}
    for source in ('RPG_POKET_2.ino.cpp','CoopRuntime.cpp','UpdateInstall.cpp'):
        lines=[]
        for line in log.splitlines():
            if 'xtensa-esp32s3-elf-g++' not in line:continue
            parsed=shlex.split(line)
            if '-E' not in parsed and any(a.endswith('/'+source) for a in parsed) and any(a.endswith('/'+source+'.o') for a in parsed):lines.append(line)
        assert len(lines)==1,('Missing/ambiguous actual compilation',source,len(lines))
        args=shlex.split(lines[0]);assert any(a.startswith('-DARDUINO=') for a in args) and '-DARDUINO_ARCH_ESP32' in args
        assert '/esp-x32/2601/' in args[0],'Unexpected target compiler version'
        definitions=[a for a in args if 'RPG_ENABLE_COOP_TRAINING' in a or 'RPG_ENABLE_WIFI_RADIO_TEST' in a]
        assert definitions==([] if override is None else [override]),('Unexpected effective training flags',source,definitions)
        assert '-Os' in args and '-c' in args,'Historical optimization/compile mode changed'
        response={}
        for arg in args:
            if arg.startswith('@') and '/3.3.12/flags/' in arg:
                path=Path(arg[1:]);assert path.is_file();response[path.name]=digest(path)
        assert 'cpp_flags' in response,'Missing official core C++ flags'
        units[source]={'command':lines[0],'core_response_flags':response}
    return units

DIAGNOSTIC_OBJECT_LIMITS = {'coopradio::diagnostic':320, 'coopradio::signals':128,
                            'coopradio::telemetry':96, '(anonymous namespace)::displaySnapshot':320}
def inspect_diagnostics(symbols,flag):
    found={name:[] for name in DIAGNOSTIC_OBJECT_LIMITS}
    for line in symbols.splitlines():
        m=re.match(r'^\s*[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+\S\s+(.+)$',line)
        if m and m[2] in found:found[m[2]].append(int(m[1],16))
    if not flag:
        assert not any(found.values()), 'Diagnostic state emitted by forced-OFF image'
        return {}
    for name,limit in DIAGNOSTIC_OBJECT_LIMITS.items():
        assert len(found[name])==1 and 0<found[name][0]<=limit, ('Diagnostic singleton/budget',name,found[name])
    sizes={name:values[0] for name,values in found.items()}
    assert sum(sizes.values())<=864, 'Diagnostic scalar/cache state exceeded bounded allocation'
    return sizes

def verify_radio_evidence(root=Path('.')):
    strict={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1:strict_string_checks=1',
            'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1','LSAN_OPTIONS':'exitcode=23'}
    result={}
    def source_hashes(report,keys):
        for key in keys:
            assert isinstance(report[key],dict) and report[key], ('Missing source binding',key)
            for name,wanted in report[key].items():
                assert not Path(name).is_absolute() and '..' not in Path(name).parts
                assert hashlib.sha256((root/name).read_bytes()).hexdigest()==wanted, 'Diagnostic source changed: '+name
    for name in ('coop-radio-recovery','coop-radio-device','coop-radio-runtime-ui'):
        report=json.loads((root/'build'/name/'results.json').read_text())
        assert report['passed'] is True and report['mode']=='both' and report['feature_enabled']==1
        assert len(report['cases'])==2 and {c['mode'] for c in report['cases']}=={'normal','sanitizers'}
        for case in report['cases']:
            assert case['passed'] is True
            if case['mode']=='sanitizers':assert case['sanitizer_environment']==strict
        if 'source_root' in report:assert Path(report['source_root']).resolve()==root.resolve()
        source_hashes(report,['sources_sha256'])
        if 'regression_sha256' in report:source_hashes(report,['regression_sha256'])
        result[name]=report
    folder=root/'build/coop-radio-render';render=json.loads((folder/'manifest.json').read_text())
    assert render['passed'] is True and render['mode']=='both' and render['feature_enabled']==1
    assert render['sanitized'] is True and render['pixel_parity'] is True and render['sanitizer_environment']==strict
    source_hashes(render,['source_sha256'])
    expected={'discovery-no-sample','discovery-strong','discovery-weak','discovery-stale','accept-with-signal','playing-with-signal',
              'recovery-dots-1','recovery-dots-2','recovery-dots-3','recovery-no-art','diagnostics-live','diagnostics-no-sample',
              'diagnostics-extremes','terminal-5','terminal-8','terminal-1','terminal-2','terminal-4'}
    assert len(render['captures'])==18 and {c['name'] for c in render['captures']}==expected
    for directory in ('normal','sanitized'):
        assert {p.stem for p in (folder/directory).glob('*.ppm')}==expected
        assert {p.name for p in (folder/directory).glob('*-labels.tsv')}=={n+'-labels.tsv' for n in expected}
    for capture in render['captures']:
        name=capture['name']
        for suffix in ('.ppm','-labels.tsv'):
            assert (folder/'normal'/(name+suffix)).read_bytes()==(folder/'sanitized'/(name+suffix)).read_bytes(), 'Diagnostic pixel/label parity changed'
        assert hashlib.sha256((folder/'normal'/(name+'.ppm')).read_bytes()).hexdigest()==capture['ppm_sha256']
        assert hashlib.sha256((folder/(name+'.png')).read_bytes()).hexdigest()==capture['png_sha256']
    result['render']=render
    return result

RADIO_OBJECT_LIMITS = {
    'wifi_radio_ui::device':8192,
    'wifi_radio_ui::screen':2048,
    'wifi_radio_test::device_detail::mailbox':2048,
    'wifi_radio_test::device_detail::nextGeneration':4,
}

def inspect_wifi_symbols(symbols,radio):
    found={name:[] for name in RADIO_OBJECT_LIMITS}
    for line in symbols.splitlines():
        m=re.match(r'^\s*[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+\S\s+(.+)$',line)
        if m and m[2] in found:found[m[2]].append(int(m[1],16))
    if not radio:
        assert not any(found.values()),'Radio state emitted by RADIO0 image'
        return {}
    for name,limit in RADIO_OBJECT_LIMITS.items():
        assert len(found[name])==1 and 0<found[name][0]<=limit,('Wi-Fi radio singleton/budget',name,found[name])
    return {name:values[0] for name,values in found.items()}

def inspect_training_symbols(symbols,coop):
    # The isolated radio also uses the existing shared owner. Its presence in
    # COOP0/RADIO1 is intentional; no other co-op state receives an exemption.
    if not coop:
        symbols='\n'.join(line for line in symbols.splitlines() if not re.search(r'\scoopradio::owner$',line))
    return parse_symbols(symbols,coop)

def record_target_builds(build_root,tools_dir):
    root=Path('.').resolve();build=build_root.resolve();assert not build.is_relative_to(root),'Build outputs must be external'
    toolchain=Path('build/wifitest77-esp32-toolchain.txt').read_text()
    assert re.search(r'^esp32:esp32\s+3\.3\.12(?:\s|$)',toolchain,re.M) and 'arduino-cli' in toolchain.lower()
    size=find_tool(tools_dir,'size');nm=find_tool(tools_dir,'nm');builds={}
    for variant,(coop,radio,override) in VARIANTS.items():
        folder=build/variant;elf=folder/'RPG_POKET_2.ino.elf';binary=folder/'RPG_POKET_2.ino.bin'
        assert elf.is_file() and binary.is_file() and not elf.is_symlink() and not binary.is_symlink()
        effective=check_options(json.loads((folder/'build.options.json').read_text()),variant)
        logpath=Path(f'build/wifitest77-esp32-{variant}.log');log=logpath.read_text();memory=parse_memory(log)
        commands=check_compile_log(log,variant)
        sections=subprocess.check_output([str(size),'-A',str(elf)],text=True)
        symbols=subprocess.check_output([str(nm),'-S','-C','--defined-only',str(elf)],text=True)
        objects=inspect_training_symbols(symbols,coop);diagnostic_objects=inspect_diagnostics(symbols,coop)
        wifi_objects=inspect_wifi_symbols(symbols,radio)
        shared=inspect_shared_linkage(symbols,coop);updater=inspect_updater_linkage(symbols)
        assert len(shared['shared_mutable']['coopradio::owner'])==1,'Radio ownership must have exactly one shared instance'
        units={}
        for name in ('RPG_POKET_2.ino.cpp.o','CoopRuntime.cpp.o','UpdateInstall.cpp.o'):
            obj=folder/'sketch'/name;assert obj.is_file() and not obj.is_symlink()
            osyms=subprocess.check_output([str(nm),'-S','-C','--defined-only',str(obj)],text=True)
            if name=='CoopRuntime.cpp.o':
                if not coop:assert not osyms.strip(),'COOP0 runtime has definitions'
                assert not re.search(r' (?:assetMeta|artFallback|font|artMemory)$|enemyArt::workspace|ruins7Art::',osyms,re.M),'Runtime duplicates art storage'
                assert not any(re.search(r'\s'+re.escape(state)+r'$',osyms,re.M) for state in RADIO_OBJECT_LIMITS),'Radio state duplicated in co-op TU'
            if name=='UpdateInstall.cpp.o':
                inspect_updater_linkage(osyms)
                assert not re.search(r' (?:assetMeta|artFallback|font|artMemory|game|menu|journal)$|enemyArt::workspace|ruins7Art::|coopSession|coopDevice|coopInput|wifi_radio_ui::|wifi_radio_test::',osyms,re.M)
            else:
                assert not any(re.search(r'\s'+re.escape(state)+r'$',osyms,re.M) for state in UPDATER_STATE)
                assert not re.search(r'\sinstallUpdate\(bool\)$',osyms,re.M)
            units[name]={**digest(obj),'defined_symbol_lines':len(osyms.splitlines())}
            sections+='\nTRANSLATION UNIT '+name+'\n'+subprocess.check_output([str(size),'-A',str(obj)],text=True)
        Path(f'build/wifitest77-esp32-sections-{variant}.txt').write_text(sections)
        Path(f'build/wifitest77-esp32-symbols-{variant}.txt').write_text('\n'.join(line for line in symbols.splitlines() if any(t in line for t in (*UPDATER_STATE,'installUpdate(bool)','coopSession','coopInput','coopDevice','coopradio::','wifi_radio','displaySnapshot','artMemory','enemyArt::workspace','assetMeta','artFallback','raceAsset','raceMask','enemyArt::enemies','enemyScale::profiles',' font','outfitColors','coopview::kHeroCrop','ruins7Art::'))) +'\n')
        builds[variant]={'coop_enabled':coop,'radio_enabled':radio,'effective_options':effective,'effective_compile_commands':commands,'image':digest(binary),'image_format':check_image(binary.read_bytes()),'memory':memory,'elf':digest(elf),'build_log':digest(logpath),'target_object_bytes':objects,'diagnostic_object_bytes':diagnostic_objects,'wifi_radio_object_bytes':wifi_objects,'translation_units':units,'linkage_audit':shared,'updater_linkage_audit':updater}
    for variant in VARIANTS:
        for source in builds[variant]['effective_compile_commands']:
            assert builds[variant]['effective_compile_commands'][source]['core_response_flags']==builds['coop1-radio1']['effective_compile_commands'][source]['core_response_flags'],'Core flags changed between variants'
    result={'schema':1,'source_commit':git('rev-parse','HEAD'),'source_tree':git('rev-parse','HEAD^{tree}'),'builds':builds,
            'compiled_budget':{'coop':compare_budgets(builds['coop0-radio1'],builds['coop1-radio1']),
                               'radio':compare_budgets(builds['coop1-radio0'],builds['coop1-radio1'])},
            'core':'3.3.12','compiler':'esp-x32/2601','fqbn':FQBN,'default_arduino_enabled':True,'physical_validation':False,'free_heap_measured':False}
    Path('build/wifitest77-esp32-build-evidence.json').write_text(json.dumps(result,indent=2)+'\n')
    return result

RADIO_SUITES=('model','flow','device','races','export','runtime','render-on','render-off')
RADIO_TEST_SOURCES={name:'tests/'+source for name,source in {
 'model':'wifi_radio_test_model.cpp','flow':'wifi_radio_test_flow.cpp',
 'device':'wifi_radio_device.cpp','races':'wifi_radio_device_races.cpp',
 'export':'wifi_radio_test_export.cpp','runtime':'wifi_radio_test_runtime.cpp',
 'render-on':'wifi_radio_test_render.cpp','render-off':'wifi_radio_test_render.cpp'}.items()}
RADIO_CAPTURES={
 'render-on':{'baseline-not-coordinated.ppm','coordinating.ppm','details-1.ppm','details-2.ppm','details-3.ppm','details-4.ppm',
              'discovery-timeout.ppm','distance-two-palms.ppm','internet-test-preserved.ppm','lobby.ppm','manual-levels.ppm',
              'manual-one-meter.ppm','missing-details-1.ppm','missing-details-2.ppm','missing-details-3.ppm',
              'report-complete.ppm','report-missing-peer.ppm','report-recovery.ppm','restore-error.ppm','running.ppm','wifi-saved-entry.ppm','wifi-browse-entry.ppm'},
 'render-off':{'internet-test-radio-off.ppm'},
}
OTA_GATE_SOURCES=('tools/test_wifitest77_release.py','tests/wifitest77_release.cpp','tools/verify_wifitest77_release.py',
                  'tools/test_coop_update_install.py','tests/coop_update_install_equivalence.py')

def verify_source_hashes(root,mapping):
    assert isinstance(mapping,dict) and mapping, 'Missing source hashes'
    for name,expected in mapping.items():
        assert isinstance(name,str) and not Path(name).is_absolute() and '..' not in Path(name).parts
        path=root/name
        assert path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(root.resolve())
        assert isinstance(expected,str) and re.fullmatch('[0-9a-f]{64}',expected)
        assert hashlib.sha256(path.read_bytes()).hexdigest()==expected, 'Evidence source changed: '+name

def verify_wifi_suite_evidence(root=Path('.')):
    result={}
    strict={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1','LSAN_OPTIONS':'exitcode=23'}
    for suite in RADIO_SUITES:
        result[suite]={}
        for mode in ('normal','sanitizers'):
            folder=root/'build/wifi-radio-test'/suite/mode
            report=json.loads((folder/'results.json').read_text())
            assert report['suite']==suite and report['passed'] is True and report['sanitizer'] is (mode=='sanitizers')
            assert all(type(report[key]) is int and report[key]==0 for key in ('compile_exit_code','run_exit_code'))
            assert report['source_error'] is None and report['capture_error'] is None and report['sources_changed_during_execution']==[]
            verify_source_hashes(root,report['sources_sha256'])
            assert {'tools/test_wifi_radio_test.py',RADIO_TEST_SOURCES[suite]}<=set(report['sources_sha256'])
            assert hashlib.sha256(json.dumps(report['sources_sha256'],sort_keys=True,separators=(',',':')).encode()).hexdigest()==report['source_sha256']
            assert report['sanitizer_environment']==(strict if mode=='sanitizers' else {})
            assert set(report['logs'])=={'compile','run'}
            for kind,record in report['logs'].items():
                assert set(record)=={'path','sha256'}
                path=Path(record['path']);path=path if path.is_absolute() else root/path
                assert path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(folder.resolve())
                assert hashlib.sha256(path.read_bytes()).hexdigest()==record['sha256'], 'Radio log changed: '+str(path)
                if kind=='compile':
                    log=path.read_text();lines=[line[6:] for line in log.splitlines() if line.startswith('argv: ')]
                    assert len(lines)==1
                    argv=json.loads(lines[0]);assert isinstance(argv,list) and all(isinstance(arg,str) for arg in argv)
                    assert argv.count('-UNDEBUG')==1 and not any(arg.startswith('-DNDEBUG') for arg in argv)
                    if mode=='sanitizers':
                        for flag in ('-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie','-O1'):assert flag in argv
                    else:assert '-O2' in argv and not any(arg.startswith('-fsanitize') for arg in argv)
            if suite in RADIO_CAPTURES:
                assert set(report['captures'])==RADIO_CAPTURES[suite], 'Missing or unexpected radio render fixture'
                for name,record in report['captures'].items():
                    assert set(record)=={'path','sha256'}
                    path=Path(record['path']);path=path if path.is_absolute() else root/path
                    assert path.name==name and path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(folder.resolve())
                    data=path.read_bytes();header=b'P6\n240 320\n255\n'
                    assert data.startswith(header) and len(data)==len(header)+240*320*3, 'Invalid target-sized render capture'
                    assert hashlib.sha256(data).hexdigest()==record['sha256']
            result[suite][mode]=report
        if suite in RADIO_CAPTURES:
            normal={name:record['sha256'] for name,record in result[suite]['normal']['captures'].items()}
            sanitized={name:record['sha256'] for name,record in result[suite]['sanitizers']['captures'].items()}
            assert normal==sanitized, 'Normal and sanitized Wi-Fi pixels differ'
    return result

def verify_ota_evidence(root=Path('.'),source_commit=None,source_tree=None):
    from test_wifitest77_release import HISTORY
    source_commit=source_commit or git('rev-parse','HEAD')
    source_tree=source_tree or git('rev-parse','HEAD^{tree}')
    expected_inputs={name:digest(root/'release'/name) for name in ('firmware.bin','artes.pak','manifest.json')}
    expected_clients={label:commit for label,commit,tree,define in HISTORY}
    expected_clients['wifitest77']=source_commit
    result={}
    for mode in ('normal','sanitizers'):
        report=json.loads((root/'build/wifitest77-ota'/('release-'+mode+'.json')).read_text())
        assert report['schema']==1 and type(report['schema']) is int
        assert report['passed'] is True and report['synthetic'] is False and report['mode']==mode
        assert report['source_commit']==source_commit and report['source_tree']==source_tree
        assert report['inputs']==expected_inputs and report['historical_clients']==13 and type(report['historical_clients']) is int
        assert all(type(record['bytes']) is int for record in report['inputs'].values())
        assert set(report['sources_sha256'])==set(OTA_GATE_SOURCES)
        verify_source_hashes(root,report['sources_sha256'])
        assert set(report['clients'])==set(expected_clients)
        for label,commit in expected_clients.items():
            client=report['clients'][label]
            assert client['source_commit']==commit
            assert client['split_installer'] is (label in ('coop77','coopfix77','coopdiag77','wifitest77'))
            headers=client['headers_sha256'];assert isinstance(headers,dict) and headers
            for name,sha in headers.items():
                assert isinstance(name,str) and not Path(name).is_absolute() and '..' not in Path(name).parts
                assert isinstance(sha,str) and re.fullmatch('[0-9a-f]{64}',sha)
            assert hashlib.sha256(json.dumps(headers,sort_keys=True).encode()).hexdigest()==client['closure_sha256']
        for field in ('save29_to30','save30_to30','all_three_slots','real_sha256','historical_headers_unchanged','no_load_or_rollback_writes','no_new_reset'):
            assert report[field] is True, 'Missing actual-byte OTA proof: '+field
        assert report['physical_validation'] is False and report['published'] is False
        expected_env={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1','LSAN_OPTIONS':'exitcode=23','RPG_DETECT_LEAKS':'1'} if mode=='sanitizers' else {}
        assert report['sanitizer_environment']==expected_env
        result[mode]=report
    return result

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--build-root',type=Path,required=True)
    parser.add_argument('--tools-dir',type=Path,default=Path.home()/'.arduino15/packages/esp32/tools')
    args=parser.parse_args()
    assert git('rev-parse','HEAD') == os.environ['GITHUB_SHA']
    assert os.environ['GITHUB_REPOSITORY'] == 'Oflodorporto/RPG-POKET-2'
    assert os.environ['GITHUB_WORKFLOW_SHA'] == os.environ['GITHUB_SHA']
    workflow = 'release.yml' if os.environ['GITHUB_REF'] == 'refs/heads/main' else 'wifitest77-check.yml'
    assert os.environ['GITHUB_WORKFLOW_REF'] == os.environ['GITHUB_REPOSITORY']+'/.github/workflows/'+workflow+'@'+os.environ['GITHUB_REF']
    assert os.environ['GITHUB_REF'] in ('refs/heads/feat/wifitest77-release','refs/heads/main')
    assert os.environ['GITHUB_EVENT_NAME'] == 'push' and os.environ['RPG_DETECT_LEAKS'] == '1'
    assert_generated_inventory(Path('.'), git('ls-files','--cached','-z').split('\0'))
    assert_generated_only(git('status','--porcelain','--untracked-files=all'))
    verify()
    lineage = published_lineage()
    summary=json.loads(Path('build/package12-validation/ci-'+os.environ['GITHUB_SHA']+'/summary.json').read_text())
    assert summary['all_passed'] and not summary['failed'] and summary['lsan'] == 'detect_leaks=1' and summary['tasks'] == 165
    aggregate_root=Path('build/package12-validation/ci-'+os.environ['GITHUB_SHA'])
    source_manifest=json.loads((aggregate_root/'source-manifest.json').read_text())
    assert source_manifest['sha256'] == summary['source_sha256']
    manifest_body={key:value for key,value in source_manifest.items() if key != 'sha256'}
    assert hashlib.sha256(json.dumps(manifest_body,sort_keys=True).encode()).hexdigest() == source_manifest['sha256']
    assert set(source_manifest['files']) == set(git('ls-files','--cached','-z').split('\0')) - {''}
    for name,digest in source_manifest['files'].items():
        assert hashlib.sha256(Path(name).read_bytes()).hexdigest() == digest, 'Aggregate input changed: '+name
    checkpoints=json.loads((aggregate_root/'checkpoints.json').read_text())
    assert len(checkpoints) == 165
    for name,entry in checkpoints.items():
        assert entry['status'] == 'passed' and hashlib.sha256((aggregate_root/(name+'.log')).read_bytes()).hexdigest() == entry['log_sha256'], 'Aggregate log changed: '+name
    from test_wifitest77_local import tasks as expected_tasks
    plan = json.loads((aggregate_root/'plan.json').read_text())
    assert plan['focused'] is False and plan['source_sha256'] == source_manifest['sha256']
    assert plan['tasks'] == json.loads(json.dumps(expected_tasks(False))), 'Aggregate command plan changed'
    assert set(checkpoints) == {'preparation'} | {name for name, _ in expected_tasks(False)}
    for name, commands in expected_tasks(False):
        assert checkpoints[name]['commands'] == commands, 'Recorded commands changed: '+name
    hotfix = {}
    strict = {'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1:strict_string_checks=1',
              'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1','LSAN_OPTIONS':'exitcode=23'}
    for name in ('coop-touch-queue', 'coop-invite-countdown'):
        result = json.loads(Path('build', name, 'results.json').read_text())
        assert result['passed'] is True and result['mode'] == 'both'
        assert len(result['cases']) == 2 and {c['mode'] for c in result['cases']} == {'normal', 'sanitizers'}
        for case in result['cases']:
            assert case['passed'] is True
            if case['mode'] == 'sanitizers': assert case['sanitizer_environment'] == strict
        if name == 'coop-invite-countdown':
            assert result['baseline'] is False and Path(result['source_root']).resolve() == Path('.').resolve()
            for path, expected in result['regression_sha256'].items():
                assert hashlib.sha256(Path(path).read_bytes()).hexdigest() == expected
        for path, expected in result['sources_sha256'].items():
            assert hashlib.sha256(Path(path).read_bytes()).hexdigest() == expected, 'Hotfix source changed: '+path
        hotfix[name] = result
    names=['firmware.bin','artes.pak','manifest.json']
    files={n:dict(bytes=(Path('release')/n).stat().st_size,sha256=hashlib.sha256((Path('release')/n).read_bytes()).hexdigest()) for n in names}
    narrative=json.loads(Path('build/narrative8-render/summary.json').read_text())
    assert narrative['rendered_frames'] == 3481 and narrative['unique_previews'] == 671
    clock_visibility=json.loads(Path('build/clock-cycle7/visibility-preview/capture-manifest.json').read_text())
    assert clock_visibility['Relogio-pulso-manha-velocidade-real-12s.gif']['duration_ms'] == 12000
    pvp=json.loads(Path('build/pvp-presentation/capture-manifest.json').read_text())
    assert sum(name.endswith('.gif') for name in pvp) == 12
    for directory in ('screens','sanitized'):
        report_text=Path('build/pvp-presentation',directory,'render-results.txt').read_text()
        assert '15030 total renders' in report_text and '2048 independent identity matrix cases' in report_text and '5376 action phase cases' in report_text
    assert len(list(Path('build/pvp-presentation/screens').glob('*.ppm'))) == 596
    assert len(list(Path('build/pvp-presentation/sanitized').glob('*.ppm'))) == 596
    bulk_capture=Path('build/bulk-shop/screens/compras-vendas-em-lote.png')
    assert bulk_capture.is_file() and len(list(Path('build/bulk-shop/screens').glob('*.ppm'))) == 14
    bulk_frames={path.name:hashlib.sha256(path.read_bytes()).hexdigest() for path in Path('build/bulk-shop/screens').glob('*.ppm')}
    sanitized_frames={path.name:hashlib.sha256(path.read_bytes()).hexdigest() for path in Path('build/bulk-shop/sanitized').glob('*.ppm')}
    assert bulk_frames == sanitized_frames, 'Optimized and sanitizer commerce pixels differ'
    bulk={'frames_sha256':bulk_frames,'optimized_sanitized_identical':True,'preview_sha256':hashlib.sha256(bulk_capture.read_bytes()).hexdigest(),'frames':14,'ration_limit':10,'historical_reader_limit':9,'firmware_downgrade_supported':False}
    target=record_target_builds(args.build_root,args.tools_dir)
    assert target['builds']['coop1-radio1']['image']==files['firmware.bin'],'Published candidate must be the default-ON image'
    coop_native=json.loads(Path('build/coop-training-native/results.json').read_text())
    coop_render=json.loads(Path('build/coop-training-render/manifest.json').read_text())
    installer=json.loads(Path('build/coop-training-native/update-install-results.json').read_text())
    expected_native={(name,'normal') for name in ('session','radio','input','device','pvp_exclusion','default_off','controller','club_protocol','club_duel','pvp_reliability_save30')}|{(name,'sanitizers') for name in ('session','radio','input','device','pvp_exclusion','controller')}
    assert coop_native['passed'] is True and coop_native['mode']=='both' and coop_native['modes']==['normal','sanitizers'] and coop_native['sanitized'] is True
    assert len(coop_native['cases'])==16 and {(c['name'],c['mode']) for c in coop_native['cases']}==expected_native and all(c['passed'] for c in coop_native['cases'])
    assert coop_native['sanitizer_environment']=={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1:strict_string_checks=1','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1','LSAN_OPTIONS':'exitcode=23'}
    for name,digest_value in coop_native['sources_sha256'].items():assert hashlib.sha256(Path(name).read_bytes()).hexdigest()==digest_value,'Native gate source changed'
    expected_installer={(name,mode) for name in ('coop_update_install_linkage','update_adapter','ota_recovery','ota_art_readback','ota_sd_diagnostics','enemy_ota') for mode in ('normal','sanitizers')}
    assert installer['mode']=='both' and len(installer['cases'])==12 and {(c['name'],c['mode']) for c in installer['cases']}==expected_installer and all(c['passed'] for c in installer['cases'])
    assert coop_render['passed'] is True and coop_render['sanitized'] is True and coop_render['mode']=='both' and coop_render['pixel_parity_verified'] is True
    assert len(coop_render['captures'])==58
    assert coop_render['feature_enabled']==1 and coop_render['compile_definitions']==['-DRPG_ENABLE_COOP_TRAINING=1']
    assert 'detect_leaks=1' in coop_render['sanitizer_environment']['ASAN_OPTIONS'] and coop_render['sanitizer_environment']['LSAN_OPTIONS']=='exitcode=23'
    normal_coop={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in Path('build/coop-training-render/screens').glob('*.ppm')}
    sanitized_coop={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in Path('build/coop-training-render/sanitized').glob('*.ppm')}
    assert len(normal_coop)==58 and normal_coop==sanitized_coop,'Coop rendered frames differ'
    from check_coopfix77_invite_render import verify as verify_invites
    invites=json.loads(Path('build/coop-training-render/invites/manifest.json').read_text())
    assert invites['passed'] is True and invites['sanitized'] is True and invites['pixel_and_label_parity'] is True
    actual_invites=verify_invites(Path('build/coop-training-render'),True)
    assert set(invites['captures'])==set(actual_invites) and len(actual_invites)==16
    for name,data in actual_invites.items():
        assert all(invites['captures'][name][key]==value for key,value in data.items())
        assert hashlib.sha256(Path('build/coop-training-render/invites',name+'.png').read_bytes()).hexdigest()==invites['captures'][name]['png_sha256']
    for name,expected in invites['sources_sha256'].items():assert hashlib.sha256(Path(name).read_bytes()).hexdigest()==expected
    assert hashlib.sha256(Path('build/coop-training-render/invites/contactsheet.png').read_bytes()).hexdigest()==invites['contactsheet_sha256']
    radio=verify_radio_evidence()
    wifi_suites=verify_wifi_suite_evidence()
    actual_byte_ota=verify_ota_evidence()
    report=dict(wifi_suites=wifi_suites,actual_byte_ota=actual_byte_ota,workflow_ref=os.environ['GITHUB_WORKFLOW_REF'],workflow_sha=os.environ['GITHUB_WORKFLOW_SHA'],run_id=os.environ['GITHUB_RUN_ID'],run_attempt=os.environ['GITHUB_RUN_ATTEMPT'],radio_diagnostics=radio,invitation_render=invites,coop_admission_protocol_version=2,coop_accept_timeout_ms=30000,coop_technical_timeout_ms=12000,hotfix_regressions=hotfix,target_builds=target,coop_native=coop_native,coop_render=coop_render,installer_multitu=installer,experimental_training=True,bulk=bulk,pvp=pvp,pvp_protocol_version=5,pvp_packet_bytes=52,current_save30_journal=True,clock_visibility=clock_visibility,narrative=narrative,source_commit=git('rev-parse','HEAD'),source_tree=git('rev-parse','HEAD^{tree}'),branch=os.environ['GITHUB_REF'],artifacts=files,esp32_core='3.3.12',leak_detection=True,aggregate=summary,published_baselines=lineage,save_writes=30,save_reads=[29,30],save_bytes=160,ration_limit=10,historical_reader_ration_limit=9,firmware_downgrade_supported=False,reset_epoch=1,new_reset=False,physical_validation=False,published=False)
    Path('build/wifitest77-source-identity.json').write_text(json.dumps(report,indent=2)+'\n')
    Path('release/SHA256SUMS').write_text(''.join(f"{files[n]['sha256']}  {n}\n" for n in names))
    p=Path('build/wifitest77-scope-budget.json');budget=json.loads(p.read_text());budget.update(candidate_firmware_bytes=files['firmware.bin']['bytes'],candidate_ota_margin_bytes=0x300000-files['firmware.bin']['bytes'],target_compile_verified=True);p.write_text(json.dumps(budget,indent=2)+'\n')
    assert_generated_inventory(Path('.'), git('ls-files','--cached','-z').split('\0'))
    assert_generated_only(git('status','--porcelain','--untracked-files=all'))


if __name__=='__main__':
    main()
