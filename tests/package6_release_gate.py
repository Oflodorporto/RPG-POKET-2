#!/usr/bin/env python3
"""Negative tests for the exact-SHA/main-only publication prerequisite."""
from pathlib import Path
import copy,sys
sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from check_package6_release_gate import qualified
sha='1'*40;repo='Oflodorporto/RPG-POKET-2';ref='refs/heads/main'
r=dict(head_sha=sha,head_branch='feat/package6-release',path='.github/workflows/package6-check.yml',conclusion='success',status='completed',event='push',head_repository=dict(full_name=repo))
assert qualified([r],sha,ref,repo)
for field,value in [('head_sha','2'*40),('head_branch','main'),('head_branch','feat/package5-release'),('path','.github/workflows/package5-check.yml'),('conclusion','failure'),('conclusion',None),('status','in_progress'),('event','pull_request'),('head_repository',dict(full_name='other/fork'))]:
 bad=copy.deepcopy(r);bad[field]=value;assert not qualified([bad],sha,ref,repo),(field,value)
for branch in ('refs/heads/feat/package6-release','refs/heads/feat/package5-release','refs/tags/v2026.10.13-explore77'):
 assert not qualified([r],sha,branch,repo)
assert not qualified([],sha,ref,repo)
print('PASS: 14 exact-SHA/main-only gate probes; qualification branch cannot publish.')
