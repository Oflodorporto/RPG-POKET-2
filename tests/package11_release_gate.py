#!/usr/bin/env python3
"""Fail-closed positive and negative tests for the future exact-SHA publisher."""
from pathlib import Path
import copy
import json
import os
import subprocess
import sys
import tempfile
from unittest.mock import patch
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import check_package11_release_gate as gate

sha = '1' * 40
repo = 'Oflodorporto/RPG-POKET-2'
ref = 'refs/heads/main'
run = dict(head_sha=sha, head_branch='feat/package11-release', path='.github/workflows/package11-check.yml', conclusion='success', status='completed', event='push', head_repository=dict(full_name=repo))
assert gate.qualified([run], sha, ref, repo)
count = 1
for field, value in [
    ('head_sha', '2'*40), ('head_sha', sha.upper()+'0'),
    ('head_branch', 'main'), ('head_branch', 'feat/package10-release'),
    ('head_branch', 'feat/package11-release-extra'),
    ('path', '.github/workflows/package10-check.yml'),
    ('path', '.github/workflows/package11-check.yml@refs/heads/main'),
    ('conclusion', 'failure'), ('conclusion', None), ('conclusion', 'cancelled'),
    ('status', 'in_progress'), ('status', 'queued'),
    ('event', 'pull_request'), ('event', 'pull_request_target'), ('event', 'workflow_dispatch'),
    ('event', 'workflow_run'), ('event', 'schedule'),
    ('head_repository', dict(full_name='other/fork')), ('head_repository', None),
    ('head_repository', {}), ('head_repository', repo),
]:
    bad = copy.deepcopy(run); bad[field] = value
    assert not gate.qualified([bad], sha, ref, repo), (field, value)
    count += 1
for field in run:
    bad = copy.deepcopy(run); del bad[field]
    assert not gate.qualified([bad], sha, ref, repo), field
    count += 1
for branch in ('refs/heads/feat/package11-release', 'refs/heads/feat/package10-release', 'refs/tags/v2026.10.14-shop77', 'main', '', None):
    assert not gate.qualified([run], sha, branch, repo); count += 1
for invalid in ([], None, {}, [None], ['success'], {'workflow_runs': [run]}):
    assert not gate.qualified(invalid, sha, ref, repo); count += 1
for invalid in ('', None, 'not-a-repository', 'other/fork', repo.lower()):
    forged = copy.deepcopy(run); forged['head_repository'] = dict(full_name=invalid)
    assert not gate.qualified([forged], sha, ref, invalid); count += 1
for invalid in ('1'*39, '1'*41, 'G'*40, None, 1, '1'*40+'\n'):
    assert not gate.qualified([run], invalid, ref, repo); count += 1
assert gate.qualified([None, {'status': 'queued'}, run], sha, ref, repo)
count += 1

# The executable gate also checks the current event, publisher identity and checkout.
with tempfile.TemporaryDirectory(prefix='package11-release-gate-') as temporary:
    fixture = Path(temporary)/'qualification.json'
    fixture.write_text(json.dumps({'workflow_runs': [run]}))
    environ = dict(GITHUB_SHA=sha, GITHUB_REF=ref, GITHUB_REPOSITORY=repo,
                   GITHUB_EVENT_NAME='push', GITHUB_WORKFLOW_REF=gate.RELEASE_WORKFLOW_REF)
    def execute(updates=None, checkout=sha):
        values = {**environ, **(updates or {})}
        with patch.dict(os.environ, values), patch.object(sys, 'argv', ['gate', str(fixture)]), patch('subprocess.check_output', return_value=checkout+'\n'):
            gate.main()
    execute(); count += 1
    for updates, checkout in (
        ({'GITHUB_EVENT_NAME': 'workflow_dispatch'}, sha),
        ({'GITHUB_REF': 'refs/heads/feat/package11-release'}, sha),
        ({'GITHUB_REPOSITORY': 'other/fork'}, sha),
        ({'GITHUB_WORKFLOW_REF': gate.RELEASE_WORKFLOW_REF.replace('release.yml', 'other.yml')}, sha),
        ({'GITHUB_WORKFLOW_REF': gate.RELEASE_WORKFLOW_REF.replace('@refs/heads/main', '@refs/heads/feat/package11-release')}, sha),
        ({}, '2'*40),
    ):
        try: execute(updates, checkout)
        except AssertionError: count += 1
        else: raise AssertionError('Accepted invalid publisher context: '+repr(updates))
    for value in (None, [], {}, {'workflow_runs': None}, {'workflow_runs': [None]}):
        fixture.write_text(json.dumps(value))
        try: execute()
        except AssertionError: count += 1
        else: raise AssertionError('Accepted malformed qualification payload')
print(f'PASS: {count} exact-SHA, fixed-repository, branch/workflow, event and malformed-input gate probes.')
