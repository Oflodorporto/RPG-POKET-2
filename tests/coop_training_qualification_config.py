#!/usr/bin/env python3
"""Parse CI and reject trigger/permission/export/artifact scope regressions."""
from pathlib import Path
import copy
import subprocess
import sys
import yaml
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from record_coop_training_build import FQBN,BASE
DIAGNOSTIC_IF="always() && !cancelled() && (steps.compile.outcome == 'success' || steps.compile.outcome == 'failure')"
DIAGNOSTIC_UPLOAD_IF="always() && !cancelled() && steps.link_diagnostics.outcome == 'success'"

def validate(document):
    assert set(document)=={'name','on','permissions','concurrency','defaults','env','jobs'}
    assert document['on']=={'push':{'branches':['feat/coop-training-check']}}
    assert document['permissions']=={'contents':'read'}
    assert document['defaults']['run']['shell']=='bash --noprofile --norc -euo pipefail {0}'
    assert document['env']=={'PYTHONDONTWRITEBYTECODE':'1'}
    assert document['concurrency']=={'group':'coop-training-check-${{ github.ref }}','cancel-in-progress':'true'}
    assert set(document['jobs'])=={'focused-native','esp32-on-off'}
    for name,job in document['jobs'].items():
        assert set(job)==({'if','runs-on','timeout-minutes','steps'}|({'needs'} if name=='esp32-on-off' else set()))
        assert job['if']=="github.event_name == 'push' && github.ref == 'refs/heads/feat/coop-training-check'"
        assert job['runs-on']=='ubuntu-latest' and int(job['timeout-minutes'])<60
        checkouts=[s for s in job['steps'] if s.get('uses')=='actions/checkout@v4']
        assert len(checkouts)==1 and checkouts[0]['with']=={'fetch-depth':'0','persist-credentials':'false'}
        for step in job['steps']:
            assert set(step)<={'name','uses','with','run','id','if'}
            if 'id' in step:assert name=='esp32-on-off' and step['id'] in ('compile','link_diagnostics')
            if 'if' in step:
                assert name=='esp32-on-off'
                if step.get('id')=='link_diagnostics':assert step['if']==DIAGNOSTIC_IF
                else:assert step.get('uses')=='actions/upload-artifact@v4' and 'LINK-DIAGNOSTICS' in step['with']['name'] and step['if']==DIAGNOSTIC_UPLOAD_IF
            if 'uses' in step:assert step['uses'] in ('actions/checkout@v4','arduino/setup-arduino-cli@v2','actions/upload-artifact@v4')
        uploads=[s for s in job['steps'] if s.get('uses')=='actions/upload-artifact@v4']
        assert len(uploads)==(1 if name=='focused-native' else 2)
        for index,step in enumerate(uploads):
            upload=step['with']
            assert set(upload)=={'name','path','if-no-files-found','retention-days'}
            assert upload['if-no-files-found']=='error' and upload['retention-days']=='7'
            assert 'EXPERIMENTAL' in upload['name'] and '${{ github.sha }}' in upload['name']
            if index==1:
                assert upload['name']=='coop-training-EXPERIMENTAL-LINK-DIAGNOSTICS-${{ github.sha }}'
                assert step.get('if')==DIAGNOSTIC_UPLOAD_IF
                expected=['build/coop-training-link-diagnostics/link-diagnostics.txt','build/coop-training-link-diagnostics/diagnostic-identity.json']
            else:
                assert 'if' not in step
                expected=['build/coop-training-native/*.log'] if name=='focused-native' else ['build/coop-training-evidence/*.json','build/coop-training-evidence/*.log','build/coop-training-evidence/*.txt']
            assert upload['path'].splitlines()==expected
    native='\n'.join(s.get('run','') for s in document['jobs']['focused-native']['steps'])
    for command in ['python3 tests/coop_training_qualification_config.py','python3 tests/coop_training_build_evidence.py','python3 tests/coop_training_link_diagnostics.py','python3 tools/check_coop_training_scope.py --ci','bash tools/test_coop_training.sh build/coop-training-native']:assert command in native
    build_job=document['jobs']['esp32-on-off'];assert build_job['needs']=='focused-native'
    build='\n'.join(s.get('run','') for s in build_job['steps'])
    for command in ['core install esp32:esp32@3.3.12',"--fqbn '"+FQBN+"'",'for flag in 0 1; do','arduino-cli compile --verbose --jobs 4',
        '--build-property "compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=$flag"',
        '--build-path "$RUNNER_TEMP/coop-training-build/flag-$flag"','--export-binaries=false firmware/RPG_POKET_2',
        'test "$(git rev-parse HEAD)" = "$GITHUB_SHA"','python3 tools/record_coop_training_build.py',
        'test -z "$(git status --porcelain --untracked-files=all)"']:assert command in build,command
    assert build.count('python3 tools/check_coop_training_scope.py --ci')==2
    steps=build_job['steps'];compile_steps=[s for s in steps if s.get('id')=='compile'];diagnostics=[s for s in steps if s.get('id')=='link_diagnostics']
    assert len(compile_steps)==len(diagnostics)==1
    assert steps.index(compile_steps[0])<steps.index(diagnostics[0])
    assert diagnostics[0].get('if')==DIAGNOSTIC_IF
    assert diagnostics[0]['run'].replace('\\\n',' ').split()==['python3','tools/diagnose_coop_link.py','--build-root','"$RUNNER_TEMP/coop-training-build"','--compile-logs','build/coop-training-evidence','--out-dir','build/coop-training-link-diagnostics']
    all_runs=native+'\n'+build
    for forbidden in ['gh ', 'git push','release/', 'secrets.', 'github.token','curl ', '--output-dir', '--export-binaries=true','build.extra_flags=', 'test_package10.sh','test_package11.sh','upload-port','arduino-cli upload','|| true','set +e','-mtext-section-literals','-mauto-litpools']:
        assert forbidden not in all_runs,forbidden

