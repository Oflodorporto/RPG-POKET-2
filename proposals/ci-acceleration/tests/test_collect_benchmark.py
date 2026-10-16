"""Synthetic collector tests. These never compile firmware or contact GitHub."""
from copy import deepcopy
from datetime import datetime, timedelta, timezone
import hashlib
from functools import lru_cache
import io
import json
import os
from pathlib import Path
import stat
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import warnings
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import collect_benchmark as target
import benchmark_adapter as adapter
import compare_benchmark as comparison


def data(value):
    return json.dumps(value, sort_keys=True, allow_nan=False).encode()


def digest(value):
    return {'bytes': len(value), 'sha256': hashlib.sha256(value).hexdigest()}


@lru_cache(maxsize=1)
def pinned_source_manifest():
    """Read the frozen source inventory; all execution/results remain synthetic."""
    root = target.HERE.parents[1]
    paths = subprocess.check_output(['git', '-C', str(root), 'ls-tree', '-r', '-z', '--name-only',
                                     comparison.SOURCE_TREE]).decode().split('\0')
    manifest = {'files': {name: digest((root / name).read_bytes())['sha256'] for name in paths if name},
                'compiler': comparison.HOST_COMPILER, 'detect_leaks': '1',
                'assertions': True, 'ubsan_halt': True}
    manifest['sha256'] = hashlib.sha256(json.dumps(manifest, sort_keys=True).encode()).hexdigest()
    if manifest['sha256'] != comparison.SOURCE_INVENTORY_SHA256:
        raise RuntimeError('Tests require the unchanged reviewed source files')
    return manifest


class Unseekable:
    """Actions-style streaming ZIP writer, with data descriptors."""
    def __init__(self):
        self.buffer = io.BytesIO()
    def write(self, value):
        return self.buffer.write(value)
    def tell(self):
        return self.buffer.tell()
    def flush(self):
        pass


def zip_bytes(entries, *, descriptors=False, directory=False, mode=None, zip64=False):
    output = Unseekable() if descriptors else io.BytesIO()
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        if directory:
            info = zipfile.ZipInfo('source/')
            info.external_attr = (stat.S_IFDIR | 0o755) << 16 | 0x10
            archive.writestr(info, b'')
        for name, body in entries:
            info = zipfile.ZipInfo(name)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = ((stat.S_IFREG | 0o644) if mode is None else mode) << 16
            with archive.open(info, 'w', force_zip64=zip64) as destination:
                destination.write(body)
    result = output.buffer.getvalue() if descriptors else output.getvalue()
    if zip64:
        # Also exercise the real ZIP64 EOCD/locator used for streaming large ZIPs.
        eocd_offset = len(result) - 22
        fields = list(struct.unpack('<4s4H2IH', result[-22:]))
        count, length, offset = fields[4:7]
        zrecord = struct.pack('<4sQ2H2I4Q', b'PK\x06\x06', 44, 45, 45, 0, 0, count, count, length, offset)
        locator = struct.pack('<4sIQI', b'PK\x06\x07', 0, eocd_offset, 1)
        fields[3:7] = [0xffff, 0xffff, 0xffffffff, 0xffffffff]
        result = result[:-22] + zrecord + locator + struct.pack('<4s4H2IH', *fields)
    return result


