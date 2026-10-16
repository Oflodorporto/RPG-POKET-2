#!/usr/bin/env python3
"""Offline, fail-closed collector for a single benchmark-only Actions attempt.

The caller downloads official API metadata, attempt-specific jobs pages and ZIPs.
This module makes no network calls, extracts nothing and executes no artifact code.
Locally supplied JSON is not an independently authenticated attestation.
"""
import argparse
from contextlib import ExitStack
from datetime import datetime, timedelta, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import re
import stat
import struct
import zipfile

import benchmark_adapter as adapter
import compare_benchmark as comparison

HERE = Path(__file__).resolve().parent
NS = 1_000_000_000
CHUNK = 1024 * 1024
MAX_ARCHIVE = 8 * 1024**3
MAX_EXPANDED = 16 * 1024**3
MAX_MEMBER = 512 * 1024**2
MAX_JSON = 32 * 1024**2
MAX_LOG = 256 * 1024**2
MAX_DIRECTORY = 64 * 1024**2
MAX_ENTRIES = 100_000
PARTIAL_PHASES = ('dependency_install', 'dependency_verify', 'aggregate',
                  'esp32_on', 'esp32_off', 'final_bytes_ota', 'identity')
RELEASE_NAMES = ('firmware.bin', 'artes.pak', 'manifest.json')
FINAL_COMMANDS = (
    ('release-manifest', ('python3', 'tools/release_coopdiag77_manifest.py')),
    ('release-verify', ('python3', 'tools/verify_coopdiag77_release.py')),
    ('release-verifier-tests', ('python3', 'tests/coopdiag77_release_verifier.py')),
    ('ota-normal', ('python3', 'tools/test_coopdiag77_release.py', 'release')),
    ('ota-sanitizers', ('python3', 'tools/test_coopdiag77_release.py', 'release', '--sanitizers')),
)


class Rejected(ValueError):
    pass


def require(value, message):
    if not value:
        raise Rejected(message)


def exact(value, names, label):
    require(type(value) is dict and set(value) == set(names), label + ': exact fields required')


def positive(value, label):
    require(type(value) is int and value > 0, label + ': positive integer required')
    return value


def numeric_id(value, label):
    require(type(value) is str and re.fullmatch(r'[1-9][0-9]{0,19}', value),
            label + ': canonical positive decimal string required')
    return int(value)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def json_data(data, *, finite_floats=False):
    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, 'Duplicate JSON key: ' + key)
            result[key] = value
        return result
    def bad(value):
        raise Rejected('Forbidden floating-point/nonfinite JSON value: ' + value)
    def floating(value):
        result = float(value)
        require(math.isfinite(result), 'Nonfinite JSON number')
        return result
    try:
        return json.loads(data, object_pairs_hook=pairs,
                          parse_float=floating if finite_floats else bad, parse_constant=bad)
    except (UnicodeError, RecursionError, json.JSONDecodeError) as error:
        raise Rejected('Invalid or excessively nested JSON') from error


def local_json(path):
    with Path(path).open('rb') as stream:
        data = stream.read(MAX_JSON + 1)
    require(len(data) <= MAX_JSON, 'Local JSON exceeds bound')
    return json_data(data)


def timestamp(value, label):
    require(type(value) is str and re.fullmatch(r'\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(?:\.0{1,9})?(?:Z|[+-]\d\d:\d\d)', value),
            label + ': whole-second ISO-8601 API timestamp required')
    try:
        return datetime.fromisoformat(value.replace('Z', '+00:00')).astimezone(timezone.utc)
    except ValueError as error:
        raise Rejected('Invalid ' + label) from error


def interval(record, label, *, allow_zero=False):
    start = timestamp(record.get('started_at'), label + ' start')
    end = timestamp(record.get('completed_at'), label + ' completion')
    require(end >= start if allow_zero else end > start, label + ': invalid API duration')
    return start, end, int((end - start).total_seconds()) * NS


def safe_name(name, *, directory=False):
    require(type(name) is str and name and len(name.encode('utf-8')) <= 2048,
            'Invalid ZIP/path name')
    require(not any(ord(c) < 32 or ord(c) == 127 for c in name) and
            '\\' not in name and ':' not in name, 'Unsafe ZIP/path characters')
    body = name[:-1] if directory and name.endswith('/') else name
    require(not name.startswith('/') and all(p not in ('', '.', '..') for p in body.split('/')),
            'Unsafe ZIP/path traversal or alias')
    require(not body.endswith('/'), 'Malformed directory name')
    return body


