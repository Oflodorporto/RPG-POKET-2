#!/usr/bin/env python3
"""Offline positive/negative probes for reuse of the existing release workflows."""
if not __debug__:
    raise RuntimeError('optimized Python is forbidden; optimized Python is forbidden')
from pathlib import Path
import copy
import subprocess
import yaml

ROOT=Path(__file__).resolve().parents[1]
BASE='aecfabf32edba31fce567a32450503be3492ea66'
QUAL='.github/workflows/wifitest77-check.yml'
PUB='.github/workflows/release.yml'
FQBN='esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,EraseFlash=none'

def parse(raw):
    data=yaml.safe_load(raw)
    assert isinstance(data,dict)
    return data

def event(data):
    return data.get('on',data.get(True))

def runs(data):
    return '\n'.join(step.get('run','') for job in data['jobs'].values() for step in job['steps'])

def uses(data):
    return [step['uses'] for job in data['jobs'].values() for step in job['steps'] if 'uses' in step]

def validate(qual,pub,old_qual,old_pub):
    q,p=event(qual),event(pub)
    oq,op=event(old_qual),event(old_pub)
    assert set(q)==set(oq)==set(p)==set(op)=={'push'}, 'No extra trigger'
    assert q['push']['branches']==['feat/ponytail-release']
    assert p['push']['branches']==['main']
    assert q['push']['paths']==oq['push']['paths'] and p['push']['paths']==op['push']['paths'], 'No widened path trigger'
    assert qual['permissions']==old_qual['permissions']=={'contents':'read'}
    assert pub['permissions']==old_pub['permissions']=={'contents':'write','actions':'read'}
    assert uses(qual)==uses(old_qual) and uses(pub)==uses(old_pub), 'No new external action'
    assert len(qual['jobs'])==len(old_qual['jobs'])==len(pub['jobs'])==len(old_pub['jobs'])==1
    qjob=qual['jobs']['validate'];pjob=pub['jobs']['release']
    assert qjob['if']=="github.repository == 'Oflodorporto/RPG-POKET-2' && github.event_name == 'push' && github.ref == 'refs/heads/feat/ponytail-release'"
    assert pjob['if']==old_pub['jobs']['release']['if']
    assert qjob['defaults']==old_qual['jobs']['validate']['defaults'] and pjob['defaults']==old_pub['jobs']['release']['defaults']
    qr,pr=runs(qual),runs(pub)
    for script in ('tools/test_ponytail.sh','tools/check_ponytail_scope.py','tools/release_ponytail_manifest.py','tools/verify_ponytail_release.py','tests/ponytail_release_verifier.py','tools/test_ponytail_release.py','tools/record_ponytail_identity.py'):
        assert script in qr and script in pr,script
    assert FQBN in qr and FQBN in pr
    assert 'coop1-radio1 coop1-radio0 coop0-radio1' in qr and 'coop1-radio1 coop1-radio0 coop0-radio1' in pr
    assert '--export-binaries=false' in qr and '--export-binaries=false' in pr
    assert '--jobs 4' in qr and '--jobs 4' in pr
    assert 'git rev-parse HEAD' in qr and 'git rev-parse HEAD' in pr
    assert 'wifitest77-check.yml/runs?branch=feat%2Fponytail-release&head_sha=$GITHUB_SHA&status=success' in pr
    assert 'tools/check_ponytail_release_gate.py' in pr
    assert 'git/ref/heads/main' in pr and '--main-ref' in pr
    assert 'matching-refs/tags/v2026.10.26-ponytail' in pr
    assert '--run-id-out "$RUNNER_TEMP/qualified-run-id"' in pr
    assert '--run-id-file "$RUNNER_TEMP/qualified-run-id"' in pr
    selection=pr.index('--run-id-out "$RUNNER_TEMP/qualified-run-id"')
    metadata=pr.index('actions/runs/$run_id/artifacts?per_page=100')
    download=pr.index('gh run download "$run_id" --repo "$GITHUB_REPOSITORY" --name "ponytail-$GITHUB_SHA"')
    compare=pr.index('tools/check_ponytail_qualified_artifact.py')
    publish=pr.index('gh release create v2026.10.26-ponytail')
    assert selection < metadata < download < compare < publish, 'Qualified bytes must be checked before publication'
    assert 'gh release create v2026.10.26-ponytail' in pr and '--target "$GITHUB_SHA"' in pr and '--latest' in pr
    assert 'cmp "release/$name"' in pr and 'releases/latest/download/manifest.json' in pr
    assert 'test "$matched" = 1' in pr

def main():
    def old(path):return parse(subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT,text=True))
    oldq,oldp=old(QUAL),old(PUB)
    q,p=parse((ROOT/QUAL).read_text()),parse((ROOT/PUB).read_text())
    validate(q,p,oldq,oldp)
    checks=1
    for index,(which,edit) in enumerate((
        ('q',lambda x:x['permissions'].update(contents='write')),
        ('p',lambda x:x['permissions'].update(actions='write')),
        ('q',lambda x:x['on']['push']['branches'].append('main')),
        ('p',lambda x:x['on'].update(workflow_dispatch={})),
        ('q',lambda x:x['on']['push']['paths'].append('**')),
        ('q',lambda x:x['jobs']['validate']['steps'].append({'uses':'example/unreviewed@v1'})),
        ('p',lambda x:x['jobs']['release']['if'].__class__ and x['jobs']['release'].update({'if':'always()'})),
        ('q',lambda x:x['jobs']['validate']['steps'][2].update({'run':'echo skipped'})),
    )):
        qa,pa=copy.deepcopy(q),copy.deepcopy(p)
        target=qa if which=='q' else pa
        # PyYAML's YAML 1.1 loader maps `on` to True.
        target['on']=target.pop(True) if True in target and 'on' not in target else target['on']
        edit(target)
        if 'on' in target and True not in target:target[True]=target.pop('on')
        try: validate(qa,pa,oldq,oldp)
        except (AssertionError,KeyError,TypeError): checks+=1
        else: raise AssertionError(f'Unsafe workflow mutation accepted: {index}')
    for fragment in ('--run-id-out "$RUNNER_TEMP/qualified-run-id"',
                     'actions/runs/$run_id/artifacts?per_page=100',
                     'gh run download "$run_id" --repo "$GITHUB_REPOSITORY" --name "ponytail-$GITHUB_SHA"',
                     'tools/check_ponytail_qualified_artifact.py'):
        altered=copy.deepcopy(p)
        for step in altered['jobs']['release']['steps']:
            if 'run' in step:step['run']=step['run'].replace(fragment,'disabled-artifact-check')
        try:validate(q,altered,oldq,oldp)
        except (AssertionError,ValueError):checks+=1
        else:raise AssertionError('Missing qualified-artifact step accepted: '+fragment)
    print(f'PASS: {checks} workflow guarantees and negative mutations; push-only, exact SHA, unchanged permissions/actions/FQBN, three builds, byte readback')

if __name__=='__main__':main()
