#!/usr/bin/env python3
"""Fail-closed positive and negative tests for the future exact-SHA publisher."""
if not __debug__:
    raise RuntimeError("Qualification requires assertions; optimized Python is forbidden")
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
import check_wifitest77_release_gate as gate

sha = '1' * 40
repo = 'Oflodorporto/RPG-POKET-2'
ref = 'refs/heads/main'
run = dict(head_sha=sha, head_branch='feat/wifitest77-release', path='.github/workflows/wifitest77-check.yml', conclusion='success', status='completed', event='push', head_repository=dict(full_name=repo))
assert gate.qualified([run], sha, ref, repo)
count = 1
for field, value in [
    ('head_sha', '2'*40), ('head_sha', sha.upper()+'0'),
    ('head_branch', 'main'), ('head_branch', 'feat/package11-release'),
    ('head_branch', 'feat/wifitest77-release-extra'),
    ('path', '.github/workflows/package11-check.yml'),
    ('path', '.github/workflows/wifitest77-check.yml@refs/heads/main'),
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
for branch in ('refs/heads/feat/wifitest77-release', 'refs/heads/feat/package11-release', 'refs/tags/v2026.10.17-wifitest77', 'main', '', None):
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
with tempfile.TemporaryDirectory(prefix='wifitest77-release-gate-') as temporary:
    fixture = Path(temporary)/'qualification.json'
    fixture.write_text(json.dumps({'workflow_runs': [run]}))
    environ = dict(GITHUB_SHA=sha, GITHUB_REF=ref, GITHUB_REPOSITORY=repo,
                   GITHUB_EVENT_NAME='push', GITHUB_WORKFLOW_REF=gate.RELEASE_WORKFLOW_REF, GITHUB_WORKFLOW_SHA=sha)
    def execute(updates=None, checkout=sha):
        values = {**environ, **(updates or {})}
        with patch.dict(os.environ, values), patch.object(sys, 'argv', ['gate', str(fixture)]), patch('subprocess.check_output', return_value=checkout+'\n'):
            gate.main()
    execute(); count += 1
    for updates, checkout in (
        ({'GITHUB_EVENT_NAME': 'workflow_dispatch'}, sha),
        ({'GITHUB_REF': 'refs/heads/feat/wifitest77-release'}, sha),
        ({'GITHUB_REPOSITORY': 'other/fork'}, sha),
        ({'GITHUB_WORKFLOW_REF': gate.RELEASE_WORKFLOW_REF.replace('release.yml', 'other.yml')}, sha),
        ({'GITHUB_WORKFLOW_REF': gate.RELEASE_WORKFLOW_REF.replace('@refs/heads/main', '@refs/heads/feat/wifitest77-release')}, sha),
        ({'GITHUB_WORKFLOW_SHA': '2'*40}, sha),
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
# A freshly fetched main ref must still name this exact commit immediately
# before release creation. Matching run evidence cannot excuse main drift.
main_ref = {'ref': 'refs/heads/main', 'object': {'type': 'commit', 'sha': sha}}
assert gate.live_main_matches(main_ref, sha); count += 1
for value in (None, [], {}, {'message': 'Not Found'}, {'ref': ref},
              {'ref': ref, 'object': None}, {'ref': ref, 'object': []},
              {'ref': ref, 'object': {}}, {'ref': ref, 'object': {'sha': sha}},
              {'ref': ref, 'object': {'type': 'commit'}},
              {'ref': 'refs/tags/v2026.10.17-wifitest77', 'object': main_ref['object']},
              {'ref': 'refs/heads/main-extra', 'object': main_ref['object']}):
    assert not gate.live_main_matches(value, sha); count += 1
for value in ('2'*40, '1'*39, '1'*41, 'A'*40, 'g'*40, sha+'\n', None, True, 1):
    bad = copy.deepcopy(main_ref); bad['object']['sha'] = value
    assert not gate.live_main_matches(bad, sha); count += 1
    assert not gate.live_main_matches(main_ref, value); count += 1
for value in ('tag', 'tree', 'blob', '', None, True):
    bad = copy.deepcopy(main_ref); bad['object']['type'] = value
    assert not gate.live_main_matches(bad, sha); count += 1

with tempfile.TemporaryDirectory(prefix='wifitest77-live-main-') as temporary:
    folder = Path(temporary)
    fixture = folder/'qualification.json'; fixture.write_text(json.dumps({'workflow_runs': [run]}))
    live = folder/'main-ref.json'; live.write_text(json.dumps(main_ref))
    arguments = ['gate', str(fixture), '--main-ref', str(live)]
    def execute_live(checkout=sha):
        with patch.dict(os.environ, environ), patch.object(sys, 'argv', arguments), patch('subprocess.check_output', return_value=checkout+'\n'):
            gate.main()
    execute_live(); count += 1
    for payload in ('', '{', 'null', '[]', '{"message":"API rate limit exceeded"}',
                    json.dumps({**main_ref, 'object': {'type': 'commit', 'sha': '2'*40}})):
        live.write_text(payload)
        try: execute_live()
        except (AssertionError, ValueError): count += 1
        else: raise AssertionError('Accepted missing, malformed or stale live main response')
    live.write_text(json.dumps(main_ref))
    try: execute_live('2'*40)
    except AssertionError: count += 1
    else: raise AssertionError('Accepted changed checkout during live main gate')
    live.unlink()
    try: execute_live()
    except FileNotFoundError: count += 1
    else: raise AssertionError('Accepted absent live main response')
    # Run the real final shell fragment with a fake gh function. A failed API
    # must stop before either the Python gate or even the fake release command.
    root = Path(__file__).resolve().parents[1]
    workflow = (root/'.github/workflows/release.yml').read_text()
    first = '          gh api "repos/$GITHUB_REPOSITORY/git/ref/heads/main"'
    last = " --notes-file docs/RELEASE_WIFITEST77_NOTAS.txt --latest"
    start = workflow.index(first)
    end = workflow.index(last, start)+len(last)
    fragment = '\n'.join(line[10:] for line in workflow[start:end].splitlines())
    sentinel = folder/'release-attempted'
    fake = 'gh() { if [[ "$1" = api ]]; then printf "{broken"; return 42; fi; printf attempted > "$RUNNER_TEMP/release-attempted"; }; export -f gh; '
    result = subprocess.run(['bash', '-euo', 'pipefail', '-c', fake+fragment], cwd=root,
                            env={**os.environ, 'GITHUB_REPOSITORY': repo, 'GITHUB_SHA': sha, 'RUNNER_TEMP': str(folder)},
                            capture_output=True, text=True)
    assert result.returncode == 42 and not sentinel.exists(), 'API failure reached publication'
    count += 1
print(f'PASS: {count} exact-SHA, fixed-repository, branch/workflow, live-main drift/API failure and malformed-input gate probes.')