def zip_directory_bound(stream, size):
    """Bound zipfile's central-directory allocation, including Actions ZIP64."""
    stream.seek(max(0, size - 65557))
    tail = stream.read(65557)
    position = tail.rfind(b'PK\x05\x06')
    require(position >= 0 and len(tail) - position >= 22, 'Missing ZIP end record')
    record = struct.unpack('<4s4H2IH', tail[position:position + 22])
    _, disk, directory_disk, disk_count, count, length, offset, comment = record
    eocd = size - len(tail) + position
    require(eocd + 22 + comment == size and disk == directory_disk == 0,
            'Truncated, trailing or multi-disk ZIP')
    boundary = eocd
    if count == 0xffff or length == 0xffffffff or offset == 0xffffffff:
        require(eocd >= 20, 'Missing ZIP64 locator')
        stream.seek(eocd - 20)
        locator = stream.read(20)
        signature, disk, zoffset, disks = struct.unpack('<4sIQI', locator)
        require(signature == b'PK\x06\x07' and disk == 0 and disks == 1,
                'Invalid ZIP64 locator')
        require(zoffset + 56 <= eocd - 20, 'Invalid ZIP64 offset')
        stream.seek(zoffset)
        raw = stream.read(56)
        require(len(raw) == 56, 'Truncated ZIP64 end record')
        sig, zlength, _made, _needed, zdisk, zdir, zcount_disk, count, length, offset = struct.unpack('<4sQ2H2I4Q', raw)
        require(sig == b'PK\x06\x06' and 44 <= zlength <= 4096 and
                zoffset + 12 + zlength == eocd - 20 and zdisk == zdir == 0 and zcount_disk == count,
                'Invalid ZIP64 end record')
        boundary = zoffset
    else:
        require(disk_count == count, 'Incomplete ZIP directory')
    require(0 < count <= MAX_ENTRIES and 0 < length <= MAX_DIRECTORY and
            offset >= 0 and offset + length == boundary, 'ZIP directory exceeds bounds or is misplaced')
    return count, offset


class Archive:
    """Hold one already digest-verified file descriptor; never extract members."""
    def __init__(self, path, metadata):
        self.stream = Path(path).open('rb')
        self.zip = None
        try:
            size = os.fstat(self.stream.fileno()).st_size
            require(0 < size <= MAX_ARCHIVE and size == metadata.get('size_in_bytes') and
                    type(metadata.get('size_in_bytes')) is int, 'Artifact ZIP byte size mismatch')
            digest = metadata.get('digest')
            require(type(digest) is str and re.fullmatch(r'sha256:[0-9a-f]{64}', digest),
                    'Missing official SHA-256 artifact digest')
            hasher = hashlib.sha256()
            counted = 0
            for block in iter(lambda: self.stream.read(CHUNK), b''):
                counted += len(block)
                require(counted <= size, 'ZIP changed while hashing')
                hasher.update(block)
            require(counted == size and 'sha256:' + hasher.hexdigest() == digest,
                    'Artifact ZIP SHA-256 mismatch')
            self.sha256, self.size = hasher.hexdigest(), size
            count, directory_offset = zip_directory_bound(self.stream, size)
            self.stream.seek(0)
            self.zip = zipfile.ZipFile(self.stream)
            infos = self.zip.infolist()
            require(len(infos) == count, 'ZIP directory entry count mismatch')
            self.members, paths, ranges, total = {}, {}, [], 0
            for item in infos:
                require(item.filename == item.orig_filename, 'NUL-truncated ZIP filename')
                name = safe_name(item.filename, directory=item.is_dir())
                require(name not in paths, 'Duplicate ZIP path')
                paths[name] = item.is_dir()
                mode = item.external_attr >> 16
                kind = stat.S_IFMT(mode)
                require(kind in (0, stat.S_IFDIR if item.is_dir() else stat.S_IFREG),
                        'ZIP symlink or nonregular entry')
                require(not mode & 0o7000 and (item.is_dir() or not mode & 0o111) and not item.external_attr & 0x400,
                        'Executable/special ZIP metadata')
                require(not item.flag_bits & ~0x080e and not item.flag_bits & 0x0001,
                        'Encrypted or unsupported ZIP flags')
                require(item.compress_type in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED),
                        'Unsupported ZIP compression')
                require(0 <= item.file_size <= MAX_MEMBER and item.compress_size >= 0,
                        'ZIP member size exceeds bound')
                require(item.file_size <= max(CHUNK, item.compress_size * 1000), 'ZIP expansion ratio exceeds bound')
                if item.is_dir():
                    require(item.file_size == 0, 'Nonempty ZIP directory')
                else:
                    self.members[name] = item
                total += item.file_size
                require(total <= MAX_EXPANDED, 'Expanded ZIP exceeds bound')
                # Validate every local header, but permit streaming data descriptors.
                require(0 <= item.header_offset < directory_offset, 'Invalid ZIP local offset')
                self.stream.seek(item.header_offset)
                raw = self.stream.read(30)
                require(len(raw) == 30, 'Truncated ZIP local header')
                header = struct.unpack('<4s5H3I2H', raw)
                require(header[0] == b'PK\x03\x04' and header[2] == item.flag_bits and
                        header[3] == item.compress_type, 'ZIP local/central metadata differ')
                nlen, xlen = header[-2:]
                raw_name = self.stream.read(nlen)
                try:
                    local_name = raw_name.decode('utf-8' if item.flag_bits & 0x800 else 'cp437')
                except UnicodeError as error:
                    raise Rejected('Invalid ZIP local name') from error
                require(local_name == item.filename, 'ZIP local/central names differ')
                end = item.header_offset + 30 + nlen + xlen + item.compress_size
                require(end <= directory_offset, 'ZIP payload overlaps central directory')
                ranges.append((item.header_offset, end))
            for path in paths:
                parts = path.split('/')
                require(all(paths.get('/'.join(parts[:n]), True) for n in range(1, len(parts))),
                        'ZIP file/directory collision')
            ordered = sorted(ranges)
            require(all(a[1] <= b[0] for a, b in zip(ordered, ordered[1:])), 'Overlapping ZIP entries')
        except BaseException:
            self.close()
            raise

    def close(self):
        if self.zip is not None:
            self.zip.close()
        self.stream.close()

    def member(self, name, limit):
        require(name in self.members, 'Missing evidence member: ' + name)
        info = self.members[name]
        require(info.file_size <= limit, 'Evidence member exceeds read bound: ' + name)
        return info

    def read(self, name, limit=MAX_JSON):
        info = self.member(name, limit)
        with self.zip.open(info) as source:
            data = source.read(limit + 1)
        require(len(data) == info.file_size and len(data) <= limit, 'Truncated/oversized evidence: ' + name)
        return data

    def json(self, name, *, finite_floats=False):
        return json_data(self.read(name), finite_floats=finite_floats)

    def digest(self, name, limit=MAX_MEMBER):
        info = self.member(name, limit)
        hasher, count = hashlib.sha256(), 0
        with self.zip.open(info) as source:
            for block in iter(lambda: source.read(CHUNK), b''):
                count += len(block)
                require(count <= limit and count <= info.file_size, 'Oversized evidence stream')
                hasher.update(block)
        require(count == info.file_size, 'Truncated evidence stream')
        return {'bytes': count, 'sha256': hasher.hexdigest()}


