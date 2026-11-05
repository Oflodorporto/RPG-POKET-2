#!/usr/bin/env python3
"""Fail-closed exact-source gate for the reviewed Ponytail visual update.

The historical wifitest77 gate and its frozen inventory remain untouched.
"""
if not __debug__:
    raise RuntimeError('optimized Python is forbidden')
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASE = 'aecfabf32edba31fce567a32450503be3492ea66'
REVIEWED_CANDIDATE = '7382fc7ab93cd973f5783fbd80370b3cf064db99'
RELEASE_FIX_PATHS = [
    '.github/workflows/release.yml',
    'tests/fixtures/ponytail-approved-sources.json',
    'tests/ponytail_qualification_config.py',
    'tests/ponytail_qualified_artifact.py',
    'tools/check_ponytail_qualified_artifact.py',
    'tools/check_ponytail_scope.py',
    'tools/test_ponytail_release.py',
]
BRANCH = 'feat/ponytail-release'
VERSION = '2026.10.26-ponytail'
BUILD = 2026102601
PACKAGE = '7.8 - Bolsa e visual'
EXEMPT = {'tools/check_ponytail_scope.py', 'tests/fixtures/ponytail-approved-sources.json'}
GENERATED = {'release/firmware.bin','release/artes.pak','release/manifest.json','release/SHA256SUMS'}
MANIFEST = ROOT/'tests/fixtures/ponytail-approved-sources.json'
# Filled after the final source review. A changed inventory must be reviewed,
# never silently accepted by a command in this repository.
INVENTORY_SHA256 = 'b4929874e62866c897633334015a006e0af3b4c2663e05ae45233d6939b5b0ed'
PROTECTED = {
    'firmware/RPG_POKET_2/Save.h',
    'firmware/RPG_POKET_2/SaveVersion.h',
    'firmware/RPG_POKET_2/Persistence.h',
    'firmware/RPG_POKET_2/NvsPersistence.h',
    'firmware/RPG_POKET_2/Rules.h',
    'firmware/RPG_POKET_2/ResetEpoch.h',
    'firmware/RPG_POKET_2/DeviceSettings.h',
    'firmware/RPG_POKET_2/UpdateDevice.h',
    'firmware/RPG_POKET_2/UpdateInstall.cpp',
    'firmware/RPG_POKET_2/UpdateInstallBridge.h',
    'firmware/RPG_POKET_2/UpdateModel.h',
}

def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)

def validate_entry(entry):
    assert set(entry) == {'path','bytes','sha256','executable'}
    p = entry['path']
    assert isinstance(p,str) and p and not Path(p).is_absolute() and '..' not in Path(p).parts
    assert '\\' not in p and '\x00' not in p and str(Path(p)) == p
    assert type(entry['bytes']) is int and entry['bytes'] >= 0
    assert isinstance(entry['sha256'],str) and re.fullmatch('[0-9a-f]{64}',entry['sha256'])
    assert type(entry['executable']) is bool
    assert p not in EXEMPT

def validate_inventory(manifest, paths):
    assert set(manifest) == {'schema','base_commit','version','build','package','delta','files','art_sha256'}
    assert manifest['schema'] == 1 and manifest['base_commit'] == BASE
    assert (manifest['version'],manifest['build'],manifest['package']) == (VERSION,BUILD,PACKAGE)
    files = manifest['files']
    assert isinstance(files,list) and files
    for entry in files: validate_entry(entry)
    names = [e['path'] for e in files]
    assert names == sorted(set(names)), 'Missing, duplicate or unsorted source path'
    assert set(names) | EXEMPT == paths, 'Unapproved added/removed source path'
    payload = json.dumps(files,sort_keys=True,separators=(',',':')).encode()
    assert hashlib.sha256(payload).hexdigest() == INVENTORY_SHA256, 'Frozen reviewed inventory changed'
    for e in files:
        p=ROOT/e['path']
        assert p.is_file() and not p.is_symlink(), e['path']
        assert not any(parent.is_symlink() for parent in p.parents if parent!=ROOT and ROOT in parent.parents),e['path']
        data=p.read_bytes()
        assert len(data)==e['bytes'] and hashlib.sha256(data).hexdigest()==e['sha256'], 'Source changed: '+e['path']
        assert bool(p.stat().st_mode & 0o111)==e['executable'], 'Mode changed: '+e['path']