class Fixture:
    def __init__(self, root):
        self.root = root
        self.now = datetime(2026, 10, 15, 12, 0, tzinfo=timezone.utc)
        self.environment = {'GITHUB_ACTIONS': 'true', 'GITHUB_REPOSITORY': adapter.REPOSITORY,
            'GITHUB_REPOSITORY_ID': '12345', 'GITHUB_SHA': 'c' * 40,
            'GITHUB_REF': adapter.BENCHMARK_REF,
            'GITHUB_WORKFLOW_REF': adapter.REPOSITORY + '/' + adapter.WORKFLOW + '@' + adapter.BENCHMARK_REF,
            'GITHUB_WORKFLOW_SHA': 'c' * 40, 'GITHUB_EVENT_NAME': 'push',
            'GITHUB_RUN_ID': '101', 'GITHUB_RUN_ATTEMPT': '2',
            'RPG_DETECT_LEAKS': '1', 'PYTHONDONTWRITEBYTECODE': '1'}
        self.plan = json.loads((target.HERE / 'reviewed-plan.json').read_bytes())
        self.manifest = deepcopy(pinned_source_manifest())
        self.source_hash = self.manifest['sha256']
        pin = json.loads((target.HERE / 'dependencies/PIN.json').read_bytes())
        self.identity = dict(source_commit=comparison.SOURCE_COMMIT, source_tree=comparison.SOURCE_TREE,
            source_inventory_sha256=self.source_hash, reviewed_plan_sha256=comparison.PLAN_SHA256,
            adapter_sha256=digest((target.HERE / 'benchmark_adapter.py').read_bytes())['sha256'],
            dependency_lock_sha256=pin['lock_sha256'], runner_image='ubuntu-24.04',
            runner_image_version='20261004.327.1', runner_os='Linux', runner_arch='X64', python='3.12.3',
            pillow='11.3.0', pyyaml='6.0.3', host_compiler=comparison.HOST_COMPILER, arduino_cli='1.5.2-rc.1',
            esp32_core='3.3.12', target_compiler='esp-x32/2601', fqbn=comparison.FQBN,
            host_jobs=3, esp32_jobs=4, orchestrator_sha='c' * 40)
        self.members = {}
        self.meta = {}
        self.jobs = []
        for role, offset, job_id, artifact_id in (('cold', 0, 201, 301), ('warm', 120, 202, 302)):
            context = {'schema': 1, 'source': deepcopy(adapter.SOURCE),
                'orchestrator': {'repository': adapter.REPOSITORY, 'repository_id': '12345',
                    'commit': 'c' * 40, 'tree': 'd' * 40, 'ref': adapter.BENCHMARK_REF,
                    'workflow_ref': self.environment['GITHUB_WORKFLOW_REF'], 'workflow_sha': 'c' * 40,
                    'workflow_sha256': 'e' * 64},
                'run': {'event': 'push', 'id': '101', 'attempt': '2'},
                'benchmark': {'comparison_id': 'run-101-attempt-2', 'phase': role,
                              'aggregate_run_id': 'ci-' + 'c' * 40}}
            self.members[role] = self.make_members(context, pin)
            def ts(seconds):
                return (self.now - timedelta(minutes=20) + timedelta(seconds=offset + seconds)).strftime('%Y-%m-%dT%H:%M:%SZ')
            steps = [{'name': 'Upload benchmark evidence', 'number': 9, 'status': 'completed',
                      'conclusion': 'success', 'started_at': ts(90), 'completed_at': ts(95)}]
            cache = ('Cache save', 80) if role == 'cold' else ('Cache restore', 5)
            steps.insert(0, {'name': cache[0], 'number': 8, 'status': 'completed', 'conclusion': 'success',
                             'started_at': ts(cache[1]), 'completed_at': ts(cache[1] + 5)})
            if role == 'cold':
                steps.insert(0, {'name': 'Verify distributions before save', 'number': 7,
                    'status': 'completed', 'conclusion': 'success', 'started_at': ts(70), 'completed_at': ts(75)})
            self.jobs.append({'id': job_id, 'name': role, 'run_id': 101, 'run_attempt': 2,
                'head_sha': 'c' * 40, 'head_branch': adapter.BENCHMARK_REF.removeprefix('refs/heads/'),
                'status': 'completed', 'conclusion': 'success', 'started_at': ts(0), 'completed_at': ts(100), 'steps': steps})
            self.meta[role] = {'id': artifact_id,
                'name': 'benchmark-' + comparison.SOURCE_COMMIT + '-' + 'c' * 40 + '-101-2-' + role,
                'expired': False, 'created_at': ts(92), 'expires_at': '2099-01-01T00:00:00Z',
                'workflow_run': {'id': 101, 'repository_id': 12345, 'head_repository_id': 12345,
                                'head_branch': adapter.BENCHMARK_REF.removeprefix('refs/heads/'), 'head_sha': 'c' * 40}}
        self.pages = [{'total_count': 2, 'jobs': self.jobs}]

    def make_members(self, context, pin):
        role = context['benchmark']['phase']
        members = {'benchmark-context.json': data(context)}
        files = {name: (role + ' synthetic ' + name).encode() for name in target.RELEASE_NAMES}
        release = {name: digest(value) for name, value in files.items()}
        members.update({'source/release/' + name: value for name, value in files.items()})
        members['source/release/SHA256SUMS'] = ''.join(release[name]['sha256'] + '  ' + name + '\n' for name in target.RELEASE_NAMES).encode()
        adapter_files = {'check_scope.py': b'# data only\n', 'record_identity.py': b'# data only\n',
                         'test_coopdiag77.sh': b'# data only\n',
                         'benchmark_runtime.py': (target.HERE / 'benchmark_adapter.py').read_bytes()}
        manifest = {'schema': 1, 'benchmark_only': True, 'cannot_publish': True, 'published': False,
                    'context': context, 'original_sha256': adapter.PINNED,
                    'generated_sha256': {name: digest(value)['sha256'] for name, value in adapter_files.items()},
                    'paths': {'source': '/fixture/source', 'orchestrator': '/fixture/orchestrator', 'output': '/fixture/adapter'},
                    'complete_patches': {}}
        members.update({'adapter/' + name: value for name, value in adapter_files.items()})
        members['adapter/adapter-manifest.json'] = data(manifest)
        ledger = []
        for index, (label, command) in enumerate(target.FINAL_COMMANDS):
            log = 'logs/' + label + '.log'
            members[log] = ('synthetic execution log ' + label + '\n').encode()
            ledger.append({'label': label, 'argv': list(command), 'cwd': manifest['paths']['source'],
                           'log': log, 'started_monotonic_ns': (index + 1) * target.NS,
                           'elapsed_ns': target.NS, 'exit_code': 0})
        members['commands.json'] = data(ledger)
        summary = {'source_sha256': self.source_hash, 'all_passed': True, 'tasks': 157, 'failed': [],
                   'lsan': 'detect_leaks=1', 'esp32_build': False, 'publication': False}
        root = 'source/build/package12-validation/' + context['benchmark']['aggregate_run_id'] + '/'
        members[root + 'source-manifest.json'] = data(self.manifest)
        members[root + 'summary.json'] = data(summary)
        members[root + 'plan.json'] = data({'focused': False, 'tasks': self.plan['tasks'], 'source_sha256': self.source_hash})
        commands = {'preparation': self.plan['preparation'], **dict(self.plan['tasks'])}
        checkpoints = {}
        for name, argv in commands.items():
            log = ('synthetic passed log ' + name + '\n').encode()
            members[root + name + '.log'] = log
            checkpoints[name] = {'status': 'passed', 'commands': argv, 'seconds': 0.25,
                                 'log_sha256': digest(log)['sha256']}
        members[root + 'checkpoints.json'] = data(checkpoints)
        preflight = 'RUN preparation\nPASSED preparation\nRESUME skip preparation\n'
        members['source/build/coopdiag77-tests.log'] = (
            preflight + adapter.AGGREGATE_START + '\n' +
            ''.join('RUN ' + name + '\nPASSED ' + name + '\n' for name in commands) +
            json.dumps(summary, sort_keys=True) + '\n').encode()
        builds = {str(flag): {'feature_enabled': bool(flag), 'image': release['firmware.bin'] if flag else digest(b'off'),
                  'effective_options': {'default_arduino_enabled': bool(flag), 'fqbn': comparison.FQBN,
                                        'explicit_override': None if flag else 0, 'options_sha256': 'f' * 64}}
                  for flag in (0, 1)}
        target_builds = {'source_commit': comparison.SOURCE_COMMIT, 'source_tree': comparison.SOURCE_TREE,
            'core': '3.3.12', 'compiler': 'esp-x32/2601', 'fqbn': comparison.FQBN,
            'default_arduino_enabled': True, 'builds': builds, 'compiled_budget': {'example_finite_ratio': 1.25}}
        identity = {'benchmark_only': True, 'cannot_publish': True, 'published': False,
            'benchmark_context': context, 'source_commit': comparison.SOURCE_COMMIT, 'source_tree': comparison.SOURCE_TREE,
            'orchestrator_ref': adapter.BENCHMARK_REF, 'leak_detection': True, 'artifacts': release,
            'adapter_manifest_sha256': digest(members['adapter/adapter-manifest.json'])['sha256'],
            'target_builds': target_builds, 'aggregate': summary,
            'synthetic_gate': {'sources_sha256': self.manifest['files']}}
        members['source/build/coopdiag77-benchmark-identity.json'] = data(identity)
        members['source/build/coopdiag77-esp32-build-evidence.json'] = data(target_builds)
        partial = {'schema': 1, 'kind': 'benchmark-partial', 'phase': role,
            'benchmark_only': True, 'cannot_publish': True, 'context': context, 'identity': self.identity,
            'cache': {'key': comparison.cache_key(self.identity, pin['context']['benchmark_salt'], 101, 2),
                'benchmark_salt': pin['context']['benchmark_salt'], 'hit': role == 'warm', 'restored': role == 'warm',
                'contents_verified': True, 'forbidden_paths': [],
                'bytes': sum(item['bytes'] for item in json.loads((target.HERE / 'dependencies/downloads.lock.json').read_bytes())['artifacts'])},
            'gates': {name: 'passed' for name in comparison.GATES},
            'coverage': {'groups': 157, 'skipped': 0, 'resumed': 0, 'lsan': 'detect_leaks=1',
                         'asan': True, 'ubsan_halt_on_error': True, 'assertions': True},
            'phases_ns': {name: 1_000_000_000 for name in target.PARTIAL_PHASES}, 'release_files': release}
        members['measurement.partial.json'] = data(partial)
        members['runner-status.json'] = data({'benchmark_only': True, 'cannot_publish': True,
                                              'status': 'passed', 'phases_ns': partial['phases_ns']})
        members['phase-times.json'] = data(partial['phases_ns'])
        members['dependencies.json'] = data({'schema': 1, 'phase': role, 'status': 'verified_and_installed',
            'context': pin['context'], 'lock_sha256': pin['lock_sha256'], 'cache_key': partial['cache']['key'],
            'cache_hit': role == 'warm', 'benchmark_salt': pin['context']['benchmark_salt'],
            'cache_bytes': partial['cache']['bytes'], 'dependency_install_ns': target.NS,
            'dependency_verify_ns': target.NS, 'total_ns': 2 * target.NS,
            'download_ns': target.NS if role == 'cold' else 0,
            'cache': {'key': partial['cache']['key'], 'hit': role == 'warm', 'restored': role == 'warm',
                      'lock_sha256': pin['lock_sha256'], 'files': pin['artifact_count'], 'bytes': partial['cache']['bytes']}})
        self.refresh_inventory(members)
        return members

    @staticmethod
    def refresh_inventory(members):
        members['source-artifact-inventory.json'] = data({name.removeprefix('source/'): digest(value)
                                                         for name, value in members.items() if name.startswith('source/')})

    def change_json(self, path, change, role='cold', refresh=False):
        value = json.loads(self.members[role][path])
        change(value)
        self.members[role][path] = data(value)
        if refresh:
            self.refresh_inventory(self.members[role])

    def write(self, *, descriptors=False, directory=False, zip64=False):
        for role in ('cold', 'warm'):
            payload = zip_bytes(self.members[role].items(), descriptors=descriptors, directory=directory, zip64=zip64)
            (self.root / (role + '.zip')).write_bytes(payload)
            self.meta[role].update(digest='sha256:' + digest(payload)['sha256'], size_in_bytes=len(payload))
        self.write_metadata()
        (self.root / 'jobs.json').write_bytes(data(self.pages))

    def write_metadata(self):
        for role in ('cold', 'warm'):
            (self.root / (role + '.json')).write_bytes(data(self.meta[role]))

    def collect(self, **kwargs):
        return target.collect(self.root / 'cold.zip', self.root / 'cold.json', '301',
                              self.root / 'warm.zip', self.root / 'warm.json', '302', self.root / 'jobs.json',
                              environment=self.environment, now=self.now, **kwargs)

    def cli_args(self, output):
        return ['--cold-zip', str(self.root / 'cold.zip'), '--cold-metadata', str(self.root / 'cold.json'), '--cold-id', '301',
                '--warm-zip', str(self.root / 'warm.zip'), '--warm-metadata', str(self.root / 'warm.json'), '--warm-id', '302',
                '--jobs', str(self.root / 'jobs.json'), '--output', str(output)]