document=yaml.load((ROOT/'.github/workflows/coop-training-check.yml').read_text(),Loader=yaml.BaseLoader)
validate(document)
checks=1
def reject(mutator):
    global checks
    bad=copy.deepcopy(document);mutator(bad)
    try:validate(bad)
    except (AssertionError,KeyError,TypeError,ValueError):checks+=1
    else:raise AssertionError('Accepted unsafe CI mutation')
reject(lambda d:d.update(on={'push':{'branches':['main']}}))
reject(lambda d:d['on'].update(workflow_dispatch={}))
reject(lambda d:d.update(permissions={'contents':'write'}))
reject(lambda d:d['jobs']['esp32-on-off'].update(needs=''))
reject(lambda d:d['jobs']['esp32-on-off'].update(**{'if':'always()'}))
reject(lambda d:d['jobs']['focused-native']['steps'][0]['with'].update({'persist-credentials':'true'}))
reject(lambda d:d['jobs']['esp32-on-off']['steps'][-1]['with'].update(path='build/**'))
reject(lambda d:d['jobs']['esp32-on-off']['steps'][-1]['with'].update(path='${{ runner.temp }}/**'))
reject(lambda d:d['jobs']['esp32-on-off']['steps'][-1]['with'].update(name='firmware-${{ github.sha }}'))
reject(lambda d:d['jobs']['esp32-on-off']['steps'][-1].update({'if':'always()'}))
reject(lambda d:next(s for s in d['jobs']['esp32-on-off']['steps'] if s.get('id')=='compile').update({'continue-on-error':'true'}))
reject(lambda d:next(s for s in d['jobs']['esp32-on-off']['steps'] if s.get('id')=='link_diagnostics').update({'if':'always()'}))
reject(lambda d:next(s for s in d['jobs']['esp32-on-off']['steps'] if s.get('id')=='link_diagnostics').update(run='env'))
for old,new in [('esp32@3.3.12','esp32@3.3.11'),('--export-binaries=false','--export-binaries=true'),('compiler.cpp.extra_flags','build.extra_flags'),('$RUNNER_TEMP/coop-training-build/flag-$flag','firmware/RPG_POKET_2/build'),('for flag in 0 1','for flag in 1'),(' --ci','')]:
    def mutate(d,old=old,new=new):
        for step in d['jobs']['esp32-on-off']['steps']:
            if 'run' in step:step['run']=step['run'].replace(old,new)
    reject(mutate)
reject(lambda d:d['jobs']['esp32-on-off']['steps'].append({'run':'gh release create unsafe'}))
# Historical publishers and package qualification files remain byte-identical.
for path in (ROOT/'.github/workflows').glob('*.yml'):
    if path.name=='coop-training-check.yml':continue
    relative=str(path.relative_to(ROOT))
    assert path.read_bytes()==subprocess.check_output(['git','show',BASE+':'+relative],cwd=ROOT)
    checks+=1
print(f'PASS: {checks} CI configuration/negative/frozen-workflow checks; no remote action performed')
