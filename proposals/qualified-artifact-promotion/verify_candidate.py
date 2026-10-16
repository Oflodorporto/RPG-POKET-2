#!/usr/bin/env python3
"""OFFLINE DESIGN PROTOTYPE. A successful result NEVER authorizes publication.

Reads a bounded ZIP as data without extracting or executing it. Caller-supplied
expectations and API observations are NOT authenticated. There is deliberately
no network client, publishing function, workflow, approval or lease consumer.
"""
import argparse
from datetime import datetime, timedelta, timezone
import hashlib
import io
import json
import os
from pathlib import Path
import re
import stat
import struct
import sys
import zipfile
import zlib

MAX_ARCHIVE = 64 * 1024 * 1024
MAX_JSON = 1024 * 1024
RELEASE_NAMES = ('firmware.bin', 'artes.pak', 'manifest.json')
FIXED_LIMITS = {
    'release/firmware.bin': 0x300000,
    'release/artes.pak': 16 * 1024 * 1024,
    'release/manifest.json': 4096,
    'release/SHA256SUMS': 512,
    'evidence/identity.json': 16384,
    'evidence/source-inventory.json': MAX_JSON,
    'evidence/plan.json': 256 * 1024,
    'evidence/checkpoints.json': 512 * 1024,
}


class Rejected(ValueError):
    """Invalid or insufficient offline evidence."""


def require(condition, message):
    if not condition:
        raise Rejected(message)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def exact_keys(value, keys, label):
    require(type(value) is dict and set(value) == set(keys), label + ': exact fields required')


def same(left, right):
    # JSON identity is type-sensitive: True must never stand in for numeric ID 1.
    return json.dumps(left, sort_keys=True, separators=(',', ':')) == json.dumps(right, sort_keys=True, separators=(',', ':'))


def positive_int(value, label):
    require(type(value) is int and value > 0, label + ': positive integer required')


def hash_string(value, length, label):
    require(type(value) is str and re.fullmatch('[0-9a-f]{%d}' % length, value), label + ': invalid hash')


def timestamp(value):
    require(type(value) is str and re.fullmatch(r'\d{4}-\d\d-\d\dT\d\d:\d\d:\d\dZ', value), 'UTC timestamp required')
    try:
        return datetime.strptime(value, '%Y-%m-%dT%H:%M:%SZ').replace(tzinfo=timezone.utc)
    except ValueError as error:
        raise Rejected('Invalid UTC timestamp') from error


def strict_json(raw):
    require(len(raw) <= MAX_JSON, 'JSON exceeds limit')
    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, 'Duplicate JSON key')
            result[key] = value
        return result
    def invalid_constant(_):
        raise Rejected('Non-finite JSON number')
    try:
        return json.loads(raw, object_pairs_hook=pairs, parse_constant=invalid_constant, parse_float=invalid_constant)
    except (ValueError, UnicodeError, RecursionError) as error:
        raise Rejected('Invalid JSON: ' + str(error)) from error


def read_regular(path, limit):
    # The CLI never follows a final-component symlink and bounds reads before allocation.
    flags = os.O_RDONLY | os.O_NONBLOCK | getattr(os, 'O_NOFOLLOW', 0)
    try:
        descriptor = os.open(path, flags)
        with os.fdopen(descriptor, 'rb') as stream:
            info = os.fstat(stream.fileno())
            require(stat.S_ISREG(info.st_mode) and 0 < info.st_size <= limit, 'Input must be a bounded regular file')
            data = stream.read(limit + 1)
            require(0 < len(data) <= limit and len(data) == info.st_size, 'Input size changed')
            return data
    except OSError as error:
        raise Rejected('Cannot read regular input: ' + str(error)) from error


