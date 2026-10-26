#!/usr/bin/env python3
"""Bind publisher bytes to the successful exact-SHA qualification artifact."""
if not __debug__:
    raise RuntimeError('Qualification requires assertions; optimized Python is forbidden')
import argparse
import hashlib
import json
from pathlib import Path
import re

from check_ponytail_release_gate import read_run_id
from verify_ponytail_release import verify
from record_ponytail_identity import check_image

REPOSITORY = 'Oflodorporto/RPG-POKET-2'
NAMES = ('firmware.bin', 'artes.pak', 'manifest.json')


def validate_metadata(response, run_id, sha):
    assert type(run_id) is int and run_id > 0
    assert isinstance(sha, str) and re.fullmatch('[0-9a-f]{40}', sha)
    assert isinstance(response, dict) and type(response.get('total_count')) is int
    artifacts = response.get('artifacts')
    assert isinstance(artifacts, list) and response['total_count'] == len(artifacts), 'Incomplete artifact listing'
    matching = [a for a in artifacts if isinstance(a, dict) and a.get('name') == 'ponytail-'+sha]
    assert len(matching) == 1, 'Exact qualification artifact missing or ambiguous'
    artifact = matching[0]
    assert type(artifact.get('id')) is int and artifact['id'] > 0
    assert type(artifact.get('size_in_bytes')) is int and artifact['size_in_bytes'] > 0
    assert artifact.get('expired') is False
    workflow = artifact.get('workflow_run')
    assert isinstance(workflow, dict) and type(workflow.get('id')) is int
    assert workflow['id'] == run_id and workflow.get('head_sha') == sha
    assert artifact.get('archive_download_url') == (f'https://api.github.com/repos/{REPOSITORY}/actions/artifacts/{artifact["id"]}/zip')
    return artifact['id']


def safe_file(root, relative):
    path = root/relative
    assert path.is_file() and not path.is_symlink(), relative
    assert path.resolve().is_relative_to(root.resolve()), relative
    assert not any(parent.is_symlink() for parent in path.parents if parent != root and root in parent.parents), relative
    return path.read_bytes()


def compare_package(artifact_root, local_release, sha):
    assert isinstance(sha, str) and re.fullmatch('[0-9a-f]{40}', sha)
    assert artifact_root.is_dir() and not artifact_root.is_symlink()
    assert local_release.is_dir() and not local_release.is_symlink()
    compared = {}
    for name in NAMES:
        qualified = safe_file(artifact_root, 'release/'+name)
        local = safe_file(local_release, name)
        assert qualified == local, 'Publisher bytes differ from qualification artifact: '+name
        compared[name] = {'bytes':len(qualified),'sha256':hashlib.sha256(qualified).hexdigest()}
    identity = json.loads(safe_file(artifact_root,'build/ponytail-source-identity.json'))
    budget = json.loads(safe_file(artifact_root,'build/ponytail-scope-budget.json'))
    assert identity['source_commit'] == sha and identity['artifacts'] == compared, 'Qualified source/byte identity mismatch'
    assert identity['published'] is False and identity['physical_validation'] is False
    assert budget['source_commit'] == sha and budget['candidate_firmware_bytes'] == compared['firmware.bin']['bytes']
    assert budget['target_compile_verified'] is True and budget['art_sha256'] == compared['artes.pak']['sha256']
    sums = safe_file(artifact_root,'release/SHA256SUMS').decode('ascii').splitlines()
    assert sums == [compared[name]['sha256']+'  '+name for name in NAMES], 'Qualified SHA256SUMS mismatch'
    return compared


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('metadata',type=Path)
    parser.add_argument('run_id_file',type=Path)
    parser.add_argument('artifact_root',type=Path)
    parser.add_argument('local_release',type=Path)
    parser.add_argument('sha')
    args = parser.parse_args()
    run_id = read_run_id(args.run_id_file)
    metadata = json.loads(args.metadata.read_text())
    validate_metadata(metadata,run_id,args.sha)
    compared = compare_package(args.artifact_root,args.local_release,args.sha)
    verify(args.artifact_root/'release',quiet=True)
    verify(args.local_release,quiet=True)
    check_image(safe_file(args.artifact_root,'release/firmware.bin'))
    check_image(safe_file(args.local_release,'firmware.bin'))
    print(f'PASS: qualified run {run_id}, exact-SHA immutable artifact, image/manifest/art integrity and byte-identical publisher triple')


if __name__ == '__main__': main()
