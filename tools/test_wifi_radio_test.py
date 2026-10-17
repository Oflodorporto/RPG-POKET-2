#!/usr/bin/env python3
"""Fresh, assertion-enabled native Wi-Fi diagnostic suites with source-bound logs.

No RF, board, leak or release pass is inferred. LeakSanitizer is mandatory by
 default; RPG_DETECT_LEAKS=0 is an explicit development-only exception recorded
 verbatim in evidence. A failed sanitizer is never retried without that sanitizer.
"""
if not __debug__:
    raise RuntimeError('Qualification requires assertions; optimized Python is forbidden')

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
SOURCES = {
    'model': 'tests/wifi_radio_test_model.cpp',
    'flow': 'tests/wifi_radio_test_flow.cpp',
    'device': 'tests/wifi_radio_device.cpp',
    'races': 'tests/wifi_radio_device_races.cpp',
    'export': 'tests/wifi_radio_test_export.cpp',
    'runtime': 'tests/wifi_radio_test_runtime.cpp',
    'render-on': 'tests/wifi_radio_test_render.cpp',
    'render-off': 'tests/wifi_radio_test_render.cpp',
}
RUNNER = 'tools/test_wifi_radio_test.py'
RENDER_ON = {
    'baseline-not-coordinated.ppm', 'coordinating.ppm',
    *('details-%d.ppm' % i for i in range(1, 5)),
    'discovery-timeout.ppm', 'distance-two-palms.ppm', 'internet-test-preserved.ppm',
    'lobby.ppm', 'manual-levels.ppm', 'manual-one-meter.ppm',
    *('missing-details-%d.ppm' % i for i in range(1, 4)),
    'report-complete.ppm', 'report-missing-peer.ppm', 'report-recovery.ppm',
    'restore-error.ppm', 'running.ppm', 'wifi-saved-entry.ppm', 'wifi-browse-entry.ppm',
}
RENDER_OFF = {'internet-test-radio-off.ppm'}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot():
    paths = [ROOT / RUNNER]
    for directory in ('firmware', 'tests'):
        paths.extend(p for p in (ROOT / directory).rglob('*')
                     if p.is_file() and p.suffix in ('.h', '.hpp', '.c', '.cpp', '.ino'))
    return {str(p.relative_to(ROOT)): digest(p) for p in paths if not p.is_symlink()}


def execute(argv, logfile, environment, timeout, sanitizer_environment):
    started = time.monotonic()
    with logfile.open('w', encoding='utf-8') as log:
        log.write('argv: ' + json.dumps(argv) + '\n')
        log.write('command: ' + shlex.join(argv) + '\n')
        log.write('cwd: ' + str(ROOT) + '\n')
        log.write('sanitizer_environment: ' + json.dumps(sanitizer_environment, sort_keys=True) + '\n')
        log.flush()
        try:
            result = subprocess.run(argv, cwd=ROOT, env=environment, stdout=log,
                                    stderr=subprocess.STDOUT, timeout=timeout, check=False)
            code = result.returncode
        except subprocess.TimeoutExpired:
            log.write('\nTIMEOUT: the process exceeded its bounded test deadline\n')
            code = 124
        except OSError as error:
            log.write('\nEXECUTION ERROR: ' + str(error) + '\n')
            code = 127
        log.write('\nexit_code: ' + str(code) + '\n')
        log.write('elapsed_seconds: %.3f\n' % (time.monotonic() - started))
    return code


def source_map(suite, dependency_file, before):
    paths = {RUNNER, SOURCES[suite]}
    if dependency_file.is_file():
        text = dependency_file.read_text().replace('\\\n', ' ')
        if ':' in text:
            for filename in shlex.split(text.split(':', 1)[1]):
                path = Path(filename)
                path = path if path.is_absolute() else ROOT / path
                if path.is_symlink() or not path.resolve().is_relative_to(ROOT):
                    raise RuntimeError('Dependency is outside the recorded source tree: ' + filename)
                paths.add(str(path.resolve().relative_to(ROOT)))
    current = {name: digest(ROOT / name) for name in sorted(paths)}
    changed = [name for name, value in current.items() if before.get(name) != value]
    return current, changed