def validate_delta(manifest, head):
    assert re.fullmatch('[0-9a-f]{40}',head)
    assert git('rev-list','--parents','-n','1',REVIEWED_CANDIDATE).decode().split() == [REVIEWED_CANDIDATE,BASE], 'Reviewed visual candidate ancestry changed'
    assert git('rev-list','--parents','-n','1',head).decode().split() == [head,REVIEWED_CANDIDATE], 'Release fix must be a single child of reviewed candidate'
    assert git('diff','--name-only',REVIEWED_CANDIDATE,head).decode().splitlines() == RELEASE_FIX_PATHS, 'Release fix changed unreviewed paths'
    actual=git('diff','--name-only',BASE,head).decode().splitlines()
    assert actual == manifest['delta'], 'Unreviewed source delta'
    assert len(actual)==len(set(actual))
    assert not (PROTECTED & set(actual)), 'Save, rules, updater or persistence changed'
    for p in PROTECTED:
        assert (ROOT/p).read_bytes()==git('show',BASE+':'+p), 'Protected bytes changed: '+p

def validate_ci_context(env, head):
    assert env.get('GITHUB_SHA')==head and env.get('GITHUB_REPOSITORY')=='Oflodorporto/RPG-POKET-2'
    assert env.get('GITHUB_EVENT_NAME')=='push'
    assert env.get('GITHUB_REF') in {'refs/heads/'+BRANCH, 'refs/heads/main'}

def main():
    assert git('rev-parse',BASE+'^{commit}').decode().strip()==BASE
    head=git('rev-parse','HEAD').decode().strip()
    if os.environ.get('GITHUB_ACTIONS')=='true':
        validate_ci_context(os.environ, head)
    paths={p.decode() for p in git('ls-files','--cached','-z').split(b'\0') if p}
    untracked={p.decode() for p in git('ls-files','--others','--exclude-standard','-z').split(b'\0') if p}
    assert untracked <= GENERATED, 'Untracked source file'
    manifest=json.loads(MANIFEST.read_text())
    validate_inventory(manifest,paths)
    validate_delta(manifest,head)
    assert re.fullmatch('[0-9a-f]{64}',manifest['art_sha256'])
    art=ROOT/'cartao/RPGPOKET/artes.pak'
    assert hashlib.sha256(art.read_bytes()).hexdigest()==manifest['art_sha256']
    header=(ROOT/'firmware/RPG_POKET_2/ReleaseVersion.h').read_text()
    assert f'FW_VERSION="{VERSION}"' in header and f'FW_BUILD={BUILD}u' in header and f'FW_PACKAGE="{PACKAGE}"' in header
    assert git('diff','--name-only').decode().strip()=='' and git('diff','--cached','--name-only').decode().strip()==''
    budget={'schema':1,'source_commit':head,'source_tree':git('rev-parse','HEAD^{tree}').decode().strip(),
            'baseline_commit':BASE,'approved_inventory_sha256':INVENTORY_SHA256,
            'reviewed_delta':manifest['delta'],'qualification_groups':165,
            'version':VERSION,'build':BUILD,'save_format':30,'save_reset_epoch':1,
            'new_reset':False,'art_sha256':manifest['art_sha256'],'art_assets':666,
            'ota_partition_bytes':0x300000,'framebuffer_bytes':153600,'decode_workspace_bytes':13760,
            'known_psram_bytes':7638272,'candidate_firmware_bytes':None,
            'target_compile_verified':False,'physical_validation':False,'published':False}
    (ROOT/'build').mkdir(exist_ok=True)
    (ROOT/'build/ponytail-scope-budget.json').write_text(json.dumps(budget,indent=2)+'\n')
    print('PASS: exact reviewed Ponytail source and release-fix ancestry, strict path/hash/mode inventory, protected save/rules/updater, art and identity')

if __name__=='__main__': main()
