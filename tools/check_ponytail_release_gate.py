#!/usr/bin/env python3
"""Fail-closed future publisher prerequisite; reading evidence never publishes."""
if not __debug__:
    raise RuntimeError("Qualification requires assertions; optimized Python is forbidden")
import json
import os
import argparse
from pathlib import Path
import re
import subprocess
import sys

REPOSITORY = 'Oflodorporto/RPG-POKET-2'
BRANCH = 'feat/ponytail-release'
WORKFLOW = '.github/workflows/wifitest77-check.yml'
RELEASE_WORKFLOW_REF = REPOSITORY + '/.github/workflows/release.yml@refs/heads/main'


def select_qualified_run(runs, sha, ref, repository, required_id=None):
    """Select a completed exact-SHA push with a concrete Actions run ID."""
    if repository != REPOSITORY or ref != 'refs/heads/main':
        return None
    if not isinstance(sha, str) or re.fullmatch(r'[0-9a-f]{40}', sha) is None:
        return None
    if not isinstance(runs, list):
        return None
    matches = [run for run in runs if isinstance(run, dict)
               and type(run.get('id')) is int and run['id'] > 0
               and (required_id is None or run['id'] == required_id)
               and run.get('head_sha') == sha
               and run.get('head_branch') == BRANCH
               and run.get('path') == WORKFLOW
               and run.get('conclusion') == 'success'
               and run.get('status') == 'completed'
               and run.get('event') == 'push'
               and isinstance(run.get('head_repository'), dict)
               and run['head_repository'].get('full_name') == REPOSITORY]
    return max(matches, key=lambda run: run['id']) if matches else None


def qualified(runs, sha, ref, repository):
    return select_qualified_run(runs, sha, ref, repository) is not None


def read_run_id(path):
    raw = Path(path).read_text()
    assert re.fullmatch(r'[1-9][0-9]*\n', raw), 'Invalid selected run ID file'
    return int(raw)


def live_main_matches(response, sha):
    """Validate the freshly fetched main ref; this is a preflight, not a lock."""
    return (isinstance(sha, str)
            and re.fullmatch(r'[0-9a-f]{40}', sha) is not None
            and isinstance(response, dict)
            and response.get('ref') == 'refs/heads/main'
            and isinstance(response.get('object'), dict)
            and response['object'].get('type') == 'commit'
            and response['object'].get('sha') == sha)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('qualification')
    parser.add_argument('--main-ref')
    parser.add_argument('--run-id-out')
    parser.add_argument('--run-id-file')
    args = parser.parse_args()
    assert not (args.run_id_out and args.run_id_file), 'Run ID output and input are exclusive'
    sha = os.environ['GITHUB_SHA']
    assert os.environ['GITHUB_EVENT_NAME'] == 'push', 'Release only accepts main push events'
    assert os.environ['GITHUB_WORKFLOW_REF'] == RELEASE_WORKFLOW_REF, 'Unexpected publisher repository/workflow/ref'
    assert os.environ['GITHUB_WORKFLOW_SHA'] == sha, 'Publisher workflow is not from the qualified SHA'
    assert subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip() == sha, 'Checkout is not the qualified SHA'
    response = json.loads(Path(args.qualification).read_text())
    assert isinstance(response, dict), 'Invalid qualification response'
    required_id = read_run_id(args.run_id_file) if args.run_id_file else None
    selected = select_qualified_run(response.get('workflow_runs'), sha, os.environ['GITHUB_REF'], os.environ['GITHUB_REPOSITORY'], required_id)
    assert selected is not None, 'No successful exact-SHA qualification on required repository, branch, workflow and run ID'
    if args.main_ref:
        main_ref = json.loads(Path(args.main_ref).read_text())
        assert live_main_matches(main_ref, sha), 'Live main ref is malformed or has advanced beyond the qualified checkout SHA'
    if args.run_id_out:
        Path(args.run_id_out).write_text(str(selected['id'])+'\n')
    print('PASS: main push, exact checkout SHA, fixed repository/publisher, completed qualification push, exact branch, workflow and run ID'+('; live main preflight matches' if args.main_ref else '')+'.')


if __name__ == '__main__':
    main()