def validate_expectations(expected):
    exact_keys(expected, ('repository', 'workflow', 'source', 'release', 'run', 'artifact', 'task_ids', 'plan_sha256'), 'Expectations')
    repository = expected['repository']
    exact_keys(repository, ('id', 'full_name'), 'Repository')
    positive_int(repository['id'], 'Repository ID')
    require(repository['full_name'] == 'Oflodorporto/RPG-POKET-2', 'Unexpected repository')
    workflow = expected['workflow']
    exact_keys(workflow, ('id', 'path', 'sha256'), 'Workflow')
    positive_int(workflow['id'], 'Workflow ID')
    require(type(workflow['path']) is str and re.fullmatch(r'\.github/workflows/[a-z0-9-]+\.yml', workflow['path']), 'Invalid workflow path')
    hash_string(workflow['sha256'], 64, 'Workflow digest')
    source = expected['source']
    exact_keys(source, ('commit', 'tree', 'inventory_sha256', 'branch'), 'Source')
    for field in ('commit', 'tree'):
        hash_string(source[field], 40, field)
    hash_string(source['inventory_sha256'], 64, 'Source inventory')
    require(type(source['branch']) is str and re.fullmatch(r'feat/[a-z0-9][a-z0-9-]{0,80}', source['branch']), 'Unapproved branch shape')
    release = expected['release']
    exact_keys(release, ('version', 'build', 'tag'), 'Release')
    require(type(release['version']) is str and re.fullmatch(r'[a-z0-9][a-z0-9.-]{0,38}', release['version']), 'Invalid version')
    positive_int(release['build'], 'Build')
    require(release['tag'] == 'v' + release['version'], 'Version/tag mismatch')
    run = expected['run']
    exact_keys(run, ('id', 'attempt'), 'Run')
    for field in run:
        positive_int(run[field], 'Run ' + field)
    artifact = expected['artifact']
    exact_keys(artifact, ('id', 'name', 'sha256', 'bytes'), 'Artifact')
    positive_int(artifact['id'], 'Artifact ID')
    positive_int(artifact['bytes'], 'Artifact size')
    require(artifact['bytes'] <= MAX_ARCHIVE, 'Archive too large')
    hash_string(artifact['sha256'], 64, 'Archive digest')
    name = 'release-candidate-%s-%d-%d' % (source['commit'], run['id'], run['attempt'])
    require(artifact['name'] == name, 'Artifact name must bind commit/run/attempt')
    tasks = expected['task_ids']
    require(type(tasks) is list and len(tasks) == 157, 'Exactly 157 reviewed tasks required')
    require(all(type(task) is str and re.fullmatch(r'[a-z][a-z0-9_-]{0,80}', task) for task in tasks), 'Invalid task ID')
    require(len(set(tasks)) == 157 and 'preparation' in tasks, 'Duplicate or missing task')
    hash_string(expected['plan_sha256'], 64, 'Reviewed plan digest')


def validate_observation(expected, observed, now):
    exact_keys(observed, ('fetched_at', 'repository', 'run', 'workflow_sha256', 'artifact_candidates', 'main_sha', 'source_tree', 'inventory_sha256', 'tag_exists', 'release_exists', 'consumed_run_attempts'), 'Observation')
    require(timedelta(0) <= now - timestamp(observed['fetched_at']) <= timedelta(minutes=5), 'Stale or future API observation')
    require(same(observed['repository'], expected['repository']), 'Repository observation mismatch')
    source, run_pin, workflow = expected['source'], expected['run'], expected['workflow']
    require(observed['main_sha'] == source['commit'], 'Main lease mismatch')
    require(observed['source_tree'] == source['tree'] and observed['inventory_sha256'] == source['inventory_sha256'], 'Source tree/inventory mismatch')
    require(observed['workflow_sha256'] == workflow['sha256'], 'Workflow content mismatch')
    require(observed['tag_exists'] is False and observed['release_exists'] is False, 'Existing tag/release cannot be overwritten')
    consumed = observed['consumed_run_attempts']
    require(type(consumed) is list and all(type(item) is str and re.fullmatch(r'[1-9][0-9]*:[1-9][0-9]*', item) for item in consumed), 'Invalid replay snapshot')
    require('%d:%d' % (run_pin['id'], run_pin['attempt']) not in consumed, 'Consumed qualification attempt')
    run = observed['run']
    exact_keys(run, ('id', 'run_attempt', 'workflow_id', 'path', 'event', 'head_sha', 'head_branch', 'repository', 'head_repository', 'status', 'conclusion', 'run_started_at', 'completed_at'), 'Run observation')
    require(type(run['id']) is int and type(run['run_attempt']) is int and type(run['workflow_id']) is int, 'Invalid observed run IDs')
    require(run['id'] == run_pin['id'] and run['run_attempt'] == run_pin['attempt'], 'Run/attempt mismatch')
    require(run['workflow_id'] == workflow['id'] and run['path'] == workflow['path'], 'Workflow identity mismatch')
    require(same(run['repository'], expected['repository']) and same(run['head_repository'], expected['repository']), 'Fork or different repository')
    require(run['event'] == 'push' and run['head_branch'] == source['branch'] and run['head_sha'] == source['commit'], 'Untrusted event/branch/SHA')
    require(run['status'] == 'completed' and run['conclusion'] == 'success', 'Qualification did not succeed')
    started, completed = timestamp(run['run_started_at']), timestamp(run['completed_at'])
    require(started <= completed <= timestamp(observed['fetched_at']) <= now and now - completed <= timedelta(hours=72), 'Stale or invalid qualification time')
    candidates = observed['artifact_candidates']
    # This is the complete paginated set for the dedicated capsule name, not a first-match search.
    require(type(candidates) is list and len(candidates) == 1, 'Missing or ambiguous candidate')
    artifact = candidates[0]
    exact_keys(artifact, ('id', 'name', 'size_in_bytes', 'digest', 'expired', 'created_at', 'expires_at', 'workflow_run'), 'Artifact observation')
    pin = expected['artifact']
    require(type(artifact['id']) is int and artifact['id'] == pin['id'], 'Artifact ID mismatch')
    require(artifact['name'] == pin['name'] and artifact['digest'] == 'sha256:' + pin['sha256'], 'Artifact name/digest mismatch')
    require(type(artifact['size_in_bytes']) is int and artifact['size_in_bytes'] == pin['bytes'], 'Artifact API size mismatch')
    require(artifact['expired'] is False, 'Expired artifact')
    require(started <= timestamp(artifact['created_at']) <= completed, 'Artifact outside approved attempt window')
    require(timestamp(artifact['expires_at']) > now + timedelta(minutes=10), 'Artifact expiry too close or past')
    require(same(artifact['workflow_run'], {
        'id': run_pin['id'], 'repository_id': expected['repository']['id'],
        'head_repository_id': expected['repository']['id'], 'head_branch': source['branch'], 'head_sha': source['commit'],
    }), 'Artifact run/repository binding mismatch')