def validate_partial(partial, role, environment):
    exact(partial, ('schema', 'kind', 'phase', 'benchmark_only', 'cannot_publish', 'context',
                    'identity', 'cache', 'gates', 'coverage', 'phases_ns', 'release_files'), 'Partial')
    require(type(partial['schema']) is int and partial['schema'] == 1 and
            partial['kind'] == 'benchmark-partial' and partial['phase'] == role and
            partial['benchmark_only'] is True and partial['cannot_publish'] is True,
            'Invalid benchmark-only partial identity')
    adapter.validate_context(partial['context'], environment)
    context = partial['context']
    require(context['benchmark']['phase'] == role and
            context['benchmark']['comparison_id'] == 'run-' + context['run']['id'] + '-attempt-' + context['run']['attempt'],
            'Benchmark phase/comparison context mismatch')
    exact(partial['identity'], comparison.IDENTITY, 'Partial identity')
    identity = partial['identity']
    require(identity['orchestrator_sha'] == context['orchestrator']['commit'] and
            identity['source_commit'] == context['source']['commit'] and identity['source_tree'] == context['source']['tree'],
            'Partial source/orchestrator binding mismatch')
    require(identity['adapter_sha256'] == sha256((HERE / 'benchmark_adapter.py').read_bytes()),
            'Unreviewed adapter identity')
    lock = sha256((HERE / 'dependencies/downloads.lock.json').read_bytes())
    pin = local_json(HERE / 'dependencies/PIN.json')
    require(identity['dependency_lock_sha256'] == pin.get('lock_sha256') == lock,
            'Dependency lock pin mismatch')
    require(identity['reviewed_plan_sha256'] == comparison.PLAN_SHA256 == sha256((HERE / 'reviewed-plan.json').read_bytes()),
            'Reviewed plan pin mismatch')
    exact(partial['cache'], ('key', 'benchmark_salt', 'hit', 'restored', 'contents_verified',
                            'forbidden_paths', 'bytes'), 'Partial cache')
    require(partial['cache']['benchmark_salt'] == pin['context']['benchmark_salt'], 'Cache salt changed')
    exact(partial['phases_ns'], PARTIAL_PHASES, 'Partial phases')
    require(all(type(x) is int and x > 0 for x in partial['phases_ns'].values()), 'Invalid monotonic timing')
    return context


