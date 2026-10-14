#!/usr/bin/env python3
"""Negative probes for the main-push/exact-SHA publication prerequisite."""
from pathlib import Path
import copy
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from check_package10_release_gate import qualified
sha = '1' * 40
repo = 'Oflodorporto/RPG-POKET-2'
ref = 'refs/heads/main'
r = dict(head_sha=sha, head_branch='feat/package10-release', path='.github/workflows/package10-check.yml', conclusion='success', status='completed', event='push', head_repository=dict(full_name=repo))
assert qualified([r], sha, ref, repo)
count = 1
for field, value in [('head_sha', '2'*40), ('head_branch', 'main'), ('head_branch', 'feat/package6-release'), ('path', '.github/workflows/package6-check.yml'), ('conclusion', 'failure'), ('conclusion', None), ('status', 'in_progress'), ('status', 'queued'), ('event', 'pull_request'), ('event', 'workflow_dispatch'), ('head_repository', dict(full_name='other/fork')), ('head_repository', None), ('head_repository', {})]:
    bad = copy.deepcopy(r); bad[field] = value
    assert not qualified([bad], sha, ref, repo), (field, value)
    count += 1
for branch in ('refs/heads/feat/package10-release', 'refs/heads/feat/package6-release', 'refs/tags/v2026.10.14-pvp77', ''):
    assert not qualified([r], sha, branch, repo); count += 1
for invalid in ([], None, {}, [None], ['success']):
    assert not qualified(invalid, sha, ref, repo); count += 1
for invalid in ('', None, 'not-a-repository'):
    assert not qualified([r], sha, ref, invalid); count += 1
for invalid in ('1'*39, '1'*41, 'G'*40, None):
    assert not qualified([r], invalid, ref, repo); count += 1
print(f'PASS: {count} exact-SHA, same-repository and main-only qualification gate probes.')
