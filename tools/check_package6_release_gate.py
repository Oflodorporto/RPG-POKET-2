#!/usr/bin/env python3
"""Pure exact-commit gate. Calling this script never publishes anything."""
import json, os, re, subprocess, sys

def qualified(runs, sha, ref, repository=None):
    if ref != 'refs/heads/main' or not re.fullmatch('[0-9a-f]{40}',sha):return False
    return any(r.get('head_sha')==sha and r.get('head_branch')=='feat/package6-release'
       and r.get('path')=='.github/workflows/package6-check.yml' and r.get('conclusion')=='success'
       and r.get('status')=='completed' and r.get('event') in ('push','workflow_dispatch')
       and (repository is None or r.get('head_repository',{}).get('full_name')==repository)
       for r in runs)

def main():
    sha=os.environ['GITHUB_SHA'];assert subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()==sha
    runs=json.load(open(sys.argv[1]))['workflow_runs']
    assert qualified(runs,sha,os.environ['GITHUB_REF'],os.environ['GITHUB_REPOSITORY']), 'No successful same-repository exact-SHA package6 qualification on the required branch'
    print('PASS: exact SHA, source repository, workflow, branch and main-only gate.')
if __name__=='__main__':main()
