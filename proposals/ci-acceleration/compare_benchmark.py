#!/usr/bin/env python3
"""Compare a complete cold/warm pair. Input reports do not authorize releases.

This is a local consistency checker, not an authenticated Actions API adapter.
Reports are supplied by a reviewed collector; cryptographic identity and signed
approval cannot be inferred from these JSON files alone.
"""
import argparse
import json
from pathlib import Path
import re
from cache_policy import PolicyError, TrustedLock, exact_cache_key, validate_context


class Rejected(ValueError):
    pass


def require(value, reason):
    if not value:
        raise Rejected(reason)


def digest(value, size=64):
    return type(value) is str and re.fullmatch('[0-9a-f]{%d}' % size, value)


def exact(value, keys, label):
    require(type(value) is dict and set(value) == set(keys), label + ': exact fields required')


PHASES = ('dependency_restore', 'dependency_install', 'dependency_verify',
          'aggregate', 'esp32_on', 'esp32_off', 'final_bytes_ota',
          'identity', 'cache_save', 'evidence_upload')
GATES = ('source_inventory', 'original_config_tests', 'preparation',
         'aggregate_157', 'captures_and_parity', 'esp32_default_on',
         'esp32_forced_off', 'compiled_budget_and_linkage',
         'release_semantics', 'ota_normal', 'ota_sanitizers', 'final_identity')
IDENTITY = ('source_commit', 'source_tree', 'source_inventory_sha256',
            'reviewed_plan_sha256', 'adapter_sha256', 'dependency_lock_sha256',
            'runner_image', 'runner_image_version', 'runner_os', 'runner_arch', 'python',
            'pillow', 'pyyaml', 'host_compiler', 'arduino_cli', 'esp32_core',
            'target_compiler', 'fqbn', 'host_jobs', 'esp32_jobs', 'orchestrator_sha')
SOURCE_COMMIT = '6026fb49db4fe874b9c3f0571cbe4fdb9e66adb3'
SOURCE_TREE = '5ad62cddfe052bea09bf99c29989731722088981'
SOURCE_INVENTORY_SHA256 = '7c81909372ead3217d0133fb49d74e877206a5a6031faaf933b09d59ea03a6fe'
PLAN_SHA256 = '244e8d63d05fc55ad35c493abb2fca6ba398d1618bef91f2d00be8965db344a5'
FQBN = 'esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,EraseFlash=none'
HOST_COMPILER = 'g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0'


def cache_key(identity, salt, run_id=None, attempt=None):
    context = {'runner_os': identity['runner_os'], 'runner_arch': identity['runner_arch'],
               'python_version': identity['python'], 'arduino_cli_version': identity['arduino_cli'],
               'esp32_core_version': identity['esp32_core'], 'pillow_version': identity['pillow'],
               'pyyaml_version': identity['pyyaml'], 'fqbn': identity['fqbn'], 'benchmark_salt': salt}
    try:
        dimensions = validate_context(context)
    except PolicyError as error:
        raise Rejected('Invalid cache dimensions: ' + str(error)) from error
    # Key reconstruction only; the report must separately supply a verified cache.
    # Empty artifacts here never constitute a trusted production lock.
    base = exact_cache_key(TrustedLock(identity['dependency_lock_sha256'], dimensions, ()))
    if run_id is None and attempt is None:
        return base
    require(type(run_id) is int and run_id > 0 and type(attempt) is int and attempt > 0, 'Invalid cache experiment namespace')
    return base + '-run-' + str(run_id) + '-attempt-' + str(attempt)


