#!/usr/bin/env python3
"""Synthetic offline tests. No firmware compilation, network, or publication."""
from copy import deepcopy
from datetime import datetime, timedelta, timezone
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import stat
import struct
import subprocess
import sys
import tempfile
import unittest
import warnings
import zipfile

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(ROOT / 'tools'))
import verify_candidate as subject


def raw(value):
    return (json.dumps(value, sort_keys=True, separators=(',', ':')) + '\n').encode()


def fixture():
    # Read the real frozen host command plan without running it. Synthetic logs
    # below are plainly fixtures, never evidence of actual firmware qualification.
    spec = importlib.util.spec_from_file_location('reference_plan', ROOT / 'tools/test_coopdiag77_local.py')
    runner = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(runner)
    preparation = [
        ['python3', 'tools/check_controller_snapshot.py'],
        ['python3', 'tools/unpack_art.py'],
        ['python3', 'tools/unpack_art.py', '--baseline-only', '--output', 'build/enemy_transition/old.pak'],
    ]
    preparation += [['python3', name, *args] for name, args in [
        ('tools/prepare_clock_art.py', ['--check']), ('tools/prepare_scene_art.py', ['--check']),
        ('tests/scene_asset_catalog.py', []),
        ('tools/prepare_enemy_art.py', ['--check', '--extract-reference', 'build/enemy_art_source']),
        ('tests/enemy_asset_catalog.py', []), ('tools/prepare_d20_mesh.py', ['--check']),
        ('tools/prepare_tent_marker.py', ['--check']), ('assets/reusable/effects01/check_library.py', []),
        ('assets/reusable/equipment01/check_library.py', []),
    ]]
    tasks = [{'id': 'preparation', 'commands': preparation}]
    tasks += [{'id': name, 'commands': commands} for name, commands in runner.tasks(False)]
    expected = {
        'repository': {'id': 1, 'full_name': 'Oflodorporto/RPG-POKET-2'},
        'workflow': {'id': 2, 'path': '.github/workflows/qualify-next.yml', 'sha256': 'c' * 64},
        'source': {'commit': 'a' * 40, 'tree': 'b' * 40, 'inventory_sha256': 'd' * 64, 'branch': 'feat/future-package'},
        'release': {'version': '2099.01.01-synthetic', 'build': 1, 'tag': 'v2099.01.01-synthetic'},
        'run': {'id': 3, 'attempt': 1},
        'artifact': {'id': 4, 'name': 'release-candidate-' + 'a' * 40 + '-3-1', 'sha256': '', 'bytes': 1},
        'task_ids': [task['id'] for task in tasks], 'plan_sha256': '',
    }
    files = {'release/firmware.bin': b'SYNTHETIC-NOT-FIRMWARE', 'release/artes.pak': b'SYNTHETIC-NOT-ART'}
    files['evidence/source-inventory.json'] = raw({'schema': 1, 'commit': expected['source']['commit'], 'tree': expected['source']['tree'], 'files': [{'path': 'firmware/SYNTHETIC.txt', 'bytes': 9, 'sha256': subject.sha256(b'SYNTHETIC'), 'executable': False}]})
    expected['source']['inventory_sha256'] = subject.sha256(files['evidence/source-inventory.json'])
    manifest = {'schema': 2, **{key: expected['release'][key] for key in ('version', 'build')}}
    for key, name in (('firmware', 'firmware.bin'), ('art', 'artes.pak')):
        data = files['release/' + name]
        manifest[key] = {'bytes': len(data), 'sha256': subject.sha256(data), 'url': 'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v2099.01.01-synthetic/' + name}
    files['release/manifest.json'] = raw(manifest)
    identity = {'schema': 1, **{key: deepcopy(expected[key]) for key in ('repository', 'workflow', 'source', 'release', 'run')}}
    identity['files'] = {name: {'bytes': len(files['release/' + name]), 'sha256': subject.sha256(files['release/' + name])} for name in subject.RELEASE_NAMES}
    identity['gates'] = {'esp32_build': 'passed', 'scope_inventory': 'passed', 'real_byte_ota_normal': 'passed', 'real_byte_ota_sanitizer': 'passed', 'asan': True, 'ubsan_halt_on_error': True, 'lsan_detect_leaks': True, 'assertions_enabled': True, 'aggregate_tasks': 157}
    files['evidence/identity.json'] = raw(identity)
    files['release/SHA256SUMS'] = ''.join('%s  %s\n' % (identity['files'][name]['sha256'], name) for name in subject.RELEASE_NAMES).encode()
    files['evidence/plan.json'] = raw({'schema': 1, 'source': expected['source'], 'sanitizers': {'ASAN_OPTIONS': 'detect_leaks=1', 'UBSAN_OPTIONS': 'halt_on_error=1', 'assertions': True}, 'tasks': tasks})
    expected['plan_sha256'] = subject.sha256(files['evidence/plan.json'])
    checkpoints = {}
    for task in tasks:
        log = ('SYNTHETIC FIXTURE ONLY, NOT EXECUTED: ' + task['id'] + '\n').encode()
        files['evidence/logs/' + task['id'] + '.log'] = log
        checkpoints[task['id']] = {'status': 'passed', 'commands': task['commands'], 'log_sha256': subject.sha256(log)}
    files['evidence/checkpoints.json'] = raw(checkpoints)
    observed = {
        'fetched_at': '2026-10-14T15:00:00Z', 'repository': deepcopy(expected['repository']),
        'run': {'id': 3, 'run_attempt': 1, 'workflow_id': 2, 'path': expected['workflow']['path'], 'event': 'push', 'head_sha': expected['source']['commit'], 'head_branch': expected['source']['branch'], 'repository': deepcopy(expected['repository']), 'head_repository': deepcopy(expected['repository']), 'status': 'completed', 'conclusion': 'success', 'run_started_at': '2026-10-14T14:00:00Z', 'completed_at': '2026-10-14T14:45:00Z'},
        'workflow_sha256': expected['workflow']['sha256'], 'artifact_candidates': [],
        'main_sha': expected['source']['commit'], 'source_tree': expected['source']['tree'], 'inventory_sha256': expected['source']['inventory_sha256'],
        'tag_exists': False, 'release_exists': False, 'consumed_run_attempts': [],
    }
    observed['artifact_candidates'] = [{
        'id': 4, 'name': expected['artifact']['name'], 'size_in_bytes': 1, 'digest': '', 'expired': False,
        'created_at': '2026-10-14T14:44:00Z', 'expires_at': '2026-10-20T15:00:00Z',
        'workflow_run': {'id': 3, 'repository_id': 1, 'head_repository_id': 1, 'head_branch': expected['source']['branch'], 'head_sha': expected['source']['commit']},
    }]
    return files, {'schema': 1, 'expected': expected, 'observed': observed}


