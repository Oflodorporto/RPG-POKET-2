#!/usr/bin/env python3
"""Fail-closed future publisher prerequisite; reading evidence never publishes."""
import json
import os
from pathlib import Path
import re
import subprocess
import sys

REPOSITORY = 'Oflodorporto/RPG-POKET-2'
BRANCH = 'feat/package11-release'
WORKFLOW = '.github/workflows/package11-check.yml'
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


def main():
    assert len(sys.argv) == 2, 'Provide exactly one qualification response file'
    sha = os.environ['GITHUB_SHA']
    assert os.environ['GITHUB_EVENT_NAME'] == 'push', 'Release only accepts main push events'
    assert os.environ['GITHUB_WORKFLOW_REF'] == RELEASE_WORKFLOW_REF, 'Unexpected publisher repository/workflow/ref'
    assert subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip() == sha, 'Checkout is not the qualified SHA'
    response = json.loads(Path(sys.argv[1]).read_text())
    assert isinstance(response, dict), 'Invalid qualification response'
    assert qualified(response.get('workflow_runs'), sha, os.environ['GITHUB_REF'], os.environ['GITHUB_REPOSITORY']), 'No successful exact-SHA qualification on required repository, branch and workflow'
    print('PASS: main push, exact checkout SHA, fixed repository/publisher, completed qualification push, exact branch and workflow.')


if __name__ == '__main__':
    main()
