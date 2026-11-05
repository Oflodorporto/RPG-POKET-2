#!/usr/bin/env python3
"""Offline refusal probes for exact-run artifact identity and publisher bytes."""
if not __debug__:
    raise RuntimeError('optimized Python is forbidden')
import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile

sys.dont_write_bytecode = True
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import check_ponytail_qualified_artifact as artifact
from check_ponytail_release_gate import read_run_id

sha='1'*40
run_id=12345
name='ponytail-'+sha
entry={'id':34567,'name':name,'size_in_bytes':500,'expired':False,
       'archive_download_url':f'https://api.github.com/repos/{artifact.REPOSITORY}/actions/artifacts/34567/zip',
       'workflow_run':{'id':run_id,'head_sha':sha}}
response={'total_count':1,'artifacts':[entry]}
count=0

def reject(fn):
    global count
    try:fn()
    except (AssertionError,KeyError,ValueError,TypeError,FileNotFoundError):count+=1
    else:raise AssertionError('Invalid qualification artifact was accepted')

assert artifact.validate_metadata(response,run_id,sha)==34567;count+=1
for field,value in [('id',0),('id',True),('name','other'),('size_in_bytes',0),
                    ('expired',True),('expired',None),('archive_download_url','https://other/zip')]:
    bad=copy.deepcopy(response);bad['artifacts'][0][field]=value
    reject(lambda bad=bad:artifact.validate_metadata(bad,run_id,sha))
for field,value in [('id',run_id+1),('head_sha','2'*40)]:
    bad=copy.deepcopy(response);bad['artifacts'][0]['workflow_run'][field]=value
    reject(lambda bad=bad:artifact.validate_metadata(bad,run_id,sha))
for bad in ({'total_count':0,'artifacts':[]},{'total_count':2,'artifacts':[entry]},
            {'total_count':2,'artifacts':[entry,entry]},
            {'total_count':1,'artifacts':[None]},None,{},[]):
    reject(lambda bad=bad:artifact.validate_metadata(bad,run_id,sha))
reject(lambda:artifact.validate_metadata(response,run_id+1,sha))
reject(lambda:artifact.validate_metadata(response,run_id,'2'*40))

