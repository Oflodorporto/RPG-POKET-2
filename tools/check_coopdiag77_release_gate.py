#!/usr/bin/env python3
"""Fail-closed future publisher prerequisite; reading evidence never publishes."""
import json
import os
from pathlib import Path
import re
import subprocess
import sys

REPOSITORY = 'Oflodorporto/RPG-POKET-2'
BRANCH = 'feat/coopdiag77-release'
WORKFLOW = '.github/workflows/coopdiag77-check.yml'
RELEASE_WORKFLOW_REF = REPOSITORY + '/.github/workflows/release.yml@refs/heads/main'


def qualified(runs, sha, ref, repository):
    """Only a completed same-repository push on the explicit branch can qualify."""
    if repository != REPOSITORY or ref != 'refs/heads/main':
        return False
    if not isinstance(sha, str) or re.fullmatch(r'[0-9a-f]{40}', sha) is None:
        return False
    if not isinstance(runs, list):
        return False
    return any(isinstance(run, dict)
               and run.get('head_sha') == sha
               and run.get('head_branch') == BRANCH
               and run.get('path') == WORKFLOW
               and run.get('conclusion') == 'success'
               and run.get('status') == 'completed'
               and run.get('event') == 'push'
               and isinstance(run.get('head_repository'), dict)
               and run['head_repository'].get('full_name') == REPOSITORY
               for run in runs)


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
    assert len(sys.argv) == 2 or (len(sys.argv) == 4 and sys.argv[2] == '--main-ref'), 'Provide qualification response and optional --main-ref response file'
    sha = os.environ['GITHUB_SHA']
    assert os.environ['GITHUB_EVENT_NAME'] == 'push', 'Release only accepts main push events'
    assert os.environ['GITHUB_WORKFLOW_REF'] == RELEASE_WORKFLOW_REF, 'Unexpected publisher repository/workflow/ref'
    assert os.environ['GITHUB_WORKFLOW_SHA'] == sha, 'Publisher workflow is not from the qualified SHA'
    assert subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip() == sha, 'Checkout is not the qualified SHA'
    response = json.loads(Path(sys.argv[1]).read_text())
    assert isinstance(response, dict), 'Invalid qualification response'
    assert qualified(response.get('workflow_runs'), sha, os.environ['GITHUB_REF'], os.environ['GITHUB_REPOSITORY']), 'No successful exact-SHA qualification on required repository, branch and workflow'
    if len(sys.argv) == 4:
        main_ref = json.loads(Path(sys.argv[3]).read_text())
        assert live_main_matches(main_ref, sha), 'Live main ref is malformed or has advanced beyond the qualified checkout SHA'
    print('PASS: main push, exact checkout SHA, fixed repository/publisher, completed qualification push, exact branch and workflow'+('; live main preflight matches' if len(sys.argv) == 4 else '')+'.')


if __name__ == '__main__':
    main()