def jobs_from_pages(pages, context):
    require(type(pages) is list and 0 < len(pages) <= 100, 'Expected gh --paginate --slurp jobs pages')
    total, jobs = None, []
    for index, page in enumerate(pages):
        exact(page, ('total_count', 'jobs'), 'Jobs page')
        positive(page['total_count'], 'Jobs total_count')
        require(page['total_count'] <= 10_000, 'Too many jobs')
        if total is None:
            total = page['total_count']
        require(total == page['total_count'] and type(page['jobs']) is list and len(page['jobs']) <= 100,
                'Inconsistent jobs pagination')
        require(len(page['jobs']) == 100 if index < len(pages) - 1 else 0 < len(page['jobs']) <= 100,
                'Incomplete or unexpected per_page=100 jobs pagination')
        jobs.extend(page['jobs'])
    require(len(jobs) == total, 'Incomplete jobs pages')
    ids, selected = set(), {}
    run_id, attempt = numeric_id(context['run']['id'], 'Run ID'), numeric_id(context['run']['attempt'], 'Attempt')
    for job in jobs:
        require(type(job) is dict, 'Invalid job record')
        job_id = positive(job.get('id'), 'Job ID')
        require(job_id not in ids, 'Duplicate job ID across pages')
        ids.add(job_id)
        require(type(job.get('run_id')) is int and job['run_id'] == run_id and
                type(job.get('run_attempt')) is int and job['run_attempt'] == attempt and
                job.get('head_sha') == context['orchestrator']['commit'] and
                job.get('head_branch') == adapter.BENCHMARK_REF.removeprefix('refs/heads/'),
                'Jobs response is for another run/attempt/head/branch')
        if job.get('name') in ('cold', 'warm'):
            name = job['name']
            require(name not in selected, 'Ambiguous benchmark job name')
            require(job.get('status') == 'completed' and job.get('conclusion') == 'success',
                    'Benchmark job has not completed successfully')
            interval(job, name + ' job')
            selected[name] = job
    require(set(selected) == {'cold', 'warm'}, 'Missing exact cold/warm jobs')
    require(timestamp(selected['warm']['started_at'], 'Warm start') >=
            timestamp(selected['cold']['completed_at'], 'Cold completion'), 'Warm job predates cold completion')
    return selected


def metadata_binding(metadata, artifact_id, role, context, job, now):
    require(type(metadata) is dict, 'Invalid artifact metadata')
    expected_id = numeric_id(artifact_id, 'Artifact ID')
    require(type(metadata.get('id')) is int and metadata['id'] == expected_id, 'Artifact API ID mismatch')
    name = 'benchmark-' + context['source']['commit'] + '-' + context['orchestrator']['commit'] + '-' + context['run']['id'] + '-' + context['run']['attempt'] + '-' + role
    require(metadata.get('name') == name, 'Artifact name/attempt/phase mismatch')
    require(metadata.get('expired') is False and timestamp(metadata.get('expires_at'), 'Artifact expiry') >= now + timedelta(minutes=10),
            'Artifact expired or expires within ten minutes')
    created = timestamp(metadata.get('created_at'), 'Artifact creation')
    start, end, _ = interval(job, role + ' job')
    require(start <= created <= end and created <= now, 'Artifact creation is outside its successful job')
    workflow = metadata.get('workflow_run')
    require(type(workflow) is dict, 'Missing artifact workflow_run')
    repo_id = numeric_id(context['orchestrator']['repository_id'], 'Repository ID')
    for name, expected in (('id', numeric_id(context['run']['id'], 'Run ID')),
                           ('repository_id', repo_id), ('head_repository_id', repo_id)):
        require(type(workflow.get(name)) is int and workflow[name] == expected,
                'Artifact workflow_run ID/repository/fork mismatch: ' + name)
    require(workflow.get('head_sha') == context['orchestrator']['commit'] and
            workflow.get('head_branch') == adapter.BENCHMARK_REF.removeprefix('refs/heads/'),
            'Artifact workflow head mismatch')


