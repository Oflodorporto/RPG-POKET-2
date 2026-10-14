#!/usr/bin/env python3
"""Fail-closed, same-repository, exact-SHA prerequisite; never publishes."""
import json
import os
import re
import subprocess
import sys


def qualified(runs, sha, ref, repository):
    if ref != 'refs/heads/main' or not isinstance(sha, str) or not re.fullmatch('[0-9a-f]{40}', sha):
        return False
    if not isinstance(repository, str) or not re.fullmatch(r'[^/\s]+/[^/\s]+', repository):
        return False
    if not isinstance(runs, list):
        return False
    return any(isinstance(r, dict) and r.get('head_sha') == sha
               and r.get('head_branch') == 'feat/package9-release'
               and r.get('path') == '.github/workflows/package9-check.yml'
               and r.get('conclusion') == 'success' and r.get('status') == 'completed'
               and r.get('event') == 'push'
               and isinstance(r.get('head_repository'), dict)
               and r['head_repository'].get('full_name') == repository for r in runs)


def main():
    sha = os.environ['GITHUB_SHA']
    assert os.environ['GITHUB_EVENT_NAME'] == 'push', 'Release only accepts main push events'
    assert subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip() == sha
    with open(sys.argv[1]) as source:
        runs = json.load(source)['workflow_runs']
    assert qualified(runs, sha, os.environ['GITHUB_REF'], os.environ['GITHUB_REPOSITORY']), 'No successful exact-SHA qualification on required repository, branch and workflow'
    print('PASS: main push, exact source SHA, same repository, successful completed qualification push, exact workflow and branch.')


if __name__ == '__main__':
    main()
