#!/usr/bin/env python3
"""Negative probes for the new exact-inventory gate; no repository mutation."""
if not __debug__:
    raise RuntimeError('optimized Python is forbidden; optimized Python is forbidden')
import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import check_ponytail_scope as scope

checks=0
def reject(fn):
    global checks
    try: fn()
    except (AssertionError,ValueError,TypeError,KeyError,FileNotFoundError): checks+=1
    else: raise AssertionError('Invalid source accepted')

with tempfile.TemporaryDirectory(prefix='ponytail-scope-') as d:
    root=Path(d)
    p=root/'firmware/test.h';p.parent.mkdir();p.write_bytes(b'fixed source\n')
    entry={'path':'firmware/test.h','bytes':13,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'executable':False}
    files=[entry]
    manifest={'schema':1,'base_commit':scope.BASE,'version':scope.VERSION,'build':scope.BUILD,'package':scope.PACKAGE,
              'delta':[],'files':files,'art_sha256':'0'*64}
    old_root,old_sha=scope.ROOT,scope.INVENTORY_SHA256
    scope.ROOT=root
    scope.INVENTORY_SHA256=hashlib.sha256(json.dumps(files,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    paths={'firmware/test.h'}|scope.EXEMPT
    try:
        scope.validate_inventory(manifest,paths);checks+=1
        for field,value in [('path','../escape'),('path','/absolute'),('path',''),('path','./firmware/test.h'),
                            ('path','firmware\\test.h'),('path','firmware/../test.h'),('path',None),
                            ('bytes',12),('bytes',-1),('bytes',True),('sha256','0'*64),('sha256','A'*64),
                            ('executable',True),('executable',0)]:
            bad=copy.deepcopy(manifest);bad['files'][0][field]=value
            reject(lambda bad=bad:scope.validate_inventory(bad,paths))
        for field in entry:
            bad=copy.deepcopy(manifest);del bad['files'][0][field]
            reject(lambda bad=bad:scope.validate_inventory(bad,paths))
        for field,value in [('schema',2),('base_commit','0'*40),('version','old'),('build',0),('package','old')]:
            bad=copy.deepcopy(manifest);bad[field]=value
            reject(lambda bad=bad:scope.validate_inventory(bad,paths))
        reject(lambda:scope.validate_inventory(manifest,paths|{'unexpected.h'}))
        bad=copy.deepcopy(manifest);bad['files'].append(entry)
        reject(lambda:scope.validate_inventory(bad,paths))
        p.write_bytes(b'changed source\n');reject(lambda:scope.validate_inventory(manifest,paths));p.write_bytes(b'fixed source\n')
        p.chmod(0o755);reject(lambda:scope.validate_inventory(manifest,paths));p.chmod(0o644)
        target=root/'target';target.write_bytes(p.read_bytes());p.unlink();p.symlink_to(target)
        reject(lambda:scope.validate_inventory(manifest,paths));p.unlink();p.write_bytes(b'fixed source\n')
    finally:
        scope.ROOT,scope.INVENTORY_SHA256=old_root,old_sha
ci={'GITHUB_SHA':'a'*40,'GITHUB_REPOSITORY':'Oflodorporto/RPG-POKET-2',
    'GITHUB_EVENT_NAME':'push','GITHUB_REF':'refs/heads/'+scope.BRANCH}
scope.validate_ci_context(ci,'a'*40);checks+=1
scope.validate_ci_context({**ci,'GITHUB_REF':'refs/heads/main'},'a'*40);checks+=1
for field,value in [('GITHUB_SHA','b'*40),('GITHUB_REPOSITORY','other/repo'),
                    ('GITHUB_EVENT_NAME','pull_request'),('GITHUB_REF','refs/heads/other'),
                    ('GITHUB_REF','refs/tags/v2026.10.26-ponytail')]:
    reject(lambda field=field,value=value:scope.validate_ci_context({**ci,field:value},'a'*40))

print(f'PASS: {checks} exact source path/hash/size/mode, symlink, identity and inventory refusal probes')
