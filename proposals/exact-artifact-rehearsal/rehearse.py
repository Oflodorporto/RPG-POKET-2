#!/usr/bin/env python3
"""Inactive, read-only exact-artifact rehearsal. Never a release authorization.

Historical qualification has its OWN API/log/checkpoint format. It is not a
benchmark run, and no historical GITHUB_* variables or command ledger are made up.
Only three pinned data files are copied. Code comes from a clean source checkout.
"""
import argparse
from datetime import datetime, timedelta, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import time

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
SUPPORT = HERE.parent / 'ci-acceleration'
sys.path.insert(0, str(SUPPORT))
import collect_benchmark as common

Rejected = common.Rejected
require = common.require
sha = common.sha256
PIN = common.local_json(HERE / 'historical-pin.json')
NAMES = ('firmware.bin', 'artes.pak', 'manifest.json')
WORKFLOW = '.github/workflows/exact-artifact-rehearsal.yml'
REF = 'refs/heads/bench/coopdiag77-cache-promotion'
COMMANDS = (
    ('release-verify', ('python3', 'tools/verify_coopdiag77_release.py')),
    ('release-verifier-tests', ('python3', 'tests/coopdiag77_release_verifier.py')),
    ('ota-normal', ('python3', 'tools/test_coopdiag77_release.py', 'release')),
    ('ota-sanitizers', ('python3', 'tools/test_coopdiag77_release.py', 'release', '--sanitizers')),
)