def api_overhead(job, role):
    steps = job.get('steps')
    require(type(steps) is list and steps, 'Missing API job steps')
    names, numbers = {}, set()
    job_start, job_end, job_ns = interval(job, role + ' job')
    for step in steps:
        require(type(step) is dict and type(step.get('name')) is str, 'Malformed API step')
        number = positive(step.get('number'), 'Step number')
        require(number not in numbers and step['name'] not in names, 'Duplicate/ambiguous API step')
        numbers.add(number)
        names[step['name']] = step
    unwanted = 'Cache restore' if role == 'cold' else 'Cache save'
    require(unwanted not in names, 'Unexpected cache action for phase')
    result = {'dependency_restore': 0, 'cache_save': 0}
    timed = [(('Cache save', 'cache_save') if role == 'cold' else
              ('Cache restore', 'dependency_restore')), ('Upload benchmark evidence', 'evidence_upload')]
    if role == 'cold':
        timed.append(('Verify distributions before save', 'dependency_verify'))
    for name, phase in timed:
        require(name in names, 'Missing API overhead step: ' + name)
        step = names[name]
        require(step.get('status') == 'completed' and step.get('conclusion') == 'success',
                'Unsuccessful API overhead step: ' + name)
        start, end, elapsed = interval(step, name, allow_zero=name == 'Verify distributions before save')
        require(job_start <= start <= end <= job_end, 'API step lies outside job')
        result[phase] = elapsed
    return result, job_ns