class Tests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.fixture = Fixture(Path(self.temporary.name))
        self.fixture.write()

    def reject(self, message=None):
        context = self.assertRaisesRegex(target.Rejected, message) if message else self.assertRaises((target.Rejected, adapter.Rejected, comparison.Rejected))
        with context:
            self.fixture.collect()

    def test_valid_streaming_zip64_pair_preserves_bounds_and_precision(self):
        self.fixture.write(descriptors=True, directory=True, zip64=True)
        result = self.fixture.collect()
        self.assertFalse(result['authenticated_provenance'])
        self.assertFalse(result['publish_authorized'])
        self.assertTrue(result['comparison']['controlled_pair_consistent'])
        self.assertEqual(result['cold']['run']['id'], 101)
        self.assertEqual(result['warm']['cache']['writer_job_id'], 201)
        self.assertEqual(result['cold']['phases_ns']['dependency_restore'], 0)
        self.assertEqual(result['warm']['phases_ns']['cache_save'], 0)
        self.assertEqual(result['cold']['phases_ns']['dependency_verify'], 6 * target.NS)
        self.assertEqual(result['timing_method']['api_timestamp_resolution_ns'], target.NS)
        self.assertEqual(result['cold']['job_ns'], 100 * target.NS)

    def test_digest_size_required_before_zip_consumption(self):
        for key, value in [('digest', 'sha256:' + '0' * 64), ('digest', None), ('size_in_bytes', True), ('size_in_bytes', 1)]:
            with self.subTest(key=key, value=value):
                self.fixture.write()
                self.fixture.meta['cold'][key] = value
                self.fixture.write_metadata()
                with patch.object(target.Archive, 'read', side_effect=AssertionError('ZIP must not be consumed')):
                    self.reject()

    def test_wrong_api_identity_and_fork(self):
        mutations = [('id', 999), ('name', 'unbound'), ('expired', True),
                     ('expires_at', '2026-10-15T12:09:59Z'), ('created_at', '2026-10-15T11:39:59Z')]
        original = deepcopy(self.fixture.meta['cold'])
        for key, value in mutations:
            with self.subTest(key=key):
                self.fixture.meta['cold'] = deepcopy(original)
                self.fixture.meta['cold'][key] = value
                self.fixture.write_metadata()
                self.reject()
        for key, value in [('id', 102), ('repository_id', 999), ('head_repository_id', 999),
                           ('id', True), ('head_sha', 'e' * 40), ('head_branch', 'main')]:
            with self.subTest(workflow=key):
                self.fixture.meta['cold'] = deepcopy(original)
                self.fixture.meta['cold']['workflow_run'][key] = value
                self.fixture.write_metadata()
                self.reject()

    def test_real_environment_binding_and_strict_numeric_conversion(self):
        for key in self.fixture.environment:
            original = self.fixture.environment[key]
            self.fixture.environment[key] = 'wrong'
            with self.subTest(key=key):
                self.reject()
            self.fixture.environment[key] = original
        for value in ('01', '0', '-1', '+1', '1.0', ' 1', True, 1):
            with self.subTest(value=value), self.assertRaises(target.Rejected):
                target.numeric_id(value, 'test')

    def test_jobs_wrong_run_attempt_head_status_and_names(self):
        original = deepcopy(self.fixture.jobs[0])
        for key, value in [('run_id', 999), ('run_attempt', 1), ('head_sha', 'f' * 40),
            ('head_branch', 'main'), ('status', 'in_progress'), ('conclusion', 'failure'),
            ('id', True), ('name', 'cold (matrix)')]:
            with self.subTest(key=key):
                self.fixture.jobs[0] = deepcopy(original)
                self.fixture.jobs[0][key] = value
                self.fixture.pages[0]['jobs'] = self.fixture.jobs
                self.fixture.write()
                self.reject()

    def test_jobs_incomplete_duplicate_or_ambiguous_pages(self):
        jobs = deepcopy(self.fixture.jobs)
        invalid = [ {'total_count': 2, 'jobs': jobs}, [],
                    [{'total_count': 3, 'jobs': jobs}],
                    [{'total_count': 2, 'jobs': jobs[:1]}, {'total_count': 2, 'jobs': jobs[1:]}],
                    [{'total_count': 3, 'jobs': jobs + [deepcopy(jobs[0])]}],
                    [{'total_count': 3, 'jobs': jobs + [{**jobs[0], 'id': 999}]}] ]
        for pages in invalid:
            with self.subTest(pages=type(pages).__name__):
                self.fixture.pages = pages
                self.fixture.write()
                self.reject()

    def test_complete_multiple_pages_allow_running_collector(self):
        filler = [{**deepcopy(self.fixture.jobs[0]), 'id': 1000 + n, 'name': 'other-' + str(n),
                   'status': 'in_progress', 'conclusion': None} for n in range(99)]
        all_jobs = self.fixture.jobs + filler
        self.fixture.pages = [{'total_count': 101, 'jobs': all_jobs[:100]},
                              {'total_count': 101, 'jobs': all_jobs[100:]}]
        self.fixture.write()
        self.assertTrue(self.fixture.collect()['comparison']['controlled_pair_consistent'])

    def test_missing_zero_skipped_duplicate_and_wrong_cache_overhead(self):
        original = deepcopy(self.fixture.jobs)
        for role, step_name in ((0, 'Cache save'), (0, 'Verify distributions before save'),
                                (1, 'Cache restore'), (0, 'Upload benchmark evidence'),
                                (1, 'Upload benchmark evidence')):
            for kind in ('missing', 'zero', 'skipped', 'duplicate'):
                self.fixture.jobs = deepcopy(original)
                steps = self.fixture.jobs[role]['steps']
                step = next(item for item in steps if item['name'] == step_name)
                if kind == 'missing':
                    steps.remove(step)
                elif kind == 'zero':
                    if step_name == 'Verify distributions before save':
                        continue
                    step['completed_at'] = step['started_at']
                elif kind == 'skipped':
                    step['conclusion'] = 'skipped'
                else:
                    steps.append({**step, 'number': 999})
                self.fixture.pages[0]['jobs'] = self.fixture.jobs
                self.fixture.write()
                with self.subTest(role=role, step=step_name, kind=kind):
                    self.reject()

    def test_partial_schema_gate_coverage_cache_and_source_drift(self):
        original = self.fixture.members['cold']['measurement.partial.json']
        changes = [lambda x: x.update(extra=True), lambda x: x.update(benchmark_only=False),
                   lambda x: x['identity'].update(source_commit='0' * 40),
                   lambda x: x['identity'].update(dependency_lock_sha256='0' * 64),
                   lambda x: x['identity'].update(adapter_sha256='0' * 64),
                   lambda x: x['identity'].update(source_inventory_sha256='0' * 64),
                   lambda x: x['identity'].update(runner_image_version='unknown'),
                   lambda x: x['gates'].update(ota_sanitizers='skipped'),
                   lambda x: x['coverage'].update(resumed=1), lambda x: x['coverage'].update(skipped=False),
                   lambda x: x['cache'].update(key='fallback-key'), lambda x: x['cache'].update(contents_verified=False),
                   lambda x: x['phases_ns'].update(identity=0)]
        for change in changes:
            self.fixture.members['cold']['measurement.partial.json'] = original
            self.fixture.change_json('measurement.partial.json', change)
            self.fixture.write()
            self.reject()

    def test_payload_mutation_even_when_official_zip_digest_updated(self):
        for path in ('source/release/firmware.bin', 'source/release/artes.pak',
                     'source/release/manifest.json', 'adapter/benchmark_runtime.py'):
            original = self.fixture.members['cold'][path]
            self.fixture.members['cold'][path] = original + b'mutation'
            self.fixture.write()
            with self.subTest(path=path):
                self.reject()
            self.fixture.members['cold'][path] = original

    def test_aggregate_checkpoint_commands_logs_and_resume(self):
        root = 'source/build/package12-validation/ci-' + 'c' * 40 + '/'
        original = deepcopy(self.fixture.members['cold'])
        for change in [lambda x: x['preparation'].update(status='failed'),
                       lambda x: x['preparation'].update(commands=[['true']]),
                       lambda x: x['preparation'].update(log_sha256='0' * 64),
                       lambda x: x.pop('preparation')]:
            self.fixture.members['cold'] = deepcopy(original)
            self.fixture.change_json(root + 'checkpoints.json', change, refresh=True)
            self.fixture.write()
            self.reject()
        log = original['source/build/coopdiag77-tests.log']
        boundary = (adapter.AGGREGATE_START + '\n').encode()
        for log in (b'RESUME skip preparation\n', b'RUN preparation\n',
                    log.replace(boundary, boundary + b'RESUME skip preparation\n'),
                    log.replace(boundary + b'RUN preparation\n', boundary),
                    log.replace(boundary, boundary + b'RUN preparation\n'),
                    log.replace(b'PASSED preparation\n', b''),
                    log.replace(boundary, b''), log + boundary):
            self.fixture.members['cold'] = deepcopy(original)
            self.fixture.members['cold']['source/build/coopdiag77-tests.log'] = log
            self.fixture.refresh_inventory(self.fixture.members['cold'])
            self.fixture.write()
            self.reject()

    def test_final_identity_default_on_source_bindings_and_no_publication(self):
        path = 'source/build/coopdiag77-benchmark-identity.json'
        original = self.fixture.members['cold'][path]
        changes = [lambda x: x.update(published=True), lambda x: x.update(cannot_publish=False),
                   lambda x: x.update(source_tree='0' * 40), lambda x: x.update(leak_detection=False),
                   lambda x: x['target_builds'].update(default_arduino_enabled=False),
                   lambda x: x['target_builds']['builds']['1'].update(image=digest(b'wrong')),
                   lambda x: x['synthetic_gate']['sources_sha256'].update({next(iter(self.fixture.manifest['files'])): '0' * 64})]
        for change in changes:
            self.fixture.members['cold'][path] = original
            self.fixture.change_json(path, change, refresh=True)
            self.fixture.write()
            self.reject()

    def test_finite_float_evidence_allowed_but_partial_float_nan_and_duplicate_keys_rejected(self):
        self.fixture.collect()  # checkpoints and target budget have finite fractional values.
        for raw in (b'{"schema":1,"schema":1}', b'{"x":NaN}', b'{"x":Infinity}', b'{"x":1.5}'):
            self.fixture.members['cold']['measurement.partial.json'] = raw
            self.fixture.write()
            self.reject()
        with self.assertRaises(target.Rejected):
            target.json_data(b'{"x":1e999}', finite_floats=True)

    def test_zip_unsafe_paths_duplicate_symlink_executable_and_nonregular(self):
        for name in ('../escape', '/absolute', 'a/../b', 'a//b', './a', 'a\\b', 'C:evil'):
            entries = list(self.fixture.members['cold'].items()) + [(name, b'bad')]
            self.replace_zip(entries)
            with self.subTest(path=name):
                self.reject()
        entries = list(self.fixture.members['cold'].items())
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', UserWarning)
            self.replace_zip(entries + entries[:1])
        self.reject()
        for mode in (stat.S_IFLNK | 0o777, stat.S_IFREG | 0o755, stat.S_IFIFO | 0o600, stat.S_IFREG | 0o4644):
            self.replace_zip(entries, mode=mode)
            with self.subTest(mode=mode):
                self.reject()

    def replace_zip(self, entries, **kwargs):
        raw = zip_bytes(entries, **kwargs)
        (self.fixture.root / 'cold.zip').write_bytes(raw)
        self.fixture.meta['cold'].update(size_in_bytes=len(raw), digest='sha256:' + digest(raw)['sha256'])
        self.fixture.write_metadata()

    def test_final_command_contract_matches_reviewed_runner(self):
        import run_benchmark
        self.assertEqual(tuple(command for _, command in target.FINAL_COMMANDS), run_benchmark.FINAL_COMMANDS)

    def test_final_command_ledger_is_required_and_exact(self):
        original = self.fixture.members['cold'].pop('commands.json')
        self.fixture.write()
        self.reject()
        mutations = [lambda entries: entries.pop(), lambda entries: entries.reverse(),
                     lambda entries: entries.append(deepcopy(entries[0]))]
        for index in range(len(target.FINAL_COMMANDS)):
            mutations.extend([
                lambda entries, i=index: entries[i].update(argv=['true']),
                lambda entries, i=index: entries[i].update(cwd='/another/source'),
                lambda entries, i=index: entries[i].update(exit_code=1),
                lambda entries, i=index: entries[i].update(exit_code=False),
                lambda entries, i=index: entries[i].update(log='logs/another.log'),
            ])
        for mutation in mutations:
            self.fixture.members['cold']['commands.json'] = original
            self.fixture.change_json('commands.json', mutation)
            self.fixture.write()
            self.reject()

    def test_every_final_log_is_required_and_nonempty(self):
        for label, _ in target.FINAL_COMMANDS:
            path = 'logs/' + label + '.log'
            original = self.fixture.members['cold'].pop(path)
            self.fixture.write()
            with self.subTest(log=path, mutation='missing'):
                self.reject()
            self.fixture.members['cold'][path] = b''
            self.fixture.write()
            with self.subTest(log=path, mutation='empty'):
                self.reject()
            self.fixture.members['cold'][path] = original

    def test_final_log_mutation_breaks_official_zip_digest(self):
        for label, _ in target.FINAL_COMMANDS:
            self.fixture.write()
            path = 'logs/' + label + '.log'
            original = self.fixture.members['cold'][path]
            self.fixture.members['cold'][path] = original.replace(b'synthetic', b'corrupted')
            payload = zip_bytes(self.fixture.members['cold'].items())
            (self.fixture.root / 'cold.zip').write_bytes(payload)
            with self.subTest(log=path):
                self.reject()
            self.fixture.members['cold'][path] = original

    def test_dependency_and_runner_status_proofs_are_required(self):
        for path in ('dependencies.json', 'runner-status.json', 'phase-times.json'):
            original = self.fixture.members['cold'].pop(path)
            self.fixture.write()
            self.reject()
            self.fixture.members['cold'][path] = original
        self.fixture.change_json('dependencies.json', lambda value: value.update(cache_bytes=1))
        self.fixture.write()
        self.reject()

    def test_api_offset_and_zero_fraction_timestamps_normalize_to_utc(self):
        self.assertEqual(target.timestamp('2026-10-15T04:00:00.000-08:00', 'step'), self.fixture.now)
        for value in ('2026-10-15T12:00:00', '2026-10-15T12:00:00.123Z', '2026-99-99T12:00:00Z'):
            with self.subTest(value=value), self.assertRaises(target.Rejected):
                target.timestamp(value, 'step')

    def test_zero_second_presave_verification_is_recorded_as_coarse_observation(self):
        step = next(item for item in self.fixture.jobs[0]['steps'] if item['name'] == 'Verify distributions before save')
        step['completed_at'] = step['started_at']
        self.fixture.write()
        result = self.fixture.collect()
        self.assertEqual(result['cold']['phases_ns']['dependency_verify'], target.NS)
        self.assertIn('below the one-second', result['timing_method']['dependency_verify'])

    def test_nul_in_actual_zip_headers_is_rejected(self):
        raw = zip_bytes(list(self.fixture.members['cold'].items()) + [('nulXname', b'bad')])
        raw = raw.replace(b'nulXname', b'nul\x00name')
        (self.fixture.root / 'cold.zip').write_bytes(raw)
        self.fixture.meta['cold'].update(size_in_bytes=len(raw), digest='sha256:' + digest(raw)['sha256'])
        self.fixture.write_metadata()
        self.reject()

    def test_member_bounds_and_noncanonical_zip_layout(self):
        with patch.object(target, 'MAX_ENTRIES', 1):
            self.reject()
        with patch.object(target, 'MAX_EXPANDED', 100):
            self.reject()
        raw = (self.fixture.root / 'cold.zip').read_bytes() + b'trailer'
        (self.fixture.root / 'cold.zip').write_bytes(raw)
        self.fixture.meta['cold'].update(size_in_bytes=len(raw), digest='sha256:' + digest(raw)['sha256'])
        self.fixture.write_metadata()
        self.reject()

    def test_cli_output_and_rejections_work_with_python_optimized(self):
        for optimized in (False, True):
            output = self.fixture.root / ('result-' + str(optimized) + '.json')
            command = [sys.executable] + (['-O'] if optimized else []) + [str(target.HERE / 'collect_benchmark.py')] + self.fixture.cli_args(output)
            completed = subprocess.run(command, env={**os.environ, **self.fixture.environment}, capture_output=True, text=True)
            self.assertEqual(completed.returncode, 0, completed.stderr)
            report = json.loads(output.read_bytes())
            self.assertFalse(report['publish_authorized'])
            old = output.read_bytes()
            again = subprocess.run(command, env={**os.environ, **self.fixture.environment}, capture_output=True, text=True)
            self.assertNotEqual(again.returncode, 0)
            self.assertEqual(output.read_bytes(), old)
            # CLI entrypoint itself rejects unauthenticated/bad input under -O too.
            wrong = self.fixture.root / ('reject-' + str(optimized) + '.json')
            bad_command = [sys.executable] + (['-O'] if optimized else []) + [str(target.HERE / 'collect_benchmark.py')] + self.fixture.cli_args(wrong)
            rejected = subprocess.run(bad_command, env={**os.environ, 'GITHUB_ACTIONS': 'false'}, capture_output=True, text=True)
            self.assertNotEqual(rejected.returncode, 0)
            self.assertIn('REJECTED:', rejected.stderr)
            self.assertFalse(wrong.exists())


if __name__ == '__main__':
    unittest.main()