with tempfile.TemporaryDirectory(prefix='ponytail-artifact-') as temp:
    root=Path(temp);download=root/'download';local=root/'local'
    (download/'release').mkdir(parents=True);(download/'build').mkdir()
    local.mkdir()
    blobs={'firmware.bin':b'firmware fixture','artes.pak':b'art fixture','manifest.json':b'{"fixture":true}\n'}
    values={n:{'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()} for n,b in blobs.items()}
    for n,b in blobs.items():
        (download/'release'/n).write_bytes(b);(local/n).write_bytes(b)
    identity={'source_commit':sha,'artifacts':values,'published':False,'physical_validation':False}
    budget={'source_commit':sha,'candidate_firmware_bytes':values['firmware.bin']['bytes'],
            'target_compile_verified':True,'art_sha256':values['artes.pak']['sha256']}
    (download/'build/ponytail-source-identity.json').write_text(json.dumps(identity))
    (download/'build/ponytail-scope-budget.json').write_text(json.dumps(budget))
    (download/'release/SHA256SUMS').write_text(''.join(values[n]['sha256']+'  '+n+'\n' for n in artifact.NAMES))
    assert artifact.compare_package(download,local,sha)==values;count+=1
    for n in artifact.NAMES:
        path=local/n;path.write_bytes(b'divergent bytes')
        reject(lambda:artifact.compare_package(download,local,sha))
        path.write_bytes(blobs[n])
        path=download/'release'/n;path.unlink()
        reject(lambda:artifact.compare_package(download,local,sha))
        path.write_bytes(blobs[n])
    for rel in ('build/ponytail-source-identity.json','build/ponytail-scope-budget.json','release/SHA256SUMS'):
        path=download/rel;original=path.read_bytes();path.unlink()
        reject(lambda:artifact.compare_package(download,local,sha))
        path.write_bytes(original)
    path=download/'build/ponytail-source-identity.json'
    path.write_text(json.dumps({**identity,'source_commit':'2'*40}))
    reject(lambda:artifact.compare_package(download,local,sha))
    path.write_text(json.dumps(identity))
    path=download/'release/firmware.bin';path.unlink();path.symlink_to(local/'firmware.bin')
    reject(lambda:artifact.compare_package(download,local,sha))
    path.unlink();path.write_bytes(blobs['firmware.bin'])
    selected=root/'run-id';selected.write_text(str(run_id)+'\n')
    assert read_run_id(selected)==run_id;count+=1
    for bad in ('','0\n','-1\n','12345','12345\nextra','1.0\n','true\n'):
        selected.write_text(bad)
        reject(lambda:read_run_id(selected))

with tempfile.TemporaryDirectory(prefix='ponytail-select-') as temp:
    root=Path(temp);download=root/'download';local=root/'local'
    (download/'release').mkdir(parents=True);(download/'build').mkdir();local.mkdir()
    qualified_fw=b'qualified firmware';local_fw=b'publisher firmware'
    assert len(qualified_fw)==len(local_fw)
    art=b'identical art'
    qualified_manifest={'version':'fixture','partition':'same','firmware':{'bytes':len(qualified_fw),'sha256':hashlib.sha256(qualified_fw).hexdigest()},'art':{'sha256':hashlib.sha256(art).hexdigest()}}
    local_manifest=copy.deepcopy(qualified_manifest)
    local_manifest['firmware']['sha256']=hashlib.sha256(local_fw).hexdigest()
    blobs={'firmware.bin':qualified_fw,'artes.pak':art,'manifest.json':(json.dumps(qualified_manifest)+'\n').encode()}
    values={n:{'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()} for n,b in blobs.items()}
    for n,b in blobs.items(): (download/'release'/n).write_bytes(b)
    (local/'firmware.bin').write_bytes(local_fw)
    (local/'artes.pak').write_bytes(art)
    (local/'manifest.json').write_text(json.dumps(local_manifest)+'\n')
    identity={'source_commit':sha,'artifacts':values,'published':False,'physical_validation':False}
    budget={'source_commit':sha,'candidate_firmware_bytes':len(qualified_fw),'target_compile_verified':True,'art_sha256':values['artes.pak']['sha256']}
    (download/'build/ponytail-source-identity.json').write_text(json.dumps(identity))
    (download/'build/ponytail-scope-budget.json').write_text(json.dumps(budget))
    (download/'release/SHA256SUMS').write_text(''.join(values[n]['sha256']+'  '+n+'\n' for n in artifact.NAMES))
    real_verify,real_check=artifact.verify,artifact.check_image
    artifact.verify=lambda *args,**kwargs:None
    artifact.check_image=lambda *args,**kwargs:None
    try:
        published=root/'published'
        assert artifact.select_qualified_package(download,local,published,sha)==values;count+=1
        assert (published/'firmware.bin').read_bytes()==qualified_fw != local_fw;count+=1
        reject(lambda:artifact.select_qualified_package(download,local,published,sha))
        (local/'artes.pak').write_bytes(b'wrong art')
        reject(lambda:artifact.select_qualified_package(download,local,root/'bad-art',sha))
        (local/'artes.pak').write_bytes(art)
        bad=copy.deepcopy(local_manifest);bad['partition']='wrong'
        (local/'manifest.json').write_text(json.dumps(bad))
        reject(lambda:artifact.select_qualified_package(download,local,root/'bad-contract',sha))
        (local/'manifest.json').write_text(json.dumps(local_manifest)+'\n')
        (local/'firmware.bin').write_bytes(local_fw+b'!')
        reject(lambda:artifact.select_qualified_package(download,local,root/'bad-size',sha))
        (local/'firmware.bin').write_bytes(local_fw)
        (download/'release/firmware.bin').write_bytes(b'corrupted firmware')
        reject(lambda:artifact.select_qualified_package(download,local,root/'bad-qualified',sha))
    finally:
        artifact.verify,artifact.check_image=real_verify,real_check

print(f'PASS: {count} exact-run metadata, absent/expired artifact, SHA, byte divergence, provenance and malformed ID refusal probes')