def write(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n')


def git(root, *args):
    return subprocess.check_output(['git', '-C', str(root), *args], text=True).strip()


def descriptor(data):
    return {'bytes': len(data), 'sha256': sha(data)}


def utc(value):
    return datetime.fromisoformat(value.replace('Z', '+00:00')).astimezone(timezone.utc)


def same(value, expected, label):
    require(json.dumps(value, sort_keys=True) == json.dumps(expected, sort_keys=True), label)


def flatten(pages, key):
    require(type(pages) is list and 0 < len(pages) <= 100, 'Missing API pages')
    count, result = pages[0].get('total_count'), []
    require(type(count) is int and 0 < count <= 10000, 'Invalid API count')
    for index, page in enumerate(pages):
        require(page.get('total_count') == count and type(page.get(key)) is list, 'Page count mismatch')
        require(len(page[key]) == 100 if index < len(pages)-1 else 0 < len(page[key]) <= 100, 'Incomplete pagination')
        result.extend(page[key])
    require(len(result) == count and len({row['id'] for row in result}) == count, 'Incomplete/duplicate API list')
    return result


def validate_api(run, attempt, jobs_pages, artifact_pages, artifact, commit, now):
    """Explicit historical contract; does not read or alter the current environment."""
    for record in (run, attempt):
        for field, value in {'id': PIN['run_id'], 'run_attempt': PIN['attempt'],
                'workflow_id': PIN['workflow_id'], 'path': PIN['workflow_path'], 'event': 'push',
                'head_sha': PIN['source_commit'], 'head_branch': PIN['branch'],
                'status': 'completed', 'conclusion': 'success'}.items():
            same(record.get(field), value, 'Historical run mismatch: ' + field)
        for name in ('repository', 'head_repository'):
            row = record.get(name, {})
            same(row.get('id'), PIN['repository_id'], 'Fork/repository ID mismatch')
            require(row.get('full_name') == PIN['repository'], 'Repository name mismatch')
    same(commit.get('sha'), PIN['source_commit'], 'Source API commit mismatch')
    same(commit.get('tree', {}).get('sha'), PIN['source_tree'], 'Source API tree mismatch')
    same([p.get('sha') for p in commit.get('parents', [])], [PIN['source_parent']], 'Source parent mismatch')
    jobs = flatten(jobs_pages, 'jobs')
    require(len(jobs) == 1, 'Historical qualification must have its one exact job')
    job = jobs[0]
    for field, value in {'id': PIN['job_id'], 'run_id': PIN['run_id'], 'run_attempt': PIN['attempt'],
            'head_sha': PIN['source_commit'], 'head_branch': PIN['branch'], 'name': PIN['job_name'],
            'status': 'completed', 'conclusion': 'success'}.items():
        same(job.get(field), value, 'Historical job mismatch: ' + field)
    start, end = utc(job['started_at']), utc(job['completed_at'])
    require(start < end <= now, 'Historical job time mismatch')
    require(len(job.get('steps', [])) == 12 and all(s.get('status') == 'completed' and
            s.get('conclusion') == 'success' for s in job['steps']), 'Historical steps incomplete')
    artifacts = flatten(artifact_pages, 'artifacts')
    require(len(artifacts) == 1, 'Missing or ambiguous historical artifact')
    same(artifacts[0], artifact, 'Artifact list/detail mismatch')
    for field, value in {'id': PIN['artifact_id'], 'name': PIN['artifact_name'],
            'size_in_bytes': PIN['zip_bytes'], 'digest': 'sha256:' + PIN['zip_sha256'], 'expired': False}.items():
        same(artifact.get(field), value, 'Historical artifact mismatch: ' + field)
    same(artifact.get('workflow_run'), {'id': PIN['run_id'], 'repository_id': PIN['repository_id'],
        'head_repository_id': PIN['repository_id'], 'head_branch': PIN['branch'],
        'head_sha': PIN['source_commit']}, 'Artifact run/repository mismatch')
    require(start <= utc(artifact['created_at']) <= end and utc(artifact['expires_at']) > now + timedelta(minutes=10),
            'Artifact time/expiry mismatch')
    return job


def validate_source(source, copied=False):
    require(source == source.resolve() and source.is_dir(), 'Noncanonical source')
    require(git(source, 'rev-parse', 'HEAD') == PIN['source_commit'], 'Source HEAD mismatch')
    require(git(source, 'rev-parse', 'HEAD^{tree}') == PIN['source_tree'], 'Source tree mismatch')
    require(git(source, 'rev-list', '--parents', '-n', '1', 'HEAD').split() ==
            [PIN['source_commit'], PIN['source_parent']], 'Source ancestry mismatch')
    require(not git(source, 'diff', 'HEAD', '--'), 'Tracked source changed')
    extras = git(source, 'ls-files', '--others').splitlines()
    require(set(extras) <= ({'release/' + n for n in NAMES} if copied else set()), 'Unexpected untracked source files')
    require(sha((source / PIN['workflow_path']).read_bytes()) == PIN['workflow_sha256'], 'Original workflow changed')
    if not copied:
        require(not (source / 'release').exists() and not (source / 'build').exists(), 'Fresh source outputs required')


def validate_evidence(archive, source, job, raw_log):
    require(len(archive.members) == PIN['zip_members'], 'Historical ZIP inventory count changed')
    require(sha(('\n'.join(sorted(archive.members)) + '\n').encode()) == PIN['member_names_sha256'], 'Historical ZIP inventory changed')
    require(sha(raw_log) == PIN['job_log_sha256'], 'Historical job log bytes changed')
    files = {n: archive.digest('release/' + n) for n in NAMES}
    same(files, PIN['payload'], 'Historical release bytes changed')
    require(archive.read('release/SHA256SUMS') == ''.join(files[n]['sha256'] + '  ' + n + '\n' for n in NAMES).encode(), 'Historical checksum list changed')
    identity = archive.json('build/coopdiag77-source-identity.json', finite_floats=True)
    require(archive.digest('build/coopdiag77-source-identity.json')['sha256'] == PIN['identity_sha256'], 'Historical final identity changed')
    require(identity.get('source_commit') == PIN['source_commit'] and identity.get('source_tree') == PIN['source_tree'] and
        identity.get('branch') == 'refs/heads/' + PIN['branch'] and identity.get('published') is False and
        identity.get('leak_detection') is True and identity.get('artifacts') == files, 'Historical final identity mismatch')
    root = 'build/package12-validation/ci-' + PIN['source_commit'] + '/'
    manifest = archive.json(root + 'source-manifest.json')
    body = {k: v for k, v in manifest.items() if k != 'sha256'}
    require(sha(json.dumps(body, sort_keys=True).encode()) == manifest.get('sha256') == PIN['source_inventory_sha256'], 'Source inventory digest mismatch')
    require(manifest.get('compiler') == PIN['host_compiler'] and manifest.get('detect_leaks') == '1' and
            manifest.get('assertions') is True and manifest.get('ubsan_halt') is True, 'Historical compiler/sanitizers changed')
    require(len(manifest['files']) == 1178, 'Source inventory count changed')
    for name, digest in manifest['files'].items():
        common.safe_name(name)
        path = source / name
        require(path.is_file() and not path.is_symlink() and sha(path.read_bytes()) == digest, 'Source bytes changed: ' + name)
    reviewed = common.local_json(SUPPORT / 'reviewed-plan.json')
    require(sha((SUPPORT / 'reviewed-plan.json').read_bytes()) == PIN['plan_sha256'], 'Reviewed plan changed')
    same(archive.json(root + 'plan.json'), {'focused': False, 'tasks': reviewed['tasks'],
        'source_sha256': PIN['source_inventory_sha256']}, 'Historical plan mismatch')
    summary = archive.json(root + 'summary.json')
    same(summary, {'source_sha256': PIN['source_inventory_sha256'], 'all_passed': True, 'tasks': 157,
        'failed': [], 'lsan': 'detect_leaks=1', 'esp32_build': False, 'publication': False}, 'Historical summary mismatch')
    same(identity.get('aggregate'), summary, 'Aggregate/final identity mismatch')
    commands = {'preparation': reviewed['preparation'], **dict(reviewed['tasks'])}
    checkpoints = archive.json(root + 'checkpoints.json', finite_floats=True)
    require(set(checkpoints) == set(commands) and len(commands) == 157, 'Historical checkpoints missing')
    for name, argv in commands.items():
        row = checkpoints[name]
        log = archive.read(root + name + '.log').decode()
        require(row.get('status') == 'passed' and row.get('commands') == argv and
            row.get('log_sha256') == sha(log.encode()), 'Historical checkpoint mismatch: ' + name)
        require(utc(job['started_at']) <= utc(row['started_utc']) <= utc(job['completed_at']), 'Checkpoint outside historical job')
        require([line[2:] for line in log.splitlines() if line.startswith('$ ')] == [' '.join(c) for c in argv], 'Checkpoint commands not in log')
        require(not any(s in log for s in ('ERROR: AddressSanitizer', 'ERROR: LeakSanitizer', 'runtime error:', 'LeakSanitizer has encountered a fatal error')), 'Historical sanitizer failed')
    aggregate = archive.read('build/coopdiag77-tests.log').decode()
    marker = re.search(r'^PASS: \d+ exact-SHA[^\n]*\n', aggregate, re.M)
    require(marker is not None, 'Missing historical scope prelude')
    fresh = aggregate[marker.end():]
    require('RESUME skip ' not in fresh and 'LOCAL LIMIT: LeakSanitizer disabled' not in fresh, 'Historical resumed/relaxed run')
    for name in commands:
        require(len(re.findall(r'^PASSED ' + re.escape(name) + r'$', fresh, re.M)) == 1, 'Missing fresh historical group: ' + name)
    target = archive.json('build/coopdiag77-esp32-build-evidence.json', finite_floats=True)
    require(archive.digest('build/coopdiag77-esp32-build-evidence.json')['sha256'] == PIN['target_evidence_sha256'], 'Target evidence bytes changed')
    same(target, identity['target_builds'], 'Target/final identity mismatch')
    require(target['source_commit'] == PIN['source_commit'] and target['source_tree'] == PIN['source_tree'] and
        target['core'] == '3.3.12' and target['compiler'] == 'esp-x32/2601' and target['default_arduino_enabled'] is True,
        'Target source/toolchain mismatch')
    for flag, enabled in [('1', True), ('0', False)]:
        build = target['builds'][flag]
        require(build['feature_enabled'] is enabled and build['effective_options']['default_arduino_enabled'] is enabled and
            build['effective_options']['fqbn'] == common.comparison.FQBN, 'ON/OFF options changed')
        same(build['effective_options']['explicit_override'], None if enabled else 0, 'ON/OFF override changed')
        require(build['linkage_audit'] and build['updater_linkage_audit'] and build['translation_units'], 'Missing target linkage evidence')
    same(target['builds']['1']['image'], files['firmware.bin'], 'Released firmware is not default ON')
    text = re.sub(r'\x1b\[[0-9;]*m', '', raw_log.decode('utf-8-sig'))
    start = text.index('##[group]Run python3 tools/release_coopdiag77_manifest.py')
    end = text.index('##[group]Run python3 tools/record_coopdiag77_identity.py', start)
    final = text[start:end]
    for command in ['python3 tools/release_coopdiag77_manifest.py', *[' '.join(c) for _, c in COMMANDS]]:
        require(command in final, 'Missing historical final command: ' + command)
    require('RPG_DETECT_LEAKS: 1' in final and 'PASS: 147 coopdiag77 verifier probes' in final and
        final.count('exact prepared/downloaded release bytes. No publication/device I/O.') == 2, 'Historical final gates missing')
    require(not any(x in final for x in ('HOST FIXTURE MODE', '--synthetic', 'detect_leaks=0', 'ERROR: AddressSanitizer', 'ERROR: LeakSanitizer', 'runtime error:')), 'Historical final gates unsafe')
    for label in ('touch77', 'scenes77', 'scale77', 'camp77', 'explore77', 'immersion77', 'story77', 'clock77', 'pvp77', 'shop77', 'coop77', 'coopfix77', 'coopdiag77'):
        require(len(re.findall(r'PASS: ' + label + r', \d+ exact production headers and real cJSON unchanged;', final)) == 2, 'Historical final client omitted: ' + label)
    return {'groups': 157, 'source_files': 1178, 'historical_final_gate_commands': 5,
        'historical_ota_modes': 2, 'source_identity_sha256': PIN['identity_sha256'], 'release_files': files}


def payload_hashes(folder):
    require(folder.is_dir() and not folder.is_symlink() and set(p.name for p in folder.iterdir()) == set(NAMES), 'Release triple shape changed')
    result = {}
    for name in NAMES:
        path = folder / name
        require(stat.S_ISREG(path.lstat().st_mode), 'Nonregular release file')
        result[name] = descriptor(path.read_bytes())
    return result


def copy_payload(archive, source):
    folder = source / 'release'
    folder.mkdir(mode=0o700)  # No reuse, overwrite, symlink or extraction.
    for name in NAMES:
        data = archive.read('release/' + name)
        same(descriptor(data), PIN['payload'][name], 'Payload changed before copy')
        with (folder / name).open('xb') as stream:
            stream.write(data)
    same(payload_hashes(folder), PIN['payload'], 'Copied bytes differ')


def gate_env(environment):
    # Preserve real GITHUB_* identity; remove credentials and interpreter/compiler injection.
    reject = ('CXX', 'CC', 'PYTHONPATH', 'PYTHONHOME', 'PYTHONUSERBASE', 'LD_PRELOAD',
        'LD_LIBRARY_PATH', 'GCC_EXEC_PREFIX', 'COMPILER_PATH', 'LIBRARY_PATH', 'CPATH', 'CPLUS_INCLUDE_PATH')
    require(not any(environment.get(k) for k in reject), 'Injected compiler/interpreter environment')
    result = {k: v for k, v in environment.items() if k not in ('GH_TOKEN', 'GITHUB_TOKEN')}
    result['CXX'] = shutil.which('g++', path=result.get('PATH'))
    require(result['CXX'] is not None, 'Host compiler missing')
    return result


def run_gates(source, work, environment):
    require(not sys.flags.optimize and not environment.get('PYTHONOPTIMIZE'), 'Assertions must remain enabled')
    require(environment.get('RPG_DETECT_LEAKS') == '1', 'LSan=1 is mandatory')
    require(environment.get('PYTHONDONTWRITEBYTECODE') == '1', 'No pycache in clean source')
    records = []
    before = payload_hashes(source / 'release')
    same(before, PIN['payload'], 'Gate inputs differ from qualified payload')
    write(work / 'hashes-before.json', before)
    try:
        for label, argv in COMMANDS:
            started = time.monotonic_ns()
            log = work / (label + '.log')
            with log.open('xb') as stream:
                actual_argv = [sys.executable, '-B', '-s', '-E', *argv[1:]]
                done = subprocess.run(actual_argv, cwd=source, env=environment, stdout=stream, stderr=subprocess.STDOUT)
            records.append({'label': label, 'argv': actual_argv, 'reviewed_command': list(argv), 'cwd': str(source), 'log': log.name,
                'started_monotonic_ns': started, 'elapsed_ns': time.monotonic_ns()-started,
                'exit_code': done.returncode, 'log_sha256': sha(log.read_bytes())})
            write(work / 'commands.json', records)
            require(done.returncode == 0, 'Gate failed: ' + label)
    finally:
        after = payload_hashes(source / 'release')
        write(work / 'hashes-after.json', after)
        same(after, before, 'Release payload mutated by a gate')
        validate_source(source, copied=True)
    return records


def current_identity(orchestrator, environment):
    require(environment.get('GITHUB_ACTIONS') == 'true', 'CI mode requires real Actions environment')
    commit = git(orchestrator, 'rev-parse', 'HEAD')
    require(re.fullmatch('[0-9a-f]{40}', commit) and commit != PIN['source_commit'], 'Separate orchestration SHA required')
    expected = {'GITHUB_REPOSITORY': PIN['repository'], 'GITHUB_REPOSITORY_ID': str(PIN['repository_id']),
        'GITHUB_SHA': commit, 'GITHUB_WORKFLOW_SHA': commit, 'GITHUB_EVENT_NAME': 'push', 'GITHUB_REF': REF,
        'GITHUB_WORKFLOW_REF': PIN['repository'] + '/' + WORKFLOW + '@' + REF}
    for name, value in expected.items():
        require(environment.get(name) == value, 'Current rehearsal identity mismatch: ' + name)
    require(not git(orchestrator, 'status', '--porcelain', '--untracked-files=all'), 'Orchestrator must be clean')
    for name in ('GITHUB_RUN_ID', 'GITHUB_RUN_ATTEMPT'):
        require(re.fullmatch('[1-9][0-9]*', environment.get(name, '')), 'Current run identity missing')
    require(environment['GITHUB_RUN_ID'] != str(PIN['run_id']), 'Historical run cannot impersonate rehearsal')
    return {name: environment[name] for name in (*expected, 'GITHUB_RUN_ID', 'GITHUB_RUN_ATTEMPT')}


def api(endpoint, destination, pages=False):
    command = ['gh', 'api', '--hostname', 'github.com', '--method', 'GET',
               '-H', 'Accept: application/vnd.github+json', '-H', 'X-GitHub-Api-Version: 2022-11-28']
    if pages:
        command += ['--paginate', '--slurp']
    with destination.open('xb') as stream:
        subprocess.run([*command, endpoint], stdout=stream, check=True, timeout=180)


def fetch_inputs(folder):
    started = time.monotonic_ns()
    started_at = datetime.now(timezone.utc).isoformat()
    base = '/repos/' + PIN['repository']
    run = base + '/actions/runs/' + str(PIN['run_id'])
    endpoints = {'run.json': run, 'attempt.json': run + '/attempts/' + str(PIN['attempt']),
        'jobs.json': run + '/attempts/' + str(PIN['attempt']) + '/jobs?per_page=100',
        'artifacts.json': run + '/artifacts?per_page=100',
        'artifact.json': base + '/actions/artifacts/' + str(PIN['artifact_id']),
        'commit.json': base + '/git/commits/' + PIN['source_commit'],
        'main-before.json': base + '/git/ref/heads/main',
        'job.log': base + '/actions/jobs/' + str(PIN['job_id']) + '/logs',
        'qualified.zip': base + '/actions/artifacts/' + str(PIN['artifact_id']) + '/zip'}
    for name, endpoint in endpoints.items():
        api(endpoint, folder / name, pages=name in ('jobs.json', 'artifacts.json'))
    elapsed = time.monotonic_ns() - started
    require(elapsed <= 300 * 1_000_000_000, 'API acquisition exceeded five-minute freshness window; retry in a fresh workspace')
    write(folder / 'fetch.json', {'started_at': started_at, 'completed_at': datetime.now(timezone.utc).isoformat(),
        'elapsed_ns': elapsed, 'method': 'GET', 'endpoints': endpoints})


def inputs(folder, local):
    read = lambda n: common.local_json(folder / n)
    # Explicit historical snapshot mode: never represents a fresh authenticated fetch.
    return (read('run.json'), read('run.json') if local else read('attempt.json'),
        [read('jobs.json')] if local else read('jobs.json'), [read('artifacts.json')] if local else read('artifacts.json'),
        read('artifacts.json')['artifacts'][0] if local else read('artifact.json'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--orchestrator', type=Path)
    parser.add_argument('--setup-report', type=Path)
    parser.add_argument('--local-snapshot', type=Path)
    parser.add_argument('--local-commit-json', type=Path)
    args = parser.parse_args()
    source, work = args.source.absolute(), args.work.absolute()
    require(source == source.resolve() and work == work.resolve(), 'Canonical paths required')
    require(source != work and source not in work.parents and work not in source.parents, 'Disjoint source/work required')
    work.mkdir(mode=0o700)  # A new workspace is mandatory, even after failure.
    start = time.monotonic_ns()
    report = {'schema': 1, 'kind': 'exact-artifact-read-only-rehearsal', 'status': 'running',
        'cannot_publish': True, 'publish_authorized': False, 'authenticated_provenance': False,
        'repository_or_release_writes': False, 'historical_run': PIN['run_id'], 'historical_attempt': PIN['attempt'],
        'historical_artifact': PIN['artifact_id'], 'source_commit': PIN['source_commit'],
        'historical_format': 'original-qualification-api-job-log-and-checkpoints', 'controlled_performance_comparison': False,
        'note': 'No release authorization, fresh qualification, publisher total, or public downloads are implied.'}
    try:
        validate_source(source)
        environment = dict(os.environ)
        local = args.local_snapshot is not None
        if local:
            require(environment.get('GITHUB_ACTIONS') != 'true', 'Local snapshots forbidden in Actions')
            require(args.local_commit_json is not None, 'Local historical source API snapshot required')
            report['mode'] = 'local-smoke-not-comparable'
            report['orchestration'] = {'script_sha256': sha(Path(__file__).read_bytes()), 'actions_run': None}
            folder, commit = args.local_snapshot, common.local_json(args.local_commit_json)
            report['download_ns'] = None
            report['fresh_api_fetched'] = False
            report['snapshot_limit'] = 'Saved historical run/jobs/artifact metadata, not a current API observation or independent attempt fetch.'
        else:
            require(args.orchestrator is not None, 'Orchestrator checkout required')
            orchestrator = args.orchestrator.absolute()
            require(Path(__file__).resolve() == orchestrator / 'proposals/exact-artifact-rehearsal/rehearse.py', 'Runner must come from current clean orchestrator')
            report['orchestration'] = current_identity(orchestrator, environment)
            require(args.setup_report is not None, 'Fresh host setup report required')
            setup = common.local_json(args.setup_report)
            import setup_host
            lock, wheels = setup_host.wheels()
            require(setup.get('status') == 'installed-fresh-host-wheels-only' and setup.get('lock_sha256') == setup_host.LOCK_SHA and
                setup.get('wheels') == wheels and setup.get('python') == sys.version.split()[0] == lock['context']['python_version'] and
                setup.get('cache_restored') is False and setup.get('arduino_or_esp32_installed') is False, 'Host setup mismatch')
            report['host_setup'] = setup
            folder = work / 'api'; folder.mkdir()
            started = time.monotonic_ns(); fetch_inputs(folder)
            report['download_ns'] = time.monotonic_ns() - started
            report['fresh_api_fetched'] = True
            commit = common.local_json(folder / 'commit.json')
            report['mode'] = 'read-only-actions'
        report['environment'] = {'host_compiler': subprocess.check_output(['g++', '--version'], text=True).splitlines()[0],
            'python': sys.version.split()[0], 'runner_image': environment.get('ImageOS'), 'runner_image_version': environment.get('ImageVersion')}
        report['same_historical_image_and_compiler'] = report['environment']['host_compiler'] == PIN['host_compiler'] and \
            report['environment']['runner_image'] == 'ubuntu24' and report['environment']['runner_image_version'] == '20261004.327.1'
        started = time.monotonic_ns()
        run, attempt, jobs, artifacts, artifact = inputs(folder, local)
        job = validate_api(run, attempt, jobs, artifacts, artifact, commit, datetime.now(timezone.utc))
        archive = common.Archive(folder / 'qualified.zip', artifact)
        try:
            report['historical_evidence'] = validate_evidence(archive, source, job, (folder / 'job.log').read_bytes())
            report['verification_ns'] = time.monotonic_ns() - started
            started = time.monotonic_ns(); copy_payload(archive, source)
            report['copy_ns'] = time.monotonic_ns() - started
        finally:
            archive.close()
        gate_environment = gate_env(environment)
        report['gate_runtime'] = {'python_executable': sys.executable, 'compiler_executable': gate_environment['CXX']}
        report['commands'] = run_gates(source, work, gate_environment)
        report['payload_byte_identical'] = True
        if not local:
            api('/repos/' + PIN['repository'] + '/git/ref/heads/main', folder / 'main-after.json')
            report['main_before'] = common.local_json(folder / 'main-before.json')['object']['sha']
            report['main_after'] = common.local_json(folder / 'main-after.json')['object']['sha']
            report['main_observation_is_not_lease_or_approval'] = True
        report['status'] = 'passed-local-smoke' if local else 'passed-read-only-rehearsal'
        report['comparison_status'] = 'inconclusive'  # Original Python/setup were not fully pinned; never a controlled pair.
        return 0
    except Exception as error:
        report['status'], report['error'] = 'failed', str(error)
        return 1
    finally:
        report['total_runner_ns'] = time.monotonic_ns() - start
        write(work / 'report.json', report)
        print(json.dumps(report, indent=2))


if __name__ == '__main__':
    raise SystemExit(main())