def validate_evidence(archive, partial):
    context, identity = partial['context'], partial['identity']
    require(archive.json('benchmark-context.json') == context, 'Staged context mismatch')
    status = archive.json('runner-status.json')
    exact(status, ('benchmark_only', 'cannot_publish', 'status', 'phases_ns'), 'Runner status')
    require(status['benchmark_only'] is True and status['cannot_publish'] is True and status['status'] == 'passed' and
            status['phases_ns'] == partial['phases_ns'] == archive.json('phase-times.json'),
            'Runner status/timing evidence mismatch')
    dependencies = archive.json('dependencies.json')
    lock = local_json(HERE / 'dependencies/downloads.lock.json')
    require(type(dependencies) is dict and type(dependencies.get('schema')) is int and dependencies['schema'] == 1 and
            dependencies.get('phase') == partial['phase'] and dependencies.get('status') == 'verified_and_installed' and
            dependencies.get('context') == lock['context'], 'Dependency installation evidence mismatch')
    for name, expected in (('lock_sha256', identity['dependency_lock_sha256']), ('cache_key', partial['cache']['key']),
                           ('cache_hit', partial['cache']['hit']), ('benchmark_salt', partial['cache']['benchmark_salt']),
                           ('cache_bytes', partial['cache']['bytes'])):
        require(dependencies.get(name) == expected and type(dependencies.get(name)) is type(expected),
                'Dependency report binding mismatch: ' + name)
    require(type(partial['cache']['bytes']) is int and partial['cache']['bytes'] == sum(item['bytes'] for item in lock['artifacts']),
            'Dependency bytes do not match locked download closure')
    require(dependencies.get('cache') == {'key': partial['cache']['key'], 'hit': partial['cache']['hit'],
            'restored': partial['cache']['restored'], 'lock_sha256': identity['dependency_lock_sha256'],
            'files': len(lock['artifacts']), 'bytes': partial['cache']['bytes']}, 'Dependency cache evidence mismatch')
    for name in ('dependency_install_ns', 'dependency_verify_ns', 'total_ns'):
        positive(dependencies.get(name), 'Dependency timing ' + name)
    require(partial['phases_ns']['dependency_install'] == dependencies['dependency_install_ns'] and
            partial['phases_ns']['dependency_verify'] >= dependencies['dependency_verify_ns'] and
            dependencies['total_ns'] == dependencies['dependency_install_ns'] + dependencies['dependency_verify_ns'],
            'Dependency phase timing mismatch')
    require(type(dependencies.get('download_ns')) is int and
            (dependencies['download_ns'] > 0 if partial['phase'] == 'cold' else dependencies['download_ns'] == 0),
            'Warm redownload or missing cold download timing')
    actual_files = {name: archive.digest('source/release/' + name) for name in RELEASE_NAMES}
    require(actual_files == partial['release_files'], 'Release payload mutation/digest mismatch')
    sums = ''.join(actual_files[name]['sha256'] + '  ' + name + '\n' for name in RELEASE_NAMES).encode()
    require(archive.read('source/release/SHA256SUMS') == sums, 'Release SHA256SUMS mismatch')
    manifest_raw = archive.read('adapter/adapter-manifest.json')
    manifest = json_data(manifest_raw)
    require(manifest.get('schema') == 1 and type(manifest.get('schema')) is int and
            manifest.get('benchmark_only') is True and manifest.get('cannot_publish') is True and
            manifest.get('published') is False and manifest.get('context') == context and
            manifest.get('original_sha256') == adapter.PINNED, 'Adapter manifest binding mismatch')
    exact(manifest.get('paths'), ('source', 'orchestrator', 'output'), 'Adapter workspace paths')
    source_cwd = manifest['paths']['source']
    require(type(source_cwd) is str and source_cwd.startswith('/') and
            str(Path(source_cwd)) == source_cwd and '..' not in Path(source_cwd).parts,
            'Invalid adapter source workspace path')
    ledger = archive.json('commands.json')
    require(type(ledger) is list and ledger, 'Missing command execution ledger')
    require(all(type(entry) is dict and type(entry.get('label')) is str for entry in ledger),
            'Malformed command execution ledger')
    labels = [entry['label'] for entry in ledger]
    require(len(labels) == len(set(labels)), 'Duplicate command execution label')
    final_labels = [name for name, _ in FINAL_COMMANDS]
    require(all(name in labels for name in final_labels), 'Missing final command evidence')
    start = labels.index(final_labels[0])
    require(labels[start:start + len(final_labels)] == final_labels,
            'Final command sequence changed or was interrupted')
    for entry, (label, command) in zip(ledger[start:start + len(final_labels)], FINAL_COMMANDS):
        exact(entry, ('label', 'argv', 'cwd', 'log', 'started_monotonic_ns', 'elapsed_ns', 'exit_code'),
              'Final command ledger entry')
        require(entry['argv'] == list(command) and entry['cwd'] == source_cwd and
                type(entry['exit_code']) is int and entry['exit_code'] == 0,
                'Final command argv/cwd/exit status changed: ' + label)
        positive(entry['started_monotonic_ns'], 'Final command start')
        positive(entry['elapsed_ns'], 'Final command duration')
        require(entry['log'] == 'logs/' + label + '.log', 'Final command log path changed: ' + label)
        require(archive.digest(entry['log'], MAX_LOG)['bytes'] > 0, 'Empty final command log: ' + label)
    generated = manifest.get('generated_sha256')
    names = ('check_scope.py', 'record_identity.py', 'test_coopdiag77.sh', 'benchmark_runtime.py')
    exact(generated, names, 'Adapter generated files')
    require({name for name in archive.members if name.startswith('adapter/')} ==
            {'adapter/' + name for name in (*names, 'adapter-manifest.json')}, 'Unexpected adapter files')
    for name, digest in generated.items():
        require(comparison.digest(digest) and archive.digest('adapter/' + name)['sha256'] == digest,
                'Adapter generated payload changed')
    require(generated['benchmark_runtime.py'] == identity['adapter_sha256'], 'Adapter runtime differs from reviewed code')
    record = archive.json('source/build/coopdiag77-benchmark-identity.json', finite_floats=True)
    require(record.get('benchmark_only') is True and record.get('cannot_publish') is True and
            record.get('published') is False and record.get('benchmark_context') == context and
            record.get('source_commit') == identity['source_commit'] and record.get('source_tree') == identity['source_tree'] and
            record.get('orchestrator_ref') == context['orchestrator']['ref'] and record.get('leak_detection') is True and
            record.get('artifacts') == actual_files and record.get('adapter_manifest_sha256') == sha256(manifest_raw),
            'Final benchmark identity binding mismatch')
    target = record.get('target_builds')
    require(type(target) is dict and target.get('source_commit') == identity['source_commit'] and
            target.get('source_tree') == identity['source_tree'] and target.get('core') == identity['esp32_core'] and
            target.get('compiler') == identity['target_compiler'] and target.get('fqbn') == identity['fqbn'] and
            target.get('default_arduino_enabled') is True, 'Target build identity mismatch')
    builds = target.get('builds')
    exact(builds, ('0', '1'), 'Target variants')
    for flag, enabled in (('0', False), ('1', True)):
        build = builds[flag]
        require(type(build) is dict and build.get('feature_enabled') is enabled, 'Target variant missing')
        options = build.get('effective_options')
        require(type(options) is dict and options.get('default_arduino_enabled') is enabled and
                options.get('fqbn') == identity['fqbn'], 'Target effective options mismatch')
        override = options.get('explicit_override')
        require(override is None if enabled else type(override) is int and override == 0,
                'Default-ON/forced-OFF override mismatch')
    require(builds['1'].get('image') == actual_files['firmware.bin'], 'Release is not default-ON firmware')
    require(archive.json('source/build/coopdiag77-esp32-build-evidence.json', finite_floats=True) == target,
            'Target evidence differs from final identity')
    aggregate_root = 'source/build/package12-validation/' + context['benchmark']['aggregate_run_id'] + '/'
    summary = archive.json(aggregate_root + 'summary.json')
    require(record.get('aggregate') == summary and summary.get('all_passed') is True and
            summary.get('failed') == [] and type(summary.get('tasks')) is int and summary['tasks'] == 157 and
            summary.get('lsan') == 'detect_leaks=1' and summary.get('source_sha256') == identity['source_inventory_sha256'] and
            summary.get('publication') is False, 'Aggregate summary mismatch')
    source_manifest = archive.json(aggregate_root + 'source-manifest.json')
    exact(source_manifest, ('files', 'compiler', 'detect_leaks', 'assertions', 'ubsan_halt', 'sha256'), 'Source manifest')
    body = {name: value for name, value in source_manifest.items() if name != 'sha256'}
    require(source_manifest['sha256'] == identity['source_inventory_sha256'] == comparison.SOURCE_INVENTORY_SHA256 and
            sha256(json.dumps(body, sort_keys=True).encode()) == source_manifest['sha256'] and
            source_manifest['compiler'] == identity['host_compiler'] and source_manifest['detect_leaks'] == '1' and
            source_manifest['assertions'] is True and source_manifest['ubsan_halt'] is True,
            'Pinned aggregate source inventory mismatch')
    require(type(source_manifest['files']) is dict and source_manifest['files'], 'Empty source inventory')
    for name, digest in source_manifest['files'].items():
        safe_name(name)
        require(comparison.digest(digest), 'Invalid source file digest')
    reviewed = local_json(HERE / 'reviewed-plan.json')
    require(archive.json(aggregate_root + 'plan.json') ==
            {'focused': False, 'tasks': reviewed['tasks'], 'source_sha256': identity['source_inventory_sha256']},
            'Aggregate plan differs from reviewed commands')
    checkpoints = archive.json(aggregate_root + 'checkpoints.json', finite_floats=True)
    commands = {'preparation': reviewed['preparation'], **dict(reviewed['tasks'])}
    require(type(checkpoints) is dict and set(checkpoints) == set(commands) and len(commands) == 157,
            'Missing or extra aggregate checkpoint')
    for name, argv in commands.items():
        entry = checkpoints[name]
        require(type(entry) is dict and entry.get('status') == 'passed' and entry.get('commands') == argv and
                entry.get('log_sha256') == archive.digest(aggregate_root + name + '.log', MAX_LOG)['sha256'],
                'Checkpoint command/status/log mismatch: ' + name)
    aggregate_log = archive.read('source/build/coopdiag77-tests.log', MAX_LOG).decode('utf-8', errors='replace')
    run_names = re.findall(r'^RUN (.+)$', aggregate_log, re.M)
    require('RESUME skip ' not in aggregate_log and len(run_names) == 157 and set(run_names) == set(commands),
            'Aggregate did not freshly run all 157 groups')
    inventory = archive.json('source-artifact-inventory.json')
    require(type(inventory) is dict and inventory, 'Missing staged source artifact inventory')
    require(set(inventory) == {name.removeprefix('source/') for name in archive.members if name.startswith('source/')},
            'Staged artifact inventory paths differ')
    # Hash all declared source evidence as a stream; large captures never enter memory.
    for name, descriptor in inventory.items():
        safe_name(name)
        exact(descriptor, ('bytes', 'sha256'), 'Staged artifact descriptor')
        require(type(descriptor['bytes']) is int and descriptor == archive.digest('source/' + name),
                'Staged evidence payload changed: ' + name)
    require('source/build/coopdiag77-source-identity.json' not in archive.members,
            'Production source identity is forbidden in benchmark evidence')
    # Bind nested source hashes from final gates to the pinned source inventory.
    def source_bindings(value):
        if type(value) is dict:
            for key, child in value.items():
                if key in ('sources_sha256', 'source_sha256', 'regression_sha256') and type(child) is dict:
                    require(child, 'Empty gate source binding')
                    for path, digest in child.items():
                        require(source_manifest['files'].get(path) == digest, 'Gate source binding changed: ' + path)
                else:
                    source_bindings(child)
        elif type(value) is list:
            for child in value:
                source_bindings(child)
    source_bindings(record)


