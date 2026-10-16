#!/usr/bin/env python3
"""PROPOSAL ONLY: explicit, hash-pinned benchmark adapters; no CI or publication.

Materialization requires a genuine source checkout and genuine Actions context.
Nothing here changes GITHUB_* or edits the source checkout. This is not a
publisher authorization mechanism or an independently authenticated attestation.
"""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys

REPOSITORY = 'Oflodorporto/RPG-POKET-2'
SOURCE = {'repository': REPOSITORY,
          'commit': '6026fb49db4fe874b9c3f0571cbe4fdb9e66adb3',
          'tree': '5ad62cddfe052bea09bf99c29989731722088981',
          'parent': 'c8738eb67bde3817feaab9f3a53c30607396a5f5'}
WORKFLOW = '.github/workflows/ci-acceleration-benchmark.yml'
BENCHMARK_REF = 'refs/heads/bench/coopdiag77-cache-promotion'
ADAPTER = 'proposals/ci-acceleration/benchmark_adapter.py'
PINNED = {
    'tools/check_coopdiag77_scope.py': 'f5c84a13400a634b2948d3afd53dc685e795267116ca0ef5f108c3eef6213fc4',
    'tools/record_coopdiag77_identity.py': 'e6f5d72aea4857b74196ab870b68f5cb64959a24db3a3583ed6d33b43edb0533',
    'tools/test_coopdiag77.sh': '5bc9276b407eee326a880336ba6bffb9891d2e25204f6d5e5b118f0f4dcbf12f',
}
GENERATED = {'release/firmware.bin', 'release/artes.pak', 'release/manifest.json', 'release/SHA256SUMS'}
# Last preflight line from the hash-frozen tests/coopdiag77_release_gate.py.
AGGREGATE_START = ('PASS: 113 exact-SHA, fixed-repository, branch/workflow, '
                   'live-main drift/API failure and malformed-input gate probes.')