def validate_local_headers(archive, entries, central_offset):
    # This prototype deliberately defines one narrow ZIP layout: ordinary local
    # headers with known CRC/sizes, no descriptors, ZIP64, extra fields or gaps.
    # A future real Actions capsule must prove compatibility before activation.
    cursor = 0
    for entry in sorted(entries, key=lambda item: item.header_offset):
        require(entry.header_offset == cursor and cursor + 30 <= central_offset, 'ZIP hidden member/gap forbidden')
        fields = struct.unpack('<4s5H3I2H', archive[cursor:cursor + 30])
        signature, version, flags, compression, _, _, crc, compressed, expanded, name_size, extra_size = fields
        require(signature == b'PK\x03\x04' and version <= 20 and entry.extract_version <= 20, 'Local ZIP signature/version/ZIP64 forbidden')
        require(not (flags & 8) and extra_size == 0 and not entry.extra, 'ZIP descriptors/extra fields forbidden')
        require(flags == entry.flag_bits and compression == entry.compress_type, 'Local ZIP flags/compression mismatch')
        require(crc == entry.CRC and compressed == entry.compress_size and expanded == entry.file_size, 'Local ZIP CRC/size mismatch')
        encoding = 'utf-8' if flags & 0x800 else 'cp437'
        require(archive[cursor + 30:cursor + 30 + name_size] == entry.orig_filename.encode(encoding), 'Local ZIP filename mismatch')
        cursor += 30 + name_size + compressed
        require(cursor <= central_offset, 'Local ZIP overlap')
    require(cursor == central_offset, 'Unlisted ZIP local member or trailing data')