def validate(report, role):
    exact(report, ('schema', 'kind', 'identity', 'run', 'cache', 'gates',
                   'coverage', 'phases_ns', 'job_ns', 'release_files',
                   'publish_authorized'), 'Report')
    require(type(report['schema']) is int and report['schema'] == 1, 'Schema mismatch')
    require(report['kind'] == 'benchmark-only-' + role, 'Report role mismatch')
    require(report['publish_authorized'] is False, 'Benchmark cannot authorize publication')
    identity = report['identity']
    exact(identity, IDENTITY, 'Identity')
    for name in ('source_commit', 'source_tree', 'orchestrator_sha'):
        require(digest(identity[name], 40), 'Invalid ' + name)
    for name in ('source_inventory_sha256', 'reviewed_plan_sha256', 'adapter_sha256', 'dependency_lock_sha256'):
        require(digest(identity[name]), 'Invalid ' + name)
    require(identity['source_commit'] == SOURCE_COMMIT and identity['source_tree'] == SOURCE_TREE,
            'This experiment must use the reviewed coopdiag77 source')
    require(identity['reviewed_plan_sha256'] == PLAN_SHA256, 'Reviewed command plan changed')
    require(identity['source_inventory_sha256'] == SOURCE_INVENTORY_SHA256, 'Reviewed aggregate source inventory changed')
    for name in set(IDENTITY) - {'host_jobs', 'esp32_jobs'}:
        require(type(identity[name]) is str and identity[name].strip(), 'Missing identity: ' + name)
    require(type(identity['host_jobs']) is int and identity['host_jobs'] == 3, 'Host concurrency changed')
    require(type(identity['esp32_jobs']) is int and identity['esp32_jobs'] == 4, 'Target concurrency changed')
    require(identity['esp32_core'] == '3.3.12', 'Historical core changed')
    require(identity['arduino_cli'] == '1.5.2-rc.1', 'CLI changed: establish a separate baseline')
    require(identity['pillow'] == '11.3.0' and identity['pyyaml'] == '6.0.3', 'Python dependencies changed')
    require(identity['runner_image'] == 'ubuntu-24.04' and identity['runner_os'] == 'Linux' and
            identity['runner_arch'] == 'X64', 'Unapproved runner platform')
    require(re.fullmatch(r'20\d{6}\.\d+\.\d+', identity['runner_image_version']), 'Missing exact runner image version')
    require(re.fullmatch(r'3\.12\.\d+', identity['python']), 'Missing exact Python 3.12 patch version')
    require(identity['host_compiler'] == HOST_COMPILER and identity['target_compiler'] == 'esp-x32/2601',
            'Compiler changed: establish a separate baseline')
    require(identity['fqbn'] == FQBN, 'Reviewed FQBN changed')
    run = report['run']
    exact(run, ('repository', 'id', 'attempt', 'job_id', 'branch', 'status'), 'Run')
    require(run['repository'] == 'Oflodorporto/RPG-POKET-2', 'Repository mismatch')
    require(run['branch'] == 'bench/coopdiag77-cache-promotion', 'Benchmark branch mismatch')
    for key in ('id', 'attempt', 'job_id'):
        require(type(run[key]) is int and run[key] > 0, 'Invalid run ID')
    require(run['status'] == 'success', 'Incomplete or failed benchmark')
    cache = report['cache']
    exact(cache, ('key', 'benchmark_salt', 'hit', 'restored', 'writer_run_id', 'writer_job_id',
                  'contents_verified', 'forbidden_paths', 'bytes'), 'Cache')
    require(cache['key'] == cache_key(identity, cache['benchmark_salt'], run['id'], run['attempt']),
            'Exact cache key/context/lock/experiment binding changed')
    require(type(cache['bytes']) is int and cache['bytes'] > 0, 'Missing cache size')
    require(all(type(cache[name]) is int and cache[name] > 0 for name in ('writer_run_id', 'writer_job_id')),
            'Invalid cache writer identity')
    require(cache['contents_verified'] is True and cache['forbidden_paths'] == [], 'Unsafe cache')
    if role == 'cold':
        require(cache['hit'] is False and cache['restored'] is False, 'Cold run restored cache')
        require(cache['writer_run_id'] == run['id'] and cache['writer_job_id'] == run['job_id'], 'Cold cache writer mismatch')
    else:
        require(cache['hit'] is True and cache['restored'] is True, 'Warm run is not an exact cache hit')
    exact(report['gates'], GATES, 'Gates')
    require(all(value == 'passed' for value in report['gates'].values()), 'Missing or failed gate')
    require(report['coverage'] == {'groups': 157, 'skipped': 0, 'resumed': 0,
                                 'lsan': 'detect_leaks=1', 'asan': True,
                                 'ubsan_halt_on_error': True, 'assertions': True},
            'Coverage or sanitizer contract changed')
    # Dict equality treats True like 1: enforce numeric counter types separately.
    require(all(type(report['coverage'][key]) is int for key in ('groups', 'skipped', 'resumed')),
            'Coverage counters must be integers')
    require(all(report['coverage'][key] is True for key in ('asan', 'ubsan_halt_on_error', 'assertions')),
            'Sanitizer flags must be booleans')
    exact(report['phases_ns'], PHASES, 'Timed phases')
    require(all(type(value) is int and value >= 0 for value in report['phases_ns'].values()), 'Invalid duration')
    require(all(report['phases_ns'][name] > 0 for name in PHASES if name not in ('dependency_restore', 'cache_save')),
            'Required phase has no duration')
    require(report['phases_ns']['cache_save' if role == 'cold' else 'dependency_restore'] > 0,
            'Cache seed/save or warm restoration overhead was omitted')
    require(type(report['job_ns']) is int and report['job_ns'] >= sum(report['phases_ns'].values()),
            'Job duration omits measured work')
    exact(report['release_files'], ('firmware.bin', 'artes.pak', 'manifest.json'), 'Release triple')
    for entry in report['release_files'].values():
        exact(entry, ('bytes', 'sha256'), 'Release file')
        require(type(entry['bytes']) is int and entry['bytes'] > 0 and digest(entry['sha256']), 'Invalid release digest/size')


