#!/usr/bin/env python3
"""Run the reviewed, non-publishing coopdiag77 cold/warm experiment.

Only genuine clean Git checkouts and a real benchmark-branch Actions context
are accepted. No product gate executes during the offline fixture tests.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import time

import benchmark_adapter as adapter
import cache_policy
import compare_benchmark as comparison

HERE = Path(__file__).resolve().parent
RUNNER_REL = 'proposals/ci-acceleration/run_benchmark.py'
ORIGINAL_WORKFLOW = '.github/workflows/coopdiag77-check.yml'
LOCK_REL = 'dependencies/downloads.lock.json'
PIN_REL = 'dependencies/PIN.json'
SALT = 'benchmark-coopdiag77-downloads-v1'
LOCK_CONTEXT = {
    'runner_os': 'Linux', 'runner_arch': 'X64', 'python_version': '3.12.3',
    'arduino_cli_version': '1.5.2-rc.1', 'esp32_core_version': '3.3.12',
    'pillow_version': '11.3.0', 'pyyaml_version': '6.0.3',
    'fqbn': comparison.FQBN, 'benchmark_salt': SALT,
}
FINAL_COMMANDS = (
    ('python3', 'tools/release_coopdiag77_manifest.py'),
    ('python3', 'tools/verify_coopdiag77_release.py'),
    ('python3', 'tests/coopdiag77_release_verifier.py'),
    ('python3', 'tools/test_coopdiag77_release.py', 'release'),
    ('python3', 'tools/test_coopdiag77_release.py', 'release', '--sanitizers'),
)
PARTIAL_PHASES = ('dependency_install', 'dependency_verify', 'aggregate',
                  'esp32_on', 'esp32_off', 'final_bytes_ota', 'identity')


class Rejected(ValueError):
    pass


class CommandFailed(Rejected):
    def __init__(self, label, returncode):
        super().__init__(f'{label} exited {returncode}; inspect evidence/logs')
        self.label, self.returncode = label, returncode


def require(condition, message):
    if not condition:
        raise Rejected(message)


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def read_json(path):
    return adapter.json_read(Path(path))


def write_json(path, value):
    path = Path(path)
    temporary = path.with_suffix(path.suffix + '.tmp')
    require(not temporary.exists() and not temporary.is_symlink(), 'Unexpected temporary JSON file')
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')
    temporary.replace(path)


def regular(path):
    path = Path(path)
    require(path.is_file() and not path.is_symlink(), 'Missing/nonregular file: ' + str(path))
    return path


def absolute(path):
    path = Path(path)
    require(path.is_absolute() and '..' not in path.parts and path == path.resolve(),
            'Absolute canonical path without symlinks required: ' + str(path))
    require('\n' not in str(path) and '\r' not in str(path), 'Newline in path')
    return path


def absent(path):
    require(not path.exists() and not path.is_symlink(), 'Fresh path required; no resume: ' + str(path))


def require_runtime(environment):
    require(sys.flags.optimize == 0, 'Python assertions required; optimized runtime is forbidden')
    require(environment.get('PYTHONOPTIMIZE', '') in ('', '0'), 'PYTHONOPTIMIZE disables product assertions')
    require(platform.python_version() == LOCK_CONTEXT['python_version'], 'Python must be exactly 3.12.3')
    for key, expected in (('RUNNER_OS', 'Linux'), ('RUNNER_ARCH', 'X64'), ('ImageOS', 'ubuntu24')):
        require(environment.get(key) == expected, 'Unexpected runner platform: ' + key)
    require(re.fullmatch(r'20\d{6}\.\d+\.\d+', environment.get('ImageVersion', '')), 'Exact ImageVersion is required')
    compiler = subprocess.check_output(['g++', '--version'], text=True, env=environment).splitlines()
    require(compiler and compiler[0] == comparison.HOST_COMPILER,
            'Host compiler changed: stop before downloads or qualification and review a new baseline')


def auxiliary_paths(environment, source, orchestrator, work):
    temp = absolute(environment.get('RUNNER_TEMP', ''))
    require(temp.is_dir(), 'RUNNER_TEMP must exist')
    result = {'cache_root': temp / 'download-cache', 'dependency_work': temp / 'dependency-work',
              'build_root': temp / 'esp32-coopdiag77'}
    paths = list(result.values())
    for candidate in paths:
        absolute(candidate)
        for root in (source, orchestrator, work):
            require(candidate != root and root not in candidate.parents and candidate not in root.parents,
                    'Dependency/build workspaces must be disjoint from checkouts and work')
    return result


def source_contract(source, orchestrator):
    base = orchestrator / 'proposals/ci-acceleration'
    contract = read_json(regular(base / 'source-contract.json'))
    require(contract['source_commit'] == adapter.SOURCE['commit'] and
            contract['source_tree'] == adapter.SOURCE['tree'] and
            contract['source_parent'] == adapter.SOURCE['parent'] and
            contract['group_count'] == 157 and contract['may_publish'] is False,
            'Unapproved source contract')
    plan_path = regular(base / 'reviewed-plan.json')
    require(digest(plan_path) == comparison.PLAN_SHA256 == contract['reviewed_plan_sha256'],
            'Reviewed command plan digest changed')
    plan = read_json(plan_path)
    require(plan['group_count'] == 157 and len(plan['tasks']) == 156 and
            plan['focused'] is False and plan['assertions'] is True and
            plan['detect_leaks'] == 1 and type(plan['detect_leaks']) is int and
            plan['ubsan_halt_on_error'] is True, 'Reviewed coverage contract changed')
    for name, expected in contract['original_scripts_sha256'].items():
        require(digest(regular(source / name)) == expected, 'Original script changed: ' + name)
    workflow = regular(source / ORIGINAL_WORKFLOW).read_text()
    validate_recipe(workflow)
    return contract, plan, workflow


def validate_recipe(workflow):
    """A pinned literal recipe avoids introducing YAML before dependencies exist."""
    require(workflow.count('bash tools/test_coopdiag77.sh 2>&1 | tee build/coopdiag77-tests.log') == 1,
            'Original aggregate invocation changed')
    for command in FINAL_COMMANDS:
        require(workflow.count('          ' + ' '.join(command) + '\n') == 1,
                'Original final-byte recipe changed')
    for anchor in (
        'for flag in 1 0; do', "--build-property 'compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=0'",
        'arduino-cli compile --verbose --jobs 4', "--fqbn '" + comparison.FQBN + "'",
        '--build-path "$RUNNER_TEMP/esp32-coopdiag77/flag-$flag"',
        '--export-binaries=false firmware/RPG_POKET_2',
        'cp "$RUNNER_TEMP/esp32-coopdiag77/flag-1/RPG_POKET_2.ino.bin" build/RPG_POKET_2.ino.bin',
        'python3 tools/record_coopdiag77_identity.py --build-root "$RUNNER_TEMP/esp32-coopdiag77"',
    ):
        require(workflow.count(anchor) == 1, 'Original ESP32/identity recipe changed: ' + anchor)
    upload_patterns(workflow)


def upload_patterns(workflow):
    match = re.search(r'^          path: \|\n((?:            [^\n]+\n)+)', workflow, re.M)
    require(match is not None and len(re.findall(r'^          path: \|$', workflow, re.M)) == 1,
            'Expected one exact artifact path block')
    patterns = tuple(line[12:] for line in match[1].splitlines())
    require(len(patterns) == len(set(patterns)) and len(patterns) > 50, 'Incomplete artifact inventory')
    for name in patterns:
        require(name.startswith(('release/', 'build/')) and not Path(name).is_absolute() and
                '..' not in Path(name).parts, 'Unsafe artifact pattern')
    return patterns


def make_context(source, orchestrator, phase, environment):
    workflow = regular(orchestrator / adapter.WORKFLOW)
    return {
        'schema': 1, 'source': dict(adapter.SOURCE),
        'orchestrator': {
            'repository': environment.get('GITHUB_REPOSITORY'),
            'repository_id': environment.get('GITHUB_REPOSITORY_ID'),
            'commit': environment.get('GITHUB_SHA'),
            'tree': adapter.git(orchestrator, 'rev-parse', 'HEAD^{tree}'),
            'ref': environment.get('GITHUB_REF'),
            'workflow_ref': environment.get('GITHUB_WORKFLOW_REF'),
            'workflow_sha': environment.get('GITHUB_WORKFLOW_SHA'),
            'workflow_sha256': digest(workflow),
        },
        'run': {'event': environment.get('GITHUB_EVENT_NAME'),
                'id': environment.get('GITHUB_RUN_ID'), 'attempt': environment.get('GITHUB_RUN_ATTEMPT')},
        'benchmark': {'comparison_id': 'run-' + environment.get('GITHUB_RUN_ID', '') +
                      '-attempt-' + environment.get('GITHUB_RUN_ATTEMPT', ''),
                      'phase': phase, 'aggregate_run_id': 'ci-' + environment.get('GITHUB_SHA', '')},
    }


def load_lock(orchestrator, cache_root):
    base = orchestrator / 'proposals/ci-acceleration'
    pin = read_json(regular(base / PIN_REL))
    require(pin.get('schema') == 1 and type(pin.get('schema')) is int and pin.get('context') == LOCK_CONTEXT,
            'Dependency PIN schema/context changed')
    expected = pin['lock_sha256']
    lock = cache_policy.load_trusted_lock(base / LOCK_REL, expected, cache_root, LOCK_CONTEXT)
    require(pin.get('artifact_count') == len(lock.artifacts) and type(pin.get('artifact_count')) is int,
            'Dependency PIN artifact count changed')
    return lock


def execution_key(lock, context):
    return cache_policy.exact_cache_key(lock) + '-run-' + context['run']['id'] + '-attempt-' + context['run']['attempt']


def output_values(state):
    return {name: state[name] for name in ('cache_key', 'artifact_name', 'evidence_dir', 'lock_sha256')}


def github_output(state, environment):
    path = absolute(environment.get('GITHUB_OUTPUT', ''))
    require(path.is_file() and not path.is_symlink(), 'Existing real GITHUB_OUTPUT file required')
    with path.open('a') as stream:
        for name, value in output_values(state).items():
            require(isinstance(value, str) and '\n' not in value and '\r' not in value, 'Unsafe output value')
            stream.write(name + '=' + value + '\n')


def prepare(source, orchestrator, work, phase, environment=None):
    environment = dict(os.environ if environment is None else environment)
    require_runtime(environment)
    source, orchestrator, work = [absolute(p) for p in (source, orchestrator, work)]
    adapter.validate_roots(source, orchestrator, work)
    require(Path(__file__).resolve() == orchestrator / RUNNER_REL, 'Execute the runner from its real orchestrator checkout')
    require(phase in ('cold', 'warm'), 'Invalid phase')
    absent(work)
    paths = auxiliary_paths(environment, source, orchestrator, work)
    for path in paths.values():
        absent(path)
    context = make_context(source, orchestrator, phase, environment)
    adapter.validate_context(context, environment)
    adapter.validate_checkouts(context, source, orchestrator)
    for root in (source, orchestrator):
        require(adapter.git(root, 'rev-parse', '--is-shallow-repository') == 'false', 'Full Git history required')
    source_contract(source, orchestrator)
    lock = load_lock(orchestrator, paths['cache_root'])
    absent(source / 'build')
    for name in adapter.GENERATED:
        absent(source / name)
    # The adapter directory must contain only its own exact manifest and scripts.
    adapter.materialize(source, orchestrator, work / 'adapter', context, environment)
    evidence = work / 'evidence'
    evidence.mkdir()
    (evidence / 'logs').mkdir()
    write_json(work / 'context.json', context)
    state = {
        'schema': 1, 'status': 'prepared', 'benchmark_only': True, 'cannot_publish': True,
        'source': str(source), 'orchestrator': str(orchestrator), 'work': str(work),
        'phase': phase, 'context': context, 'cache_key': execution_key(lock, context),
        'lock_sha256': lock.sha256, 'evidence_dir': str(evidence),
        'artifact_name': 'benchmark-' + adapter.SOURCE['commit'] + '-' +
                         context['orchestrator']['commit'] + '-' + context['run']['id'] + '-' +
                         context['run']['attempt'] + '-' + phase,
        'paths': {key: str(value) for key, value in paths.items()},
    }
    write_json(work / 'state.json', state)
    write_json(evidence / 'benchmark-context.json', context)
    github_output(state, environment)
    return state


def compile_command(flag, build_root):
    require(type(flag) is int and flag in (0, 1), 'Invalid compile variant')
    command = ['arduino-cli', 'compile', '--verbose', '--jobs', '4', '--fqbn', comparison.FQBN]
    if flag == 0:
        command += ['--build-property', 'compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=0']
    return command + ['--build-path', str(build_root / ('flag-' + str(flag))),
                      '--export-binaries=false', 'firmware/RPG_POKET_2']


class Recorder:
    def __init__(self, source, evidence, environment):
        self.source, self.evidence, self.environment = source, evidence, environment
        self.commands = []
        self.phases = {}

    def run(self, label, command, *, product_log=None, append=False):
        require(re.fullmatch('[a-z0-9_-]+', label), 'Unsafe command label')
        log = self.evidence / 'logs' / (label + '.log')
        absent(log)
        started = time.monotonic_ns()
        entry = {'label': label, 'argv': [str(arg) for arg in command], 'cwd': str(self.source),
                 'log': 'logs/' + log.name, 'started_monotonic_ns': started}
        try:
            with log.open('wb') as stream:
                result = subprocess.run(entry['argv'], cwd=self.source, env=self.environment,
                                        stdout=stream, stderr=subprocess.STDOUT, check=False)
            entry['exit_code'] = result.returncode
        except OSError as error:
            entry.update(exit_code=None, error=str(error))
            raise
        finally:
            entry['elapsed_ns'] = time.monotonic_ns() - started
            self.commands.append(entry)
            write_json(self.evidence / 'commands.json', self.commands)
            if product_log is not None and log.exists():
                with Path(product_log).open('ab' if append else 'wb') as destination, log.open('rb') as source:
                    shutil.copyfileobj(source, destination)
        if entry['exit_code'] != 0:
            raise CommandFailed(label, entry['exit_code'])
        return log

    def phase(self, name, action):
        started = time.monotonic_ns()
        try:
            return action()
        finally:
            self.phases[name] = time.monotonic_ns() - started
            write_json(self.evidence / 'phase-times.json', self.phases)


def verify_dependency_report(report, state, workspace):
    require(report.get('schema') == 1 and type(report.get('schema')) is int and
            report.get('status') == 'verified_and_installed' and report.get('phase') == state['phase'] and
            report.get('context') == LOCK_CONTEXT, 'Incomplete or wrong-phase dependency report')
    expected = {'lock_sha256': state['lock_sha256'], 'cache_key': state['cache_key'],
                'cache_hit': state['phase'] == 'warm', 'benchmark_salt': SALT}
    for name, value in expected.items():
        require(report.get(name) == value and type(report.get(name)) is type(value), 'Dependency report mismatch: ' + name)
    for name in ('cache_bytes', 'dependency_install_ns', 'dependency_verify_ns'):
        require(type(report.get(name)) is int and report[name] > 0, 'Invalid dependency measurement: ' + name)
    for name in ('python', 'cli', 'config', 'tools_dir'):
        path = Path(report[name])
        # A venv's final Python symlink legitimately targets the system interpreter.
        if name == 'python':
            require(path.is_absolute() and path.parent == path.parent.resolve() and '..' not in path.parts,
                    'Noncanonical venv interpreter path')
        else:
            path = absolute(path)
        require(workspace in path.parents, 'Dependency output escaped fresh workspace: ' + name)
        require(path.is_dir() if name == 'tools_dir' else path.is_file(), 'Missing installed dependency: ' + name)
    require(report.get('paths') == {'python': report['python'], 'arduino_cli': report['cli'],
            'arduino_config': report['config'], 'downloads': state['paths']['cache_root']},
            'Dependency report paths disagree')
    require(report.get('cache') == {'key': state['cache_key'], 'hit': state['phase'] == 'warm',
            'restored': state['phase'] == 'warm', 'lock_sha256': state['lock_sha256'],
            'files': 27, 'bytes': report['cache_bytes']}, 'Dependency cache report mismatch')
    pin = read_json(Path(state['orchestrator']) / 'proposals/ci-acceleration' / PIN_REL)
    require(report.get('tool_identity') == {'python': '3.12.3', 'pillow': '11.3.0', 'pyyaml': '6.0.3',
            'arduino_cli_version': '1.5.2-rc.1', 'arduino_cli_commit': pin['arduino_cli_commit'],
            'esp32_core_version': '3.3.12'}, 'Installed dependency tool identities disagree')
    return report


def dependency_environment(environment, report):
    result = dict(environment)
    result['PATH'] = str(Path(report['python']).parent) + os.pathsep + str(Path(report['cli']).parent) + os.pathsep + result.get('PATH', '')
    result['ARDUINO_CONFIG_FILE'] = report['config']
    require({k: v for k, v in result.items() if k.startswith('GITHUB_')} ==
            {k: v for k, v in environment.items() if k.startswith('GITHUB_')}, 'GITHUB context mutated')
    return result


def check_fresh_qualification(state, environment):
    require(state['status'] == 'prepared', 'Qualification is single-use; no resume')
    warm = state['phase'] == 'warm'
    require(environment.get('BENCHMARK_CACHE_HIT') == ('true' if warm else 'false'), 'Exact cache hit/miss required')
    require(environment.get('BENCHMARK_CACHE_KEY') == (state['cache_key'] if warm else ''),
            'Warm must restore exact prepared key; cold must restore nothing')
    source = Path(state['source'])
    absent(source / 'build')
    for name in adapter.GENERATED:
        absent(source / name)
    absent(Path(state['paths']['build_root']))
    require(Path(state['paths']['dependency_work']).is_dir(), 'Fresh dependency installation is missing')
    require(Path(state['paths']['cache_root']).is_dir(), 'Verified download cache is missing')


def verify_aggregate(source, state, plan):
    root = source / 'build/package12-validation' / state['context']['benchmark']['aggregate_run_id']
    summary = read_json(regular(root / 'summary.json'))
    source_manifest = read_json(regular(root / 'source-manifest.json'))
    actual_plan = read_json(regular(root / 'plan.json'))
    checkpoints = read_json(regular(root / 'checkpoints.json'))
    require(summary['all_passed'] is True and summary['failed'] == [] and
            type(summary['tasks']) is int and summary['tasks'] == 157 and summary['lsan'] == 'detect_leaks=1',
            'Aggregate coverage or LSan changed')
    require(source_manifest['compiler'] == comparison.HOST_COMPILER and source_manifest['detect_leaks'] == '1' and
            source_manifest['assertions'] is True and source_manifest['ubsan_halt'] is True, 'Aggregate toolchain/sanitizers changed')
    body = {key: value for key, value in source_manifest.items() if key != 'sha256'}
    require(hashlib.sha256(json.dumps(body, sort_keys=True).encode()).hexdigest() ==
            source_manifest['sha256'] == summary['source_sha256'], 'Aggregate inventory mismatch')
    require(summary['source_sha256'] == comparison.SOURCE_INVENTORY_SHA256, 'Reviewed source inventory changed')
    require(actual_plan == {'focused': False, 'tasks': plan['tasks'], 'source_sha256': summary['source_sha256']},
            'Aggregate exact reviewed plan changed')
    commands = {'preparation': plan['preparation'], **dict(plan['tasks'])}
    require(set(checkpoints) == set(commands) and len(checkpoints) == 157, 'Missing aggregate groups')
    for name, expected in commands.items():
        entry = checkpoints[name]
        require(entry['status'] == 'passed' and entry['commands'] == expected and
                entry['log_sha256'] == digest(regular(root / (name + '.log'))), 'Checkpoint mismatch: ' + name)
    log = regular(source / 'build/coopdiag77-tests.log').read_text(errors='replace')
    require('RESUME skip ' not in log, 'Resumed or skipped aggregate task')
    run_names = re.findall(r'^RUN (.+)$', log, re.M)
    require(len(run_names) == 157 and set(run_names) == set(commands), 'All 157 fresh group executions required')
    return summary


def stage_evidence(source, work, workflow, complete=True):
    evidence = work / 'evidence'
    patterns = upload_patterns(workflow)
    files = set()
    for pattern in patterns:
        if pattern == 'build/coopdiag77-source-identity.json':
            pattern = 'build/coopdiag77-benchmark-identity.json'
        files.update(path for path in source.glob(pattern) if path.is_file() or path.is_symlink())
    for required in ('release/firmware.bin', 'release/artes.pak', 'release/manifest.json', 'release/SHA256SUMS',
                     'build/coopdiag77-benchmark-identity.json'):
        if complete:
            require(source / required in files, 'Missing required final evidence: ' + required)
    absent(source / 'build/coopdiag77-source-identity.json')
    inventory = {}
    for path in sorted(files):
        require(path == path.resolve() and path.is_file() and not path.is_symlink(), 'Symlink/nonregular artifact rejected')
        relative = path.relative_to(source)
        destination = evidence / 'source' / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        require(not destination.is_symlink(), 'Symlink evidence destination rejected')
        shutil.copyfile(path, destination)
        inventory[str(relative)] = {'bytes': destination.stat().st_size, 'sha256': digest(destination)}
    if not (evidence / 'adapter').exists():
        shutil.copytree(work / 'adapter', evidence / 'adapter')
    write_json(evidence / 'source-artifact-inventory.json', inventory)
    return inventory


def make_partial(state, environment, dependencies, summary, identity, phases):
    require(summary['source_sha256'] == comparison.SOURCE_INVENTORY_SHA256, 'Reviewed source inventory changed')
    require(identity['benchmark_only'] is True and identity['cannot_publish'] is True and identity['published'] is False,
            'Final identity is not benchmark-only')
    require(identity['benchmark_context'] == state['context'] and identity['source_commit'] == adapter.SOURCE['commit'] and
            identity['source_tree'] == adapter.SOURCE['tree'] and identity['aggregate'] == summary,
            'Final identity binding mismatch')
    require(identity['adapter_manifest_sha256'] == digest(Path(state['work']) / 'adapter/adapter-manifest.json'),
            'Final adapter manifest binding mismatch')
    require(identity['target_builds']['compiler'] == 'esp-x32/2601' and identity['target_builds']['core'] == '3.3.12' and
            identity['target_builds']['fqbn'] == comparison.FQBN, 'Target compiler/core/FQBN mismatch')
    require(set(phases) == set(PARTIAL_PHASES) and all(type(x) is int and x > 0 for x in phases.values()), 'Missing phase timing')
    expected_files = {}
    for name in ('firmware.bin', 'artes.pak', 'manifest.json'):
        path = regular(Path(state['source']) / 'release' / name)
        require(path.stat().st_size > 0, 'Empty final release file')
        expected_files[name] = {'bytes': path.stat().st_size, 'sha256': digest(path)}
    require(identity['artifacts'] == expected_files, 'Final release bytes changed after identity gate')
    observed = {
        'source_commit': adapter.SOURCE['commit'], 'source_tree': adapter.SOURCE['tree'],
        'source_inventory_sha256': summary['source_sha256'], 'reviewed_plan_sha256': comparison.PLAN_SHA256,
        'adapter_sha256': digest(Path(state['orchestrator']) / adapter.ADAPTER),
        'dependency_lock_sha256': state['lock_sha256'], 'runner_image': 'ubuntu-24.04',
        'runner_image_version': environment['ImageVersion'], 'runner_os': environment['RUNNER_OS'],
        'runner_arch': environment['RUNNER_ARCH'], 'python': LOCK_CONTEXT['python_version'],
        'pillow': LOCK_CONTEXT['pillow_version'], 'pyyaml': LOCK_CONTEXT['pyyaml_version'],
        'host_compiler': comparison.HOST_COMPILER, 'arduino_cli': LOCK_CONTEXT['arduino_cli_version'],
        'esp32_core': identity['target_builds']['core'], 'target_compiler': identity['target_builds']['compiler'],
        'fqbn': identity['target_builds']['fqbn'], 'host_jobs': 3, 'esp32_jobs': 4,
        'orchestrator_sha': state['context']['orchestrator']['commit'],
    }
    require(set(observed) == set(comparison.IDENTITY), 'Comparator identity schema changed')
    return {
        'schema': 1, 'kind': 'benchmark-partial', 'phase': state['phase'],
        'benchmark_only': True, 'cannot_publish': True, 'context': state['context'], 'identity': observed,
        'cache': {'key': state['cache_key'], 'benchmark_salt': SALT, 'hit': dependencies['cache_hit'],
                  'restored': state['phase'] == 'warm', 'contents_verified': True, 'forbidden_paths': [],
                  'bytes': dependencies['cache_bytes']},
        'gates': {name: 'passed' for name in comparison.GATES},
        'coverage': {'groups': 157, 'skipped': 0, 'resumed': 0, 'lsan': 'detect_leaks=1',
                     'asan': True, 'ubsan_halt_on_error': True, 'assertions': True},
        'phases_ns': phases, 'release_files': expected_files,
    }


def qualify(source, orchestrator, work, phase, environment=None):
    environment = dict(os.environ if environment is None else environment)
    require_runtime(environment)
    source, orchestrator, work = [absolute(p) for p in (source, orchestrator, work)]
    adapter.validate_roots(source, orchestrator, work)
    require(Path(__file__).resolve() == orchestrator / RUNNER_REL, 'Execute the real orchestrator runner')
    state = read_json(regular(work / 'state.json'))
    for name, value in (('source', str(source)), ('orchestrator', str(orchestrator)), ('work', str(work)), ('phase', phase)):
        require(state[name] == value, 'Prepared state mismatch: ' + name)
    context = make_context(source, orchestrator, phase, environment)
    require(context == state['context'] == read_json(work / 'context.json'), 'Prepared context changed')
    manifest = adapter.verify_materialized(work / 'adapter', environment)
    require(manifest['context'] == context, 'Prepared adapter/context mismatch')
    paths = auxiliary_paths(environment, source, orchestrator, work)
    require(state['paths'] == {key: str(value) for key, value in paths.items()}, 'Prepared workspace paths changed')
    lock = load_lock(orchestrator, paths['cache_root'])
    require(state['lock_sha256'] == lock.sha256 and state['cache_key'] == execution_key(lock, context), 'Prepared lock/key changed')
    _, plan, workflow = source_contract(source, orchestrator)
    check_fresh_qualification(state, environment)
    evidence = work / 'evidence'
    require(state['evidence_dir'] == str(evidence), 'Prepared evidence path changed')
    recorder = Recorder(source, evidence, environment)
    state['status'] = 'running'
    write_json(work / 'state.json', state)
    try:
        verify_started = time.monotonic_ns()
        dependencies = verify_dependency_report(read_json(regular(evidence / 'dependencies.json')), state, paths['dependency_work'])
        cache_policy._verify_tree(paths['cache_root'], lock)
        require(dependencies['cache_bytes'] == sum(item.bytes for item in lock.artifacts), 'Dependency cache byte count changed')
        install_log = regular(paths['dependency_work'] / 'install.log')
        shutil.copyfile(install_log, evidence / 'logs/dependency-install.log')
        recorder.phases.update({name: dependencies[name + '_ns'] for name in ('dependency_install', 'dependency_verify')})
        recorder.environment = dependency_environment(environment, dependencies)
        # Check the selected interpreter and installed packages, not just labels.
        recorder.run('python-dependency-versions', ['python3', '-c',
                     'import sys,PIL,yaml; '
                     'assert sys.flags.optimize == 0; '
                     'assert sys.version_info[:3] == (3,12,3); '
                     'assert PIL.__version__ == "11.3.0"; assert yaml.__version__ == "6.0.3"; '
                     'print(sys.version); print(PIL.__version__); print(yaml.__version__)'])
        recorder.phases['dependency_verify'] += time.monotonic_ns() - verify_started
        (source / 'build').mkdir()
        recorder.phase('aggregate', lambda: recorder.run('aggregate', ['bash', str(work / 'adapter/test_coopdiag77.sh')],
                                                       product_log=source / 'build/coopdiag77-tests.log'))
        summary = verify_aggregate(source, state, plan)
        scope = ['python3', str(work / 'adapter/check_scope.py')]
        def build_on():
            require(adapter.git(source, 'rev-parse', 'HEAD') == adapter.SOURCE['commit'], 'Source HEAD changed before compile')
            recorder.run('scope-before-build', scope)
            toolchain = source / 'build/coopdiag77-esp32-toolchain.txt'
            recorder.run('arduino-cli-version', ['arduino-cli', 'version'], product_log=toolchain)
            require(re.search(r'Version:\s*1\.5\.2-rc\.1(?:\s|$)', toolchain.read_text()), 'Observed CLI version mismatch')
            recorder.run('arduino-core-list', ['arduino-cli', 'core', 'list'], product_log=toolchain, append=True)
            require(re.search(r'^esp32:esp32\s+3\.3\.12(?:\s|$)', toolchain.read_text(), re.M), 'Observed ESP32 core mismatch')
            recorder.run('esp32-on', compile_command(1, paths['build_root']),
                         product_log=source / 'build/coopdiag77-esp32-flag-1.log')
        recorder.phase('esp32_on', build_on)
        def build_off():
            recorder.run('esp32-off', compile_command(0, paths['build_root']),
                         product_log=source / 'build/coopdiag77-esp32-flag-0.log')
            shutil.copyfile(regular(paths['build_root'] / 'flag-1/RPG_POKET_2.ino.bin'), source / 'build/RPG_POKET_2.ino.bin')
            recorder.run('scope-after-build', scope)
        recorder.phase('esp32_off', build_off)
        def final_bytes():
            for label, command in zip(('release-manifest', 'release-verify', 'release-verifier-tests', 'ota-normal', 'ota-sanitizers'), FINAL_COMMANDS):
                recorder.run(label, list(command))
            recorder.run('scope-final-bytes', scope)
        recorder.phase('final_bytes_ota', final_bytes)
        recorder.phase('identity', lambda: recorder.run('identity', ['python3', str(work / 'adapter/record_identity.py'),
                       '--build-root', str(paths['build_root']), '--tools-dir', dependencies['tools_dir']]))
        identity = read_json(regular(source / 'build/coopdiag77-benchmark-identity.json'))
        adapter.verify_materialized(work / 'adapter', environment)
        partial = make_partial(state, environment, dependencies, summary, identity, recorder.phases)
        stage_evidence(source, work, workflow)
        write_json(evidence / 'measurement.partial.json', partial)
        state['status'] = 'passed'
    except BaseException as error:
        state['status'] = 'failed'
        try:
            stage_evidence(source, work, workflow, complete=False)
        except Exception as staging_error:
            write_json(evidence / 'staging-error.json', {'error': str(staging_error)})
        write_json(evidence / 'runner-status.json', {'benchmark_only': True, 'cannot_publish': True,
                   'status': 'failed', 'error': str(error), 'commands': recorder.commands, 'phases_ns': recorder.phases})
        raise
    finally:
        write_json(work / 'state.json', state)
    write_json(evidence / 'runner-status.json', {'benchmark_only': True, 'cannot_publish': True,
               'status': 'passed', 'phases_ns': recorder.phases})
    return partial


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=('prepare', 'qualify'))
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--orchestrator', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--phase', choices=('cold', 'warm'), required=True)
    args = parser.parse_args(argv)
    action = prepare if args.command == 'prepare' else qualify
    action(args.source, args.orchestrator, args.work, args.phase)
    print('Benchmark-only ' + args.command + ' completed; cannot publish.')


if __name__ == '__main__':
    try:
        main()
    except (Rejected, adapter.Rejected, cache_policy.PolicyError, OSError, KeyError, ValueError) as error:
        raise SystemExit('REJECTED: ' + str(error))
