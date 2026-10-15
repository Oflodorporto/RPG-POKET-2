#!/usr/bin/env python3
"""Offline negative source-inventory probes; no production files are mutated."""
from pathlib import Path
import copy,hashlib,sys,tempfile
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from check_coopfix77_scope import validate_files,assert_generated_inventory,production_inventory
checks=0
def reject(fn):
 global checks
 try:fn()
 except (AssertionError,KeyError,ValueError,FileNotFoundError):checks+=1
 else:raise AssertionError('Accepted invalid frozen source input')
with tempfile.TemporaryDirectory(prefix='coopfix77-scope-') as d:
 root=Path(d);path=root/'firmware/test.h';path.parent.mkdir();path.write_bytes(b'fixed source\n')
 entry={'path':'firmware/test.h','bytes':13,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'executable':False}
 validate_files(root,[entry]);checks+=1
 for field,value in [('path','../escape'),('path','/absolute'),('path',''),('bytes',12),('sha256','0'*64),('executable',True)]:
  bad={**entry,field:value};reject(lambda:validate_files(root,[bad]))
 reject(lambda:validate_files(root,[entry,entry]))
 path.write_bytes(b'changed data\n');reject(lambda:validate_files(root,[entry]));path.write_bytes(b'fixed source\n')
 path.chmod(0o755);reject(lambda:validate_files(root,[entry]));path.chmod(0o644)
 target=root/'target';target.write_bytes(path.read_bytes());path.unlink();path.symlink_to(target);reject(lambda:validate_files(root,[entry]));path.unlink();path.write_bytes(b'fixed source\n')
 assert production_inventory(root,{'firmware/test.h'})=={'firmware/test.h'};checks+=1
 extra=root/'firmware/hidden.h';extra.write_text('hidden');reject(lambda:production_inventory(root,{'firmware/test.h'}));extra.unlink()
 assert_generated_inventory(root,['firmware/test.h']);checks+=1
 for name in ('release/firmware.bin','release/artes.pak','release/manifest.json','release/SHA256SUMS'):
  reject(lambda name=name:assert_generated_inventory(root,[name]))
  output=root/name;output.parent.mkdir(exist_ok=True);output.symlink_to(target)
  reject(lambda:assert_generated_inventory(root,[]));output.unlink()
print(f'PASS: {checks} scope path/hash/mode/duplicate/symlink/generated-payload negative probes. Offline only.')