class Rejected(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise Rejected(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def exact(value, keys, label):
    require(type(value) is dict and set(value) == set(keys), label + ': unexpected fields')


def json_read(path):
    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, 'Duplicate JSON key: ' + key)
            result[key] = value
        return result
    return json.loads(path.read_text(), object_pairs_hook=pairs)


def validate_aggregate_transcript(log, commands, summary):
    """Read only the real runner protocol, not earlier mocked checkpoint probes.

    The frozen shell wrapper runs release-gate probes immediately before the
    aggregate. Its child commands redirect output to individual checkpoint logs;
    successful outer stdout is therefore RUN/PASSED pairs and the exact report
    serialized to summary.json. No last-RUN search or task-name exclusion is used.
    Callers still validate source/plan/checkpoint hashes and fresh workspaces.
    """
    require(type(summary.get('tasks')) is int and summary['tasks'] == 157 and
            len(commands) == 157 and 'preparation' in commands, 'Expected 157 aggregate groups')
    lines = log.splitlines()
    # Match the exact serialization emitted from the same frozen report object.
    finish = json.dumps(summary)
    require(lines.count(AGGREGATE_START) == 1 and lines.count(finish) == 1,
            'Missing or duplicate aggregate transcript boundary')
    start, end = lines.index(AGGREGATE_START), lines.index(finish)
    require(start < end, 'Aggregate transcript boundaries out of order')
    progress = lines[start + 1:end]
    require(len(progress) == 314, 'All 157 fresh RUN/PASSED pairs required')
    require(progress[:2] == ['RUN preparation', 'PASSED preparation'],
            'Preparation must finish before aggregate task groups')
    state = dict.fromkeys(commands)
    for line in progress:
        status, _, name = line.partition(' ')
        require(status in ('RUN', 'PASSED') and name in state,
                'Unexpected aggregate progress line: ' + line[:120])
        require(state[name] is None if status == 'RUN' else state[name] == 'RUN',
                'Duplicate or out-of-order aggregate progress: ' + name)
        state[name] = status
    require(all(status == 'PASSED' for status in state.values()),
            'All 157 fresh aggregate completions required')


def git(root, *args):
    return subprocess.check_output(['git', '-C', str(root), *args], text=True).strip()


def validate_context(context, environment):
    """Check identity without changing or impersonating the Actions environment."""
    exact(context, ('schema', 'source', 'orchestrator', 'run', 'benchmark'), 'Context')
    require(type(context['schema']) is int and context['schema'] == 1, 'Wrong schema')
    require(context['source'] == SOURCE, 'Source commit/tree/parent not approved')
    o, r, b = context['orchestrator'], context['run'], context['benchmark']
    exact(o, ('repository', 'repository_id', 'commit', 'tree', 'ref', 'workflow_ref', 'workflow_sha', 'workflow_sha256'), 'Orchestrator')
    exact(r, ('event', 'id', 'attempt'), 'Run')
    exact(b, ('comparison_id', 'phase', 'aggregate_run_id'), 'Benchmark')
    require(o['repository'] == REPOSITORY, 'Wrong repository')
    for field in ('commit', 'tree', 'workflow_sha'):
        require(type(o[field]) is str and re.fullmatch('[0-9a-f]{40}', o[field]), 'Invalid ' + field)
    require(type(o['workflow_sha256']) is str and re.fullmatch('[0-9a-f]{64}', o['workflow_sha256']), 'Invalid workflow digest')
    require(o['commit'] == o['workflow_sha'] and o['commit'] != SOURCE['commit'], 'Orchestrator workflow/source separation required')
    require(o['ref'] == BENCHMARK_REF, 'Only the exact reviewed benchmark branch is accepted')
    require(o['workflow_ref'] == REPOSITORY + '/' + WORKFLOW + '@' + o['ref'], 'Unexpected workflow identity')
    require(r['event'] == 'push', 'This proposal accepts only benchmark branch push events')
    for value in (o['repository_id'], r['id'], r['attempt']):
        require(type(value) is str and re.fullmatch('[1-9][0-9]*', value), 'IDs must be canonical positive strings')
    require(type(b['comparison_id']) is str and re.fullmatch('[a-zA-Z0-9_.-]{1,96}', b['comparison_id']), 'Invalid comparison ID')
    require(b['phase'] in ('cold', 'warm'), 'Invalid phase')
    require(b['aggregate_run_id'] == 'ci-' + o['commit'], 'Aggregate label must name real orchestrator SHA')
    expected = {'GITHUB_ACTIONS': 'true', 'GITHUB_REPOSITORY': o['repository'],
                'GITHUB_REPOSITORY_ID': o['repository_id'], 'GITHUB_SHA': o['commit'],
                'GITHUB_REF': o['ref'], 'GITHUB_WORKFLOW_REF': o['workflow_ref'],
                'GITHUB_WORKFLOW_SHA': o['workflow_sha'], 'GITHUB_EVENT_NAME': r['event'],
                'GITHUB_RUN_ID': r['id'], 'GITHUB_RUN_ATTEMPT': r['attempt'],
                'RPG_DETECT_LEAKS': '1', 'PYTHONDONTWRITEBYTECODE': '1'}
    for name, value in expected.items():
        require(environment.get(name) == value, 'Real environment mismatch: ' + name)


def validate_roots(source, orchestrator, output):
    roots = [Path(p).absolute() for p in (source, orchestrator, output)]
    for path in roots:
        require(path == path.resolve(), 'Symlink or noncanonical workspace path')
    for i, left in enumerate(roots):
        for right in roots[i + 1:]:
            require(left != right and left not in right.parents and right not in left.parents,
                    'Source, orchestrator and output must be disjoint sibling workspaces')
    return roots


def load_originals(source):
    originals = {}
    for name, expected in PINNED.items():
        path = source / name
        require(path.is_file() and not path.is_symlink(), 'Missing/nonregular original: ' + name)
        data = path.read_bytes()
        require(digest(data) == expected, 'Unapproved original bytes: ' + name)
        originals[name] = data.decode('utf-8')
    return originals


def validate_checkouts(context, source, orchestrator, git_reader=git):
    require(git_reader(source, 'rev-parse', 'HEAD') == SOURCE['commit'], 'Source HEAD differs from pin')
    require(git_reader(source, 'rev-parse', 'HEAD^{tree}') == SOURCE['tree'], 'Source tree differs from pin')
    require(git_reader(source, 'rev-list', '--parents', '-n', '1', 'HEAD').split() == [SOURCE['commit'], SOURCE['parent']], 'Source must be sole child of exact baseline')
    source_status = git_reader(source, 'status', '--porcelain', '--untracked-files=all')
    require(all(line.startswith('?? ') and line[3:] in GENERATED for line in source_status.splitlines()), 'Changed or unexpected source paths')
    o = context['orchestrator']
    require(git_reader(orchestrator, 'rev-parse', 'HEAD') == o['commit'], 'Orchestrator HEAD mismatch')
    require(git_reader(orchestrator, 'rev-parse', 'HEAD^{tree}') == o['tree'], 'Orchestrator tree mismatch')
    require(not git_reader(orchestrator, 'status', '--porcelain', '--untracked-files=all'), 'Orchestrator checkout must be clean')
    workflow = orchestrator / WORKFLOW
    require(workflow.is_file() and not workflow.is_symlink(), 'Missing benchmark workflow')
    require(digest(workflow.read_bytes()) == o['workflow_sha256'], 'Workflow bytes differ from pin')


def replace_exact(text, before, after, count=1):
    require(text.count(before) == count, 'Patch anchor count changed: ' + before[:90])
    return text.replace(before, after)


def render(originals, source, output):
    """The only permitted source-derived edits; complete diffs accompany output."""
    bootstrap = ('import sys as _benchmark_sys\n'
                 '_benchmark_sys.path.insert(0, ' + repr(str(source / 'tools')) + ')\n'
                 'from benchmark_runtime import runtime_guard as _benchmark_guard\n'
                 '_benchmark = _benchmark_guard(__file__)\n')
    scope = originals['tools/check_coopdiag77_scope.py']
    scope = replace_exact(scope, 'from pathlib import Path\n', bootstrap + 'from pathlib import Path\n')
    scope = replace_exact(scope, 'ROOT=Path(__file__).resolve().parents[1]', 'ROOT=Path(_benchmark["paths"]["source"])')
    scope = replace_exact(scope, "os.environ.get('GITHUB_ACTIONS')=='true',os.environ.get('GITHUB_SHA'))", "os.environ.get('GITHUB_ACTIONS')=='true',_benchmark['context']['source']['commit'])")
    identity = originals['tools/record_coopdiag77_identity.py']
    identity = replace_exact(identity, 'from pathlib import Path\n', bootstrap + 'from pathlib import Path\n')
    old = "    assert git('rev-parse','HEAD') == os.environ['GITHUB_SHA']\n    assert os.environ['GITHUB_REF'] in ('refs/heads/feat/coopdiag77-release','refs/heads/main')\n    assert os.environ['GITHUB_EVENT_NAME'] == 'push' and os.environ['RPG_DETECT_LEAKS'] == '1'"
    new = "    assert git('rev-parse','HEAD') == _benchmark['context']['source']['commit']\n    assert os.environ['RPG_DETECT_LEAKS'] == '1'"
    identity = replace_exact(identity, old, new)
    identity = replace_exact(identity, "branch=os.environ['GITHUB_REF']", "orchestrator_ref=os.environ['GITHUB_REF']")
    old = "    Path('build/coopdiag77-source-identity.json').write_text(json.dumps(report,indent=2)+'\\n')"
    new = "    report.update(benchmark_only=True,cannot_publish=True,published=False,benchmark_context=_benchmark['context'],adapter_manifest_sha256=_benchmark['manifest_sha256'])\n    Path('build/coopdiag77-benchmark-identity.json').write_text(json.dumps(report,indent=2)+'\\n')"
    identity = replace_exact(identity, old, new)
    wrapper = replace_exact(originals['tools/test_coopdiag77.sh'], 'python3 tools/check_coopdiag77_scope.py', 'python3 ' + shlex.quote(str(output / 'check_scope.py')), count=2)
    return {'check_scope.py': scope, 'record_identity.py': identity, 'test_coopdiag77.sh': wrapper}


def package(source, orchestrator, output, context):
    originals = load_originals(source)
    rendered = render(originals, source, output)
    names = {'check_scope.py': 'tools/check_coopdiag77_scope.py', 'record_identity.py': 'tools/record_coopdiag77_identity.py', 'test_coopdiag77.sh': 'tools/test_coopdiag77.sh'}
    patches = {name: ''.join(difflib.unified_diff(originals[old].splitlines(True), text.splitlines(True), fromfile=old, tofile=name)) for name, text in rendered.items() for old in (names[name],)}
    runtime_path = orchestrator / ADAPTER
    require(runtime_path.is_file() and not runtime_path.is_symlink(), 'Missing reviewed adapter')
    rendered['benchmark_runtime.py'] = runtime_path.read_text()
    manifest = {'schema': 1, 'benchmark_only': True, 'cannot_publish': True, 'published': False,
                'context': context, 'paths': {'source': str(source), 'orchestrator': str(orchestrator), 'output': str(output)},
                'original_sha256': PINNED, 'generated_sha256': {name: digest(text.encode()) for name, text in rendered.items()},
                'complete_patches': patches}
    return rendered, manifest


def materialize(source, orchestrator, output, context, environment=None):
    source, orchestrator, output = validate_roots(source, orchestrator, output)
    validate_context(context, os.environ if environment is None else environment)
    validate_checkouts(context, source, orchestrator)
    require(not (source / 'build').exists() and not (source / 'build').is_symlink(), 'Fresh benchmark requires no source/build or prior checkpoints')
    require(not any((source / name).exists() or (source / name).is_symlink() for name in GENERATED), 'Fresh benchmark requires no generated release files')
    require(not output.exists(), 'Output must be a new directory; no overwrite/reuse')
    rendered, manifest = package(source, orchestrator, output, context)
    output.mkdir(parents=True)
    for name, text in rendered.items():
        (output / name).write_text(text)
    (output / 'adapter-manifest.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    verify_materialized(output, os.environ if environment is None else environment)
    return manifest


def verify_materialized(output, environment=None):
    output = Path(output).resolve()
    manifest_path = output / 'adapter-manifest.json'
    require(manifest_path.is_file() and not manifest_path.is_symlink(), 'Missing adapter manifest')
    manifest = json_read(manifest_path)
    exact(manifest, ('schema', 'benchmark_only', 'cannot_publish', 'published', 'context', 'paths', 'original_sha256', 'generated_sha256', 'complete_patches'), 'Manifest')
    exact(manifest['paths'], ('source', 'orchestrator', 'output'), 'Workspace paths')
    source, orchestrator, claimed_output = validate_roots(*(manifest['paths'][key] for key in ('source', 'orchestrator', 'output')))
    require(output == claimed_output, 'Materialization relocated')
    validate_context(manifest['context'], os.environ if environment is None else environment)
    validate_checkouts(manifest['context'], source, orchestrator)
    rendered, expected = package(source, orchestrator, output, manifest['context'])
    require(json.dumps(manifest, sort_keys=True) == json.dumps(expected, sort_keys=True), 'Unapproved patch or manifest change')
    require({p.name for p in output.iterdir()} == set(rendered) | {'adapter-manifest.json'}, 'Unexpected adapter files')
    for name, text in rendered.items():
        path = output / name
        require(path.is_file() and not path.is_symlink() and path.read_bytes() == text.encode(), 'Unapproved generated bytes: ' + name)
    return manifest


def runtime_guard(script):
    require(sys.flags.optimize == 0, 'Original gates require Python assertions; optimized Python is forbidden')
    manifest = verify_materialized(Path(script).parent)
    require(Path.cwd().resolve() == Path(manifest['paths']['source']), 'Gates must run in exact source checkout')
    return {**manifest, 'manifest_sha256': digest((Path(script).parent / 'adapter-manifest.json').read_bytes())}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, required=True)
    parser.add_argument('--orchestrator-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--context', type=Path, required=True)
    args = parser.parse_args()
    materialize(args.source_root, args.orchestrator_root, args.output, json_read(args.context))
    print('Prepared explicit benchmark adapters only; no qualification, CI or publication executed.')


if __name__ == '__main__':
    main()