def assemble(partial, job, cold_job):
    context = partial['context']
    extra, job_ns = api_overhead(job, partial['phase'])
    report = {'schema': 1, 'kind': 'benchmark-only-' + partial['phase'], 'identity': partial['identity'],
              'run': {'repository': context['orchestrator']['repository'], 'id': numeric_id(context['run']['id'], 'Run ID'),
                      'attempt': numeric_id(context['run']['attempt'], 'Attempt'), 'job_id': positive(job['id'], 'Job ID'),
                      'branch': context['orchestrator']['ref'].removeprefix('refs/heads/'), 'status': 'success'},
              'cache': {**partial['cache'], 'writer_run_id': numeric_id(context['run']['id'], 'Writer run ID'),
                        'writer_job_id': positive(cold_job['id'], 'Writer job ID')},
              'gates': partial['gates'], 'coverage': partial['coverage'],
              'phases_ns': {**partial['phases_ns'], **extra,
                            'dependency_verify': partial['phases_ns']['dependency_verify'] + extra.get('dependency_verify', 0)},
              'job_ns': job_ns,
              'release_files': partial['release_files'], 'publish_authorized': False}
    comparison.validate(report, partial['phase'])
    return report


def collect(cold_zip, cold_metadata, cold_id, warm_zip, warm_metadata, warm_id, jobs, *, environment=None, now=None):
    environment = dict(os.environ if environment is None else environment)
    now = datetime.now(timezone.utc) if now is None else now
    require(now.tzinfo is not None and now.utcoffset() == timedelta(0), 'Collector clock must use UTC')
    require(numeric_id(cold_id, 'Cold artifact ID') != numeric_id(warm_id, 'Warm artifact ID'), 'Artifact IDs must differ')
    metadata = {'cold': local_json(cold_metadata), 'warm': local_json(warm_metadata)}
    paths, ids = {'cold': cold_zip, 'warm': warm_zip}, {'cold': cold_id, 'warm': warm_id}
    reports, provenance = {}, {}
    with ExitStack() as stack:
        archives, partials, contexts = {}, {}, {}
        for role in ('cold', 'warm'):
            archive = Archive(paths[role], metadata[role])
            stack.callback(archive.close)
            archives[role] = archive
            partial = archive.json('measurement.partial.json')
            partials[role] = partial
            contexts[role] = validate_partial(partial, role, environment)
        left = {**contexts['cold'], 'benchmark': {**contexts['cold']['benchmark'], 'phase': 'warm'}}
        require(left == contexts['warm'], 'Cold/warm contexts differ')
        selected = jobs_from_pages(local_json(jobs), contexts['cold'])
        for role in ('cold', 'warm'):
            job, archive = selected[role], archives[role]
            metadata_binding(metadata[role], ids[role], role, contexts[role], job, now)
            validate_evidence(archive, partials[role])
            reports[role] = assemble(partials[role], job, selected['cold'])
            provenance[role] = {'artifact_id': numeric_id(ids[role], 'Artifact ID'),
                                'artifact_name': metadata[role]['name'], 'zip_sha256': archive.sha256,
                                'zip_bytes': archive.size, 'job_id': job['id'],
                                'created_at': metadata[role]['created_at'], 'expires_at': metadata[role]['expires_at']}
    result = comparison.compare(reports['cold'], reports['warm'])
    return {'schema': 1, 'kind': 'benchmark-collected-pair', 'benchmark_only': True, 'cannot_publish': True,
            'authenticated_provenance': False, 'publish_authorized': False,
            **reports, 'comparison': result,
            'transport_consistency': {'verified': True, 'artifacts': provenance,
                'jobs_endpoint': '/repos/' + adapter.REPOSITORY + '/actions/runs/' + contexts['cold']['run']['id'] +
                                 '/attempts/' + contexts['cold']['run']['attempt'] + '/jobs?per_page=100',
                'trust_boundary': 'Official API responses and ZIPs must be fetched by the reviewed read-only workflow. '
                                  'These local files and this JSON cannot independently authenticate their own origin.'},
            'timing_method': {'internal_phases': 'Runner time.monotonic_ns, positive integer nanoseconds',
                              'api_phases': ['dependency_restore', 'cache_save', 'evidence_upload'],
                              'dependency_verify': 'Runner monotonic_ns plus cold-only Verify distributions before save API duration; a zero API interval is below the one-second timestamp resolution, not free verification',
                              'api_duration': '(completed_at - started_at) * 1000000000',
                              'api_timestamp_resolution_ns': NS, 'job_duration_source': 'GitHub attempt jobs API',
                              'note': 'API values are whole-second observations, not nanosecond-accurate clocks. '
                                      'Cold restore and warm cache save are zero because those actions are absent. '
                                      'One pair is observational; publication and fleet-wide savings are not measured.'}}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    for role in ('cold', 'warm'):
        parser.add_argument('--' + role + '-zip', type=Path, required=True)
        parser.add_argument('--' + role + '-metadata', type=Path, required=True)
        parser.add_argument('--' + role + '-id', required=True)
    parser.add_argument('--jobs', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(argv)
    result = collect(args.cold_zip, args.cold_metadata, args.cold_id,
                     args.warm_zip, args.warm_metadata, args.warm_id, args.jobs)
    # A rejected pair never produces a result; existing paths are never replaced.
    with args.output.open('x', encoding='utf-8') as output:
        output.write(json.dumps(result, indent=2, sort_keys=True, allow_nan=False) + '\n')
    return result


if __name__ == '__main__':
    try:
        main()
    except (Rejected, adapter.Rejected, comparison.Rejected, OSError, ValueError,
            TypeError, KeyError, zipfile.BadZipFile, NotImplementedError) as error:
        raise SystemExit('REJECTED: ' + str(error))