def read_capsule(archive, tasks):
    limits = dict(FIXED_LIMITS)
    limits.update({'evidence/logs/' + task + '.log': 2 * 1024 * 1024 for task in tasks})
    require(type(archive) is bytes and 22 <= len(archive) <= MAX_ARCHIVE, 'Invalid archive bytes')
    require(archive.startswith(b'PK\x03\x04') and archive[-22:-18] == b'PK\x05\x06', 'ZIP prefix/trailer/comment forbidden')
    disk, central_disk, count_disk, count, central_size, central_offset, comment = struct.unpack('<4H2IH', archive[-18:])
    require(disk == central_disk == comment == 0 and count_disk == count == len(limits), 'Split/ZIP64/comment archive forbidden')
    require(central_offset + central_size == len(archive) - 22, 'ZIP central directory framing mismatch')
    require(0 < central_size <= len(limits) * 512, 'ZIP central directory metadata exceeds bound')
    try:
        with zipfile.ZipFile(io.BytesIO(archive)) as capsule:
            entries = capsule.infolist()
            require(entries and min(entry.header_offset for entry in entries) == 0, 'ZIP prefix forbidden')
            names = [entry.filename for entry in entries]
            require(len(entries) == len(limits) and len(set(names)) == len(names), 'Duplicate/missing/extra ZIP members')
            require(set(names) == set(limits), 'ZIP member outside exact allowlist')
            validate_local_headers(archive, entries, central_offset)
            require(sum(entry.file_size for entry in entries) <= MAX_ARCHIVE, 'Expanded archive exceeds bound')
            result = {}
            for entry in entries:
                require(entry.orig_filename == entry.filename, 'Truncated/NUL member name')
                require(not entry.comment, 'ZIP member comment forbidden')
                mode = entry.external_attr >> 16
                require(stat.S_IFMT(mode) in (0, stat.S_IFREG) and not (mode & 0o111), 'Non-regular or executable ZIP member')
                require(not (entry.external_attr & 0x10) and not entry.is_dir(), 'Directory member forbidden')
                require(not (entry.flag_bits & 1), 'Encrypted member forbidden')
                require(entry.compress_type in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED), 'Unsupported compression')
                require(0 < entry.file_size <= limits[entry.filename], 'ZIP member size exceeds bound')
                require(entry.compress_size > 0 and entry.file_size <= entry.compress_size * 200, 'ZIP expansion ratio exceeds bound')
                with capsule.open(entry) as stream:
                    data = stream.read(limits[entry.filename] + 1)
                require(len(data) == entry.file_size, 'ZIP actual size mismatch')
                result[entry.filename] = data
            return result
    except (zipfile.BadZipFile, RuntimeError, NotImplementedError, EOFError, zlib.error, struct.error) as error:
        raise Rejected('Invalid ZIP: ' + str(error)) from error