def zip_bytes(files, extra=(), modes=None, compression=zipfile.ZIP_STORED, comments=None):
    result = io.BytesIO()
    with warnings.catch_warnings():
        warnings.simplefilter('ignore', UserWarning)
        with zipfile.ZipFile(result, 'w', compression=compression) as archive:
            for name, data in [*files.items(), *extra]:
                info = zipfile.ZipInfo(name, date_time=(2026, 10, 14, 0, 0, 0))
                info.compress_type = compression
                info.create_system = 3
                info.comment = (comments or {}).get(name, b'')
                info.external_attr = ((modes or {}).get(name, stat.S_IFREG | 0o600)) << 16
                archive.writestr(info, data)
    return result.getvalue()


def bind_archive(context, archive):
    digest, size = subject.sha256(archive), len(archive)
    context['expected']['artifact'].update(sha256=digest, bytes=size)
    context['observed']['artifact_candidates'][0].update(digest='sha256:' + digest, size_in_bytes=size)


NOW = datetime(2026, 10, 14, 15, 0, 0, tzinfo=timezone.utc)


class CandidateTests(unittest.TestCase):
    def setUp(self):
        self.files, self.context = fixture()
        self.archive = zip_bytes(self.files)
        bind_archive(self.context, self.archive)

    def check(self, archive=None):
        return subject.verify(self.archive if archive is None else archive, self.context, NOW)

    def rezip(self, **kwargs):
        self.archive = zip_bytes(self.files, **kwargs)
        bind_archive(self.context, self.archive)

    def mutate_json(self, path, change):
        item = json.loads(self.files[path])
        change(item)
        self.files[path] = raw(item)
        self.rezip()

    def test_valid_fixture_never_authorizes(self):
        result = self.check()
        self.assertTrue(result['offline_checks_passed'])
        for key in ('publish_authorized', 'authenticated_provenance', 'lease_consumed', 'real_byte_ota_executed_by_this_tool'):
            self.assertIs(result[key], False)

    def test_all_metadata_mutations_fail(self):
        mutations = [
            ('observed.run.id', 99), ('observed.run.run_attempt', 2), ('observed.run.workflow_id', 99),
            ('observed.run.path', '.github/workflows/other.yml'), ('observed.run.event', 'pull_request'),
            ('observed.run.event', 'workflow_run'), ('observed.run.head_branch', 'main'), ('observed.run.head_sha', 'f' * 40),
            ('observed.run.head_repository.id', 99), ('observed.run.head_repository.full_name', 'fork/RPG-POKET-2'),
            ('observed.run.repository.id', 99), ('observed.run.status', 'in_progress'), ('observed.run.conclusion', 'failure'),
            ('observed.run.conclusion', 'cancelled'), ('observed.run.conclusion', None),
            ('observed.main_sha', 'f' * 40), ('observed.source_tree', 'f' * 40), ('observed.inventory_sha256', 'f' * 64),
            ('observed.workflow_sha256', 'f' * 64), ('observed.tag_exists', True), ('observed.release_exists', True),
            ('observed.tag_exists', 0), ('observed.consumed_run_attempts', ['3:1']),
            ('observed.fetched_at', '2026-10-14T14:54:59Z'), ('observed.fetched_at', '2026-10-14T15:00:01Z'),
            ('observed.run.completed_at', '2026-10-14T15:00:01Z'), ('observed.run.run_started_at', '2026-10-14T14:46:00Z'),
            ('expected.run.id', True), ('expected.run.attempt', True), ('expected.repository.id', True),
            ('observed.repository.id', True), ('observed.run.head_repository.id', True),
            ('expected.release.build', True), ('expected.release.tag', 'vwrong'), ('expected.source.branch', '../../evil'),
            ('expected.workflow.path', '.github/workflows/../evil.yml'), ('expected.plan_sha256', ''),
            ('observed.fetched_at', '2026-02-30T15:00:00Z'), ('observed.fetched_at', '2026-10-14T15:00:00'),
        ]
        original = deepcopy(self.context)
        for path, value in mutations:
            with self.subTest(path=path, value=value):
                self.context = deepcopy(original)
                target = self.context
                parts = path.split('.')
                for part in parts[:-1]:
                    target = target[part]
                target[parts[-1]] = value
                with self.assertRaises(subject.Rejected):
                    self.check()

    def test_artifact_mutations_fail(self):
        original = deepcopy(self.context)
        changes = [('id', 99), ('name', 'latest'), ('digest', None), ('digest', 'sha256:' + 'f' * 64), ('size_in_bytes', 5), ('expired', True), ('expired', 0), ('created_at', '2026-10-14T13:59:59Z'), ('created_at', '2026-10-14T14:45:01Z'), ('expires_at', '2026-10-14T15:10:00Z'), ('expires_at', '2026-10-01T00:00:00Z')]
        for field, value in changes:
            with self.subTest(field=field):
                self.context = deepcopy(original)
                self.context['observed']['artifact_candidates'][0][field] = value
                with self.assertRaises(subject.Rejected):
                    self.check()
        for field, value in [('id', 9), ('repository_id', 9), ('head_repository_id', 9), ('head_repository_id', True), ('head_sha', 'f' * 40), ('head_branch', 'main')]:
            with self.subTest(binding=field):
                self.context = deepcopy(original)
                self.context['observed']['artifact_candidates'][0]['workflow_run'][field] = value
                with self.assertRaises(subject.Rejected):
                    self.check()

    def test_missing_ambiguous_candidate(self):
        candidate = deepcopy(self.context['observed']['artifact_candidates'][0])
        for candidates in ([], [candidate, candidate]):
            self.context['observed']['artifact_candidates'] = candidates
            with self.assertRaises(subject.Rejected):
                self.check()

    def test_observation_cannot_precede_claimed_events(self):
        self.context['observed']['fetched_at'] = '2026-10-14T14:55:00Z'
        self.context['observed']['run']['completed_at'] = '2026-10-14T14:59:00Z'
        self.context['observed']['artifact_candidates'][0]['created_at'] = '2026-10-14T14:58:00Z'
        with self.assertRaises(subject.Rejected):
            self.check()

    def test_stale_qualification(self):
        self.context['observed']['run']['run_started_at'] = '2026-10-10T14:00:00Z'
        self.context['observed']['run']['completed_at'] = '2026-10-10T14:45:00Z'
        with self.assertRaises(subject.Rejected):
            self.check()

    def test_transport_tamper_and_size(self):
        for changed in (self.archive + b'x', self.archive[:-1], bytes([self.archive[0] ^ 1]) + self.archive[1:]):
            with self.assertRaises(subject.Rejected):
                self.check(changed)

    def test_exact_source_and_gate_claims(self):
        originals = deepcopy(self.files)
        for key, value in [('source', {'commit': 'f' * 40}), ('run', {'id': 3, 'attempt': 2}), ('release', {'version': 'old'}), ('files', {}), ('schema', True)]:
            with self.subTest(key=key):
                self.files = deepcopy(originals)
                self.mutate_json('evidence/identity.json', lambda item: item.update({key: value}))
                with self.assertRaises(subject.Rejected):
                    self.check()
        for gate, value in [('asan', False), ('ubsan_halt_on_error', False), ('lsan_detect_leaks', False), ('assertions_enabled', False), ('aggregate_tasks', 142), ('real_byte_ota_sanitizer', 'skipped'), ('esp32_build', 'skipped'), ('scope_inventory', 'skipped'), ('asan', 1)]:
            with self.subTest(gate=gate):
                self.files = deepcopy(originals)
                self.mutate_json('evidence/identity.json', lambda item: item['gates'].update({gate: value}))
                with self.assertRaises(subject.Rejected):
                    self.check()

    def test_manifest_contract_after_hashes_are_rebound(self):
        original = deepcopy(self.files)
        mutations = [
            lambda item: item.update(version='old'), lambda item: item.update(build=2),
            lambda item: item.update(schema=True), lambda item: item.update(build=True),
            lambda item: item['firmware'].update(url='https://evil.example/firmware.bin'),
            lambda item: item['firmware'].update(sha256='f' * 64),
            lambda item: item['art'].update(bytes=999),
        ]
        for mutation in mutations:
            self.files = deepcopy(original)
            manifest = json.loads(self.files['release/manifest.json'])
            mutation(manifest)
            self.files['release/manifest.json'] = raw(manifest)
            identity = json.loads(self.files['evidence/identity.json'])
            identity['files']['manifest.json'] = {'bytes': len(self.files['release/manifest.json']), 'sha256': subject.sha256(self.files['release/manifest.json'])}
            self.files['evidence/identity.json'] = raw(identity)
            self.files['release/SHA256SUMS'] = ''.join('%s  %s\n' % (identity['files'][name]['sha256'], name) for name in subject.RELEASE_NAMES).encode()
            self.rezip()
            with self.assertRaises(subject.Rejected):
                self.check()

    def test_missing_context_field_and_extra_fields(self):
        original = deepcopy(self.context)
        for key in original['expected']:
            self.context = deepcopy(original)
            del self.context['expected'][key]
            with self.assertRaises(subject.Rejected):
                self.check()
        self.context = deepcopy(original)
        self.context['publish_authorized'] = True
        with self.assertRaises(subject.Rejected):
            self.check()

    def test_plan_cannot_claim_disabled_sanitizers(self):
        self.mutate_json('evidence/plan.json', lambda item: item['sanitizers'].update(ASAN_OPTIONS='detect_leaks=0'))
        self.context['expected']['plan_sha256'] = subject.sha256(self.files['evidence/plan.json'])
        with self.assertRaises(subject.Rejected):
            self.check()

    def test_zip_nul_names_and_metadata_count(self):
        for changed in (self.archive.replace(b'release/firmware.bin', b'release/firmware.b\x00n'), self.archive[:-12] + b'\xff\xff' + self.archive[-10:]):
            bind_archive(self.context, changed)
            with self.assertRaises(subject.Rejected):
                self.check(changed)

    def test_inventory_tamper_with_valid_transport(self):
        self.mutate_json('evidence/source-inventory.json', lambda item: item['files'][0].update(sha256='f' * 64))
        with self.assertRaises(subject.Rejected):
            self.check()

    def test_payload_hash_tamper_with_valid_transport(self):
        self.files['release/firmware.bin'] += b'evil'
        self.rezip()
        with self.assertRaises(subject.Rejected):
            self.check()

    def test_plan_tamper_even_when_transport_rebound(self):
        self.mutate_json('evidence/plan.json', lambda item: item['tasks'][0]['commands'].append(['echo', 'fake-pass']))
        with self.assertRaises(subject.Rejected):
            self.check()

    def test_checkpoint_failed_missing_commands_or_log(self):
        originals = deepcopy(self.files)
        changes = [lambda x: x.pop('preparation'), lambda x: x['preparation'].update(status='failed'), lambda x: x['preparation'].update(status='skipped'), lambda x: x['preparation'].update(commands=[['echo', 'pass']]), lambda x: x['preparation'].update(log_sha256='f' * 64)]
        for change in changes:
            self.files = deepcopy(originals)
            self.mutate_json('evidence/checkpoints.json', change)
            with self.assertRaises(subject.Rejected):
                self.check()
        self.files = deepcopy(originals)
        self.files['evidence/logs/preparation.log'] += b'tampered'
        self.rezip()
        with self.assertRaises(subject.Rejected):
            self.check()

    def test_member_names_traversal_absolute_backslash_extra_duplicate(self):
        for name in ('../escape', '/tmp/escape', 'release/../firmware.bin', 'release\\firmware.bin', 'release/./firmware.bin', 'release/FIRMWARE.BIN', 'unexpected', 'release/firmware.bin'):
            with self.subTest(name=name):
                self.rezip(extra=[(name, b'evil')])
                with self.assertRaises(subject.Rejected):
                    self.check()

    def test_symlink_fifo_executable_member(self):
        for mode in (stat.S_IFLNK | 0o777, stat.S_IFIFO | 0o600, stat.S_IFREG | 0o700, stat.S_IFDIR | 0o700):
            self.rezip(modes={'release/firmware.bin': mode})
            with self.assertRaises(subject.Rejected):
                self.check()

    def test_missing_member_and_oversize(self):
        self.files.pop('release/SHA256SUMS')
        self.rezip()
        with self.assertRaises(subject.Rejected):
            self.check()
        self.files, self.context = fixture()
        self.files['release/manifest.json'] = b'x' * 4097
        self.rezip()
        with self.assertRaises(subject.Rejected):
            self.check()

    def test_zip_member_comment_forbidden(self):
        self.rezip(comments={'release/firmware.bin': b'comment'})
        with self.assertRaises(subject.Rejected):
            self.check()

    def test_zip_expansion_bomb(self):
        self.files['evidence/logs/preparation.log'] = b'x' * (1024 * 1024)
        self.rezip(compression=zipfile.ZIP_DEFLATED)
        with self.assertRaises(subject.Rejected):
            self.check()

    def test_zip_prefix_trailer_and_comment_forbidden(self):
        for changed in (b'prefix' + self.archive, self.archive + b'trailer', b'PK\x03\x04prefix' + self.archive):
            bind_archive(self.context, changed)
            with self.assertRaises(subject.Rejected):
                self.check(changed)

    def test_corrupt_deflate_is_structured_rejection(self):
        archive = bytearray(zip_bytes(self.files, compression=zipfile.ZIP_DEFLATED))
        # First local header: 30 bytes, then filename and no extra fields.
        offset = 30 + len('release/firmware.bin')
        archive[offset:offset + 3] = b'\xff\xff\xff'
        archive = bytes(archive)
        bind_archive(self.context, archive)
        with self.assertRaises(subject.Rejected):
            self.check(archive)

    def test_local_zip64_forbidden(self):
        result = io.BytesIO()
        with zipfile.ZipFile(result, 'w') as archive:
            for name, data in self.files.items():
                with archive.open(name, 'w', force_zip64=True) as member:
                    member.write(data)
        archive = result.getvalue()
        bind_archive(self.context, archive)
        with self.assertRaises(subject.Rejected):
            self.check(archive)

    def test_hidden_local_member_forbidden(self):
        hidden_archive = zip_bytes({'../hidden-member': b'evil'})
        hidden_central = struct.unpack_from('<I', hidden_archive, len(hidden_archive) - 6)[0]
        hidden_local = hidden_archive[:hidden_central]
        central = struct.unpack_from('<I', self.archive, len(self.archive) - 6)[0]
        archive = bytearray(self.archive[:central] + hidden_local + self.archive[central:])
        struct.pack_into('<I', archive, len(archive) - 6, central + len(hidden_local))
        archive = bytes(archive)
        bind_archive(self.context, archive)
        with self.assertRaises(subject.Rejected):
            self.check(archive)

    def test_invalid_zip_with_bound_digest(self):
        self.archive = b'not a zip'
        bind_archive(self.context, self.archive)
        with self.assertRaises(subject.Rejected):
            self.check()

    def test_duplicate_json_nonfinite_malformed(self):
        for value in (b'{"schema":1,"schema":1}', b'{"x":NaN}', b'{"x":Infinity}', b'{"x":1e999}', b'{"x":1.0}', b'{', b'\xff', b'[' * 2000):
            with self.subTest(value=value[:30]):
                with self.assertRaises(subject.Rejected):
                    subject.strict_json(value)

    def test_no_weakening_via_task_count_and_types(self):
        for tasks in (self.context['expected']['task_ids'][:-1], ['preparation'] * 157, [True] * 157):
            self.context['expected']['task_ids'] = tasks
            with self.assertRaises(subject.Rejected):
                self.check()

    def test_cli_is_read_only_and_optimized_mode_fails_closed(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            archive_path, context_path = folder / 'candidate.zip', folder / 'context.json'
            archive_path.write_bytes(self.archive)
            # Real current time is used by CLI, so a fresh snapshot is constructed.
            now = datetime.now(timezone.utc).replace(microsecond=0)
            def stamp(value):
                return value.strftime('%Y-%m-%dT%H:%M:%SZ')
            observation = self.context['observed']
            observation['fetched_at'] = stamp(now)
            observation['run']['run_started_at'] = stamp(now - timedelta(hours=1))
            observation['run']['completed_at'] = stamp(now - timedelta(minutes=1))
            observation['artifact_candidates'][0]['created_at'] = stamp(now - timedelta(minutes=2))
            observation['artifact_candidates'][0]['expires_at'] = stamp(now + timedelta(days=1))
            context_path.write_bytes(raw(self.context))
            for option in ([], ['-O']):
                command = [sys.executable, *option, str(HERE / 'verify_candidate.py'), '--archive', str(archive_path), '--context', str(context_path)]
                result = subprocess.run(command, cwd=folder, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
                self.assertIs(json.loads(result.stdout)['publish_authorized'], False)
                before = {p.name: p.read_bytes() for p in folder.iterdir()}
                observation['tag_exists'] = True
                context_path.write_bytes(raw(self.context))
                result = subprocess.run(command, cwd=folder, capture_output=True, text=True)
                self.assertEqual(result.returncode, 1, result.stderr + result.stdout)
                self.assertIs(json.loads(result.stdout)['publish_authorized'], False)
                self.assertEqual(set(p.name for p in folder.iterdir()), set(before))
                self.assertEqual(archive_path.read_bytes(), self.archive)
                observation['tag_exists'] = False
                context_path.write_bytes(raw(self.context))

    def test_local_symlink_input_refused(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            (folder / 'regular').write_bytes(b'contents')
            (folder / 'link').symlink_to(folder / 'regular')
            with self.assertRaises(subject.Rejected):
                subject.read_regular(folder / 'link', 100)
            with self.assertRaises(subject.Rejected):
                subject.read_regular(folder, 100)


if __name__ == '__main__':
    unittest.main(verbosity=2)
