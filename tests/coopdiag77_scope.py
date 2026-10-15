#!/usr/bin/env python3
"""Offline negative source-inventory probes; no production files are mutated."""
from pathlib import Path
import copy,hashlib,json,sys,tempfile
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import check_coopdiag77_scope as scope
from check_coopdiag77_scope import (validate_files,validate_ancestry,validate_frozen,
 validate_paths,validate_reviewed_production,assert_generated_inventory,production_inventory)
checks=0
def reject(fn):
 global checks
 try:fn()
 except (AssertionError,KeyError,ValueError,FileNotFoundError,TypeError):checks+=1
 else:raise AssertionError('Accepted invalid frozen source input')
with tempfile.TemporaryDirectory(prefix='coopdiag77-scope-') as d:
 root=Path(d);path=root/'firmware/test.h';path.parent.mkdir();path.write_bytes(b'fixed source\n')
 entry={'path':'firmware/test.h','bytes':13,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'executable':False}
 validate_files(root,[entry]);checks+=1
 for field,value in [('path','../escape'),('path','/absolute'),('path',''),('path','firmware/../test.h'),
                     ('path','./firmware/test.h'),('path','firmware//test.h'),('path','firmware/test.h/'),
                     ('path','firmware\\test.h'),('path','firmware/\x00test.h'),('path',None),
                     ('bytes',12),('bytes',-1),('bytes',True),('bytes',13.0),
                     ('sha256','0'*64),('sha256','a'*63),('sha256','A'*64),('sha256',None),
                     ('executable',True),('executable',0),('executable','false')]:
  bad={**entry,field:value};reject(lambda:validate_files(root,[bad]))
 for field in entry:
  bad=copy.deepcopy(entry);del bad[field];reject(lambda:validate_files(root,[bad]))
 reject(lambda:validate_files(root,[{**entry,'mode':'100644'}]))
 reject(lambda:validate_files(root,(entry,)))
 reject(lambda:validate_files(root,[entry,entry]))
 reject(lambda:validate_files(root,[{**entry,'path':'tools/check_coopdiag77_scope.py'}]))
 path.write_bytes(b'changed data\n');reject(lambda:validate_files(root,[entry]));path.write_bytes(b'fixed source\n')
 path.chmod(0o755);reject(lambda:validate_files(root,[entry]));path.chmod(0o644)
 target=root/'target';target.write_bytes(path.read_bytes());path.unlink();path.symlink_to(target)
 reject(lambda:validate_files(root,[entry]));path.unlink();path.write_bytes(b'fixed source\n')
 path.unlink();path.mkdir();reject(lambda:validate_files(root,[entry]));path.rmdir();path.write_bytes(b'fixed source\n')
 outside=root/'outside';outside.mkdir();(outside/'test.h').write_bytes(path.read_bytes())
 link=root/'via-link';link.symlink_to(outside,target_is_directory=True)
 reject(lambda:validate_files(root,[{**entry,'path':'via-link/test.h'}]));link.unlink()
 assert production_inventory(root,{'firmware/test.h'})=={'firmware/test.h'};checks+=1
 for name in ('hidden.h','sketch.ino.bin','build/sketch.o','build/sketch.elf','build/sketch.map'):
  extra=root/'firmware'/name;extra.parent.mkdir(parents=True,exist_ok=True);extra.write_text('hidden')
  reject(lambda:production_inventory(root,{'firmware/test.h'}));extra.unlink()
 extra=root/'firmware/symlink';extra.symlink_to(target);reject(lambda:production_inventory(root,{'firmware/test.h','firmware/symlink'}));extra.unlink()
 assert_generated_inventory(root,['firmware/test.h']);checks+=1
 for name in sorted(scope.GENERATED):
  reject(lambda name=name:assert_generated_inventory(root,[name]))
  output=root/name;output.parent.mkdir(exist_ok=True);output.symlink_to(target)
  reject(lambda:assert_generated_inventory(root,[]));output.unlink()
 (root/'release').rmdir();(root/'release').symlink_to(outside,target_is_directory=True)
 reject(lambda:assert_generated_inventory(root,[]));(root/'release').unlink()
 # All packaging is explicitly required, with no symlink escapes or unrelated files.
 for name in scope.PACKAGING:
  p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text('packaging fixture\n')
 expected={entry['path']}|scope.PACKAGING
 assert validate_paths(root,[entry],[],expected)==expected;checks+=1
 for name in expected:reject(lambda name=name:validate_paths(root,[entry],[],expected-{name}))
 for name in ('unreviewed.cpp','firmware/generated.bin','tools/__pycache__/gate.pyc','tests/generated.o'):
  reject(lambda name=name:validate_paths(root,[entry],[],expected|{name}))
 for name in scope.GENERATED:
  assert validate_paths(root,[entry],[],expected|{name})==expected;checks+=1
  reject(lambda name=name:validate_paths(root,[entry],[name],expected|{name}))
 p=root/'tools/check_coopdiag77_scope.py';data=p.read_bytes();p.unlink();p.symlink_to(target)
 reject(lambda:validate_paths(root,[entry],[],expected));p.unlink();p.write_bytes(data)
 tools=root/'tools';moved=root/'moved-tools';tools.rename(moved);tools.symlink_to(moved,target_is_directory=True)
 reject(lambda:validate_paths(root,[entry],[],expected));tools.unlink();moved.rename(tools)