def validate_evidence(expected, files):
    inventory_raw = files['evidence/source-inventory.json']
    require(sha256(inventory_raw) == expected['source']['inventory_sha256'], 'Reviewed exact source inventory changed')
    inventory = strict_json(inventory_raw)
    exact_keys(inventory, ('schema', 'commit', 'tree', 'files'), 'Source inventory')
    require(type(inventory['schema']) is int and inventory['schema'] == 1, 'Inventory schema')
    require(inventory['commit'] == expected['source']['commit'] and inventory['tree'] == expected['source']['tree'], 'Inventory source mismatch')
    require(type(inventory['files']) is list and inventory['files'], 'Empty source inventory')
    paths = []
    for entry in inventory['files']:
        exact_keys(entry, ('path', 'bytes', 'sha256', 'executable'), 'Inventory entry')
        name = entry['path']
        require(type(name) is str and re.fullmatch(r'[A-Za-z0-9_.-]+(?:/[A-Za-z0-9_.-]+)*', name), 'Unsafe source inventory path')
        require(all(part not in ('.', '..') for part in name.split('/')), 'Noncanonical source inventory path')
        require(type(entry['bytes']) is int and entry['bytes'] >= 0 and type(entry['executable']) is bool, 'Inventory type mismatch')
        hash_string(entry['sha256'], 64, 'Inventory file digest')
        paths.append(name)
    require(paths == sorted(set(paths)), 'Unsorted/duplicate source inventory')
    identity = strict_json(files['evidence/identity.json'])
    exact_keys(identity, ('schema', 'repository', 'workflow', 'source', 'release', 'run', 'files', 'gates'), 'Identity')
    require(type(identity['schema']) is int and identity['schema'] == 1, 'Identity schema')
    for key in ('repository', 'workflow', 'source', 'release', 'run'):
        require(same(identity[key], expected[key]), 'Identity mismatch: ' + key)
    wanted = {name: {'bytes': len(files['release/' + name]), 'sha256': sha256(files['release/' + name])} for name in RELEASE_NAMES}
    require(same(identity['files'], wanted), 'Payload hash/size mismatch')
    require(same(identity['gates'], {
        'esp32_build': 'passed', 'scope_inventory': 'passed',
        'real_byte_ota_normal': 'passed', 'real_byte_ota_sanitizer': 'passed',
        'asan': True, 'ubsan_halt_on_error': True, 'lsan_detect_leaks': True,
        'assertions_enabled': True, 'aggregate_tasks': 157,
    }), 'Missing or weakened qualification gate claims')
    sums = ''.join('%s  %s\n' % (wanted[name]['sha256'], name) for name in RELEASE_NAMES).encode()
    require(files['release/SHA256SUMS'] == sums, 'Checksum inventory mismatch')
    plan_raw = files['evidence/plan.json']
    require(sha256(plan_raw) == expected['plan_sha256'], 'Reviewed exact command plan changed')
    plan = strict_json(plan_raw)
    exact_keys(plan, ('schema', 'source', 'sanitizers', 'tasks'), 'Plan')
    require(type(plan['schema']) is int and plan['schema'] == 1 and same(plan['source'], expected['source']), 'Plan source/schema mismatch')
    require(same(plan['sanitizers'], {'ASAN_OPTIONS': 'detect_leaks=1', 'UBSAN_OPTIONS': 'halt_on_error=1', 'assertions': True}), 'Sanitizers weakened')
    require(type(plan['tasks']) is list and len(plan['tasks']) == 157, 'Plan task count mismatch')
    checkpoints = strict_json(files['evidence/checkpoints.json'])
    exact_keys(checkpoints, expected['task_ids'], 'Checkpoint task set')
    seen = []
    for task in plan['tasks']:
        exact_keys(task, ('id', 'commands'), 'Plan task')
        task_id = task['id']
        require(type(task_id) is str and task_id in expected['task_ids'] and task_id not in seen, 'Plan task identity mismatch')
        seen.append(task_id)
        commands = task['commands']
        require(type(commands) is list and len(commands) > 0, 'Missing commands')
        require(all(type(command) is list and command and all(type(arg) is str and arg and '\0' not in arg for arg in command) for command in commands), 'Invalid command data')
        checkpoint = checkpoints[task_id]
        exact_keys(checkpoint, ('status', 'commands', 'log_sha256'), 'Checkpoint')
        require(checkpoint['status'] == 'passed' and checkpoint['commands'] == commands, 'Missing/failed/changed task commands')
        require(checkpoint['log_sha256'] == sha256(files['evidence/logs/' + task_id + '.log']), 'Task log hash mismatch')
    require(seen == expected['task_ids'], 'Reviewed task order changed')
    manifest = strict_json(files['release/manifest.json'])
    require(type(manifest) is dict, 'Manifest must be an object')
    require(manifest.get('schema') == 2 and type(manifest.get('schema')) is int, 'Manifest schema')
    require(manifest.get('version') == expected['release']['version'] and type(manifest.get('build')) is int and manifest['build'] == expected['release']['build'], 'Manifest version/build mismatch')
    for key, name in (('firmware', 'firmware.bin'), ('art', 'artes.pak')):
        item = manifest.get(key)
        require(type(item) is dict, 'Missing manifest payload')
        require(type(item.get('bytes')) is int and item['bytes'] == wanted[name]['bytes'] and item.get('sha256') == wanted[name]['sha256'], 'Manifest payload mismatch')
        url = 'https://github.com/%s/releases/download/%s/%s' % (expected['repository']['full_name'], expected['release']['tag'], name)
        require(item.get('url') == url, 'Manifest destination mismatch')
    return wanted


def verify(archive, context, now=None):
    """Check internally consistent offline claims. Does NOT authenticate them."""
    now = now or datetime.now(timezone.utc)
    require(now.tzinfo is not None, 'Timezone-aware clock required')
    exact_keys(context, ('schema', 'expected', 'observed'), 'Context')
    require(type(context['schema']) is int and context['schema'] == 1, 'Context schema')
    expected = context['expected']
    validate_expectations(expected)
    validate_observation(expected, context['observed'], now)
    require(len(archive) == expected['artifact']['bytes'] and sha256(archive) == expected['artifact']['sha256'], 'Downloaded archive digest/size mismatch')
    files = read_capsule(archive, expected['task_ids'])
    payload = validate_evidence(expected, files)
    return {
        'offline_checks_passed': True,
        'publish_authorized': False,
        'authenticated_provenance': False,
        'lease_consumed': False,
        'real_byte_ota_executed_by_this_tool': False,
        'warning': 'INACTIVE PROTOTYPE: supplied claims only; production activation blockers remain.',
        'payload': payload,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', required=True, type=Path)
    parser.add_argument('--context', required=True, type=Path)
    args = parser.parse_args()
    try:
        result = verify(read_regular(args.archive, MAX_ARCHIVE), strict_json(read_regular(args.context, MAX_JSON)))
    except (Rejected, TypeError, KeyError, OverflowError, RecursionError) as error:
        print(json.dumps({'offline_checks_passed': False, 'publish_authorized': False, 'error': str(error)}))
        return 1
    print(json.dumps(result, indent=2))
    return 0


if __name__ == '__main__':
    sys.exit(main())