def compare(cold, warm):
    validate(cold, 'cold')
    validate(warm, 'warm')
    require(cold['identity'] == warm['identity'], 'Source, scope, versions, runner image or adapter changed')
    require((cold['run']['id'], cold['run']['attempt']) == (warm['run']['id'], warm['run']['attempt']),
            'Cold/warm must be separate jobs of the same workflow attempt')
    require((cold['run']['id'], cold['run']['job_id']) != (warm['run']['id'], warm['run']['job_id']),
            'The same job is not an independent cold/warm pair')
    require(cold['cache']['key'] == warm['cache']['key'], 'Cache keys differ')
    require(warm['cache']['writer_run_id'] == cold['run']['id'] and
            warm['cache']['writer_job_id'] == cold['run']['job_id'], 'Warm cache does not come from this cold job')
    require(cold['cache']['bytes'] == warm['cache']['bytes'], 'Cache contents changed size')
    phases = {name: {'cold_ns': cold['phases_ns'][name], 'warm_ns': warm['phases_ns'][name],
                     'saved_ns': cold['phases_ns'][name] - warm['phases_ns'][name]} for name in PHASES}
    return {'schema': 1, 'controlled_pair_consistent': True,
            'authenticated_provenance': False, 'publish_authorized': False,
            'sample_pairs': 1, 'cold_job_ns': cold['job_ns'], 'warm_job_ns': warm['job_ns'],
            'job_saved_ns': cold['job_ns'] - warm['job_ns'], 'phases': phases,
            'note': 'One pair measures observed time only. Negative savings are retained. '
                    'Each firmware was validated independently; recompilation need not be byte-identical. '
                    'Cache transfer/verification/save are included. No publication-time saving is inferred.'}


def load(path):
    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, 'Duplicate JSON key')
            result[key] = value
        return result
    def bad(_):
        raise Rejected('Floating-point/nonfinite values are forbidden')
    return json.loads(path.read_bytes(), object_pairs_hook=pairs, parse_float=bad, parse_constant=bad)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cold', type=Path)
    parser.add_argument('warm', type=Path)
    args = parser.parse_args()
    print(json.dumps(compare(load(args.cold), load(args.warm)), indent=2))


if __name__ == '__main__':
    try:
        main()
    except (Rejected, ValueError, OSError) as error:
        raise SystemExit('REJECTED: ' + str(error))