def run_suite(suite, out, sanitized, leaks):
    out.mkdir(parents=True, exist_ok=True)
    out = out.resolve()
    binary, dependency = out / ('wifi-radio-' + suite), out / 'dependencies.d'
    # Never reuse a stale depfile or executable after a failed compilation.
    for path in (binary, dependency):
        if path.exists():
            path.unlink()
    before = snapshot()
    compiler = shlex.split(os.environ.get('CXX', 'g++'))
    flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-UNDEBUG', '-g']
    flags += ['-O1', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie'] if sanitized else ['-O2']
    flags += ['-I', str(ROOT / 'firmware/RPG_POKET_2'), '-I', str(ROOT / 'tests')]
    if suite in ('device', 'races', 'runtime', 'render-on'):
        flags += ['-I', str(ROOT / 'tests/wifi_radio_fake'), '-pthread', '-DRPG_ENABLE_WIFI_RADIO_TEST=1']
    elif suite == 'render-off':
        flags += ['-DRPG_ENABLE_WIFI_RADIO_TEST=0']
    command = compiler + flags + ['-MMD', '-MF', str(dependency), str(ROOT / SOURCES[suite]), '-o', str(binary)]
    env = os.environ.copy()
    sanitizer_environment = ({'ASAN_OPTIONS': 'detect_leaks=' + leaks + ':halt_on_error=1',
                              'UBSAN_OPTIONS': 'halt_on_error=1',
                              'LSAN_OPTIONS': 'exitcode=23'} if sanitized else {})
    env.update(sanitizer_environment)
    compile_log, run_log = out / 'compile.log', out / 'run.log'
    compile_code = execute(command, compile_log, env, 300, sanitizer_environment)
    argv = [str(binary)]
    capture_dir = out / ('captures-' + str(time.time_ns()))
    if suite in ('runtime', 'render-on', 'render-off'):
        argv.append(str(capture_dir))
    if compile_code == 0:
        run_code = execute(argv, run_log, env, 180, sanitizer_environment)
    else:
        run_code = None
        run_log.write_text('argv (not executed): ' + json.dumps(argv) + '\nNOT RUN: compilation failed\n')
    try:
        sources, changed = source_map(suite, dependency, before)
        source_error = None
    except (OSError, RuntimeError, ValueError) as error:
        sources = {name: before[name] for name in (RUNNER, SOURCES[suite]) if name in before}
        changed, source_error = [], str(error)
    captures = {}
    capture_error = None
    if suite in ('render-on', 'render-off') and run_code == 0:
        files = list(capture_dir.glob('*.ppm'))
        expected = RENDER_ON if suite == 'render-on' else RENDER_OFF
        if {path.name for path in files} != expected:
            capture_error = 'Render capture set does not match the required fixture set'
        for path in sorted(files):
            data = path.read_bytes()
            if not data.startswith(b'P6\n240 320\n255\n') or len(data) != 15 + 240 * 320 * 3:
                capture_error = 'Invalid 240x320 P6 capture: ' + path.name
            captures[path.name] = {'path': str(path), 'sha256': digest(path)}
    passed = (compile_code == 0 and run_code == 0 and not changed
              and source_error is None and capture_error is None)
    canonical = json.dumps(sources, sort_keys=True, separators=(',', ':')).encode()
    report = {
        'suite': suite, 'passed': passed, 'sanitizer': sanitized,
        'source_sha256': hashlib.sha256(canonical).hexdigest(), 'sources_sha256': sources,
        'sanitizer_environment': sanitizer_environment,
        'logs': {'compile': {'path': str(compile_log), 'sha256': digest(compile_log)},
                 'run': {'path': str(run_log), 'sha256': digest(run_log)}},
        'compile_exit_code': compile_code, 'run_exit_code': run_code,
        'sources_changed_during_execution': changed, 'source_error': source_error,
        'captures': captures, 'capture_error': capture_error,
    }
    (out / 'results.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(('PASS' if passed else 'FAIL') + ': ' + suite + (' ASan/UBSan, detect_leaks=' + leaks if sanitized else ' normal') + ' -> ' + str(out / 'results.json'), flush=True)
    if not passed:
        print('  Compile log: ' + str(compile_log) + '\n  Run log: ' + str(run_log), flush=True)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--suite', choices=SOURCES)
    parser.add_argument('--out', type=Path, default=Path('build/wifi-radio-test/fresh'))
    parser.add_argument('--sanitizers', action='store_true')
    args = parser.parse_args()
    leaks = os.environ.get('RPG_DETECT_LEAKS', '1')
    if leaks not in ('0', '1'):
        parser.error('RPG_DETECT_LEAKS must be 1 (qualification) or explicit 0 (development only)')
    suites = [args.suite] if args.suite else list(SOURCES)
    reports = [run_suite(suite, args.out if args.suite else args.out / suite, args.sanitizers, leaks)
               for suite in suites]
    if not args.suite:
        (args.out / 'results.json').write_text(json.dumps({'passed': all(r['passed'] for r in reports),
                                                         'suites': reports}, indent=2, sort_keys=True) + '\n')
    return 0 if all(report['passed'] for report in reports) else 1


if __name__ == '__main__':
    sys.exit(main())