candidate='a'*40
validate_ancestry(scope.BASELINE,[scope.BASELINE,scope.COOP_COMMIT]);checks+=1
validate_ancestry(candidate,[candidate,scope.BASELINE]);checks+=1
validate_ancestry(candidate,[candidate,scope.BASELINE],True,candidate);checks+=1
for head,parents,ci,ci_sha in (
 (candidate,[candidate],False,None),
 (candidate,[candidate,scope.COOP_COMMIT],False,None),
 (candidate,[candidate,scope.BASELINE,scope.COOP_COMMIT],False,None),
 (candidate,[scope.COOP_COMMIT,scope.BASELINE],False,None),
 (scope.BASELINE,[scope.BASELINE,scope.COOP_COMMIT],True,scope.BASELINE),
 (candidate,[candidate,scope.BASELINE],True,None),
 (candidate,[candidate,scope.BASELINE],True,'b'*40),
 ('short',['short',scope.BASELINE],False,None),
 ('A'*40,['A'*40,scope.BASELINE],False,None),
):reject(lambda:validate_ancestry(head,parents,ci,ci_sha))

# Exercise fixture parsing with a self-contained synthetic inventory. This never
# substitutes a different digest for the production gate or mutates source files.
fixture={'schema':1,'baseline_commit':scope.BASELINE,'baseline_tree':scope.BASELINE_TREE,
 'coop_commit':scope.COOP_COMMIT,'coop_tree':scope.COOP_TREE,'packaging_exceptions':sorted(scope.PACKAGING),
 'reviewed_changes':sorted(scope.REVIEWED_CHANGES),'reviewed_production':scope.REVIEWED_PRODUCTION,
 'contract':scope.CONTRACT,'frozen_files':[]}
real_inventory_sha=scope.INVENTORY_SHA
try:
 scope.INVENTORY_SHA=hashlib.sha256(b'[]').hexdigest()
 assert validate_frozen(fixture)==[];checks+=1
 for key in fixture:
  bad=copy.deepcopy(fixture);del bad[key];reject(lambda:validate_frozen(bad))
 for key,value in [('schema',True),('schema',2),('baseline_commit',scope.COOP_COMMIT),
                   ('baseline_tree','0'*40),('coop_commit',scope.BASELINE),('coop_tree','0'*40),
                   ('packaging_exceptions',[]),('reviewed_changes',[]),('reviewed_production',{}),
                   ('contract',{}),('frozen_files',[{'path':'unreviewed.cpp'}])]:
  bad=copy.deepcopy(fixture);bad[key]=value;reject(lambda:validate_frozen(bad))
 bad=copy.deepcopy(fixture);bad['unexpected']=True;reject(lambda:validate_frozen(bad))
 for key in ('packaging_exceptions','reviewed_changes'):
  bad=copy.deepcopy(fixture);bad[key].append(bad[key][0]);reject(lambda:validate_frozen(bad))
  bad=copy.deepcopy(fixture);bad[key].reverse();reject(lambda:validate_frozen(bad))
 for key,value in [('qualification_groups',156),('version','2026.10.15-coopfix77'),('build',2026101501),
                   ('save_format',31),('save_bytes',161),('pvp_protocol_version',6),('pvp_packet_bytes',53),
                   ('coop_admission_version',3),('new_psram_allocations',1),('art_assets',667)]:
  bad=copy.deepcopy(fixture);bad['contract'][key]=value;reject(lambda:validate_frozen(bad))
finally:scope.INVENTORY_SHA=real_inventory_sha

# The external review's eleven production hashes must still match, independent
# of the full inventory. Do not modify any production file for a negative probe.
entries=[{'path':name,'sha256':digest} for name,digest in scope.REVIEWED_PRODUCTION.items()]
validate_reviewed_production(ROOT,entries);checks+=1
for index in range(len(entries)):
 bad=copy.deepcopy(entries);bad[index]['sha256']='0'*64;reject(lambda:validate_reviewed_production(ROOT,bad))
 reject(lambda index=index:validate_reviewed_production(ROOT,entries[:index]+entries[index+1:]))

print(f'PASS: {checks} coopdiag77 frozen-source path/hash/mode/symlink, exact-parent, manifest, packaging and generated-payload negative probes. Offline only.')
