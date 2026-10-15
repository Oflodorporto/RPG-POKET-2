#!/usr/bin/env python3
"""Coop input-loss regression; no flashing, publication or source rewriting.

Normal mode is a local diagnostic, not release qualification. Sanitizer mode
requires ASan/UBSan/LSan, with no local-environment fallback or leak suppression.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
FLAGS = ['-std=c++17', '-UNDEBUG', '-Wall', '-Wextra', '-Werror',
         '-Wno-misleading-indentation', '-Wno-sign-compare',
         '-Wno-unused-function', '-Wno-switch', '-Wno-unused-parameter']
SAN_FLAGS = ['-O1', '-g', '-fno-omit-frame-pointer', '-no-pie',
             '-fsanitize=address,undefined']
SAN_ENV = {'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1:strict_string_checks=1',
           'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1',
           'LSAN_OPTIONS': 'exitcode=23'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=('normal', 'sanitizers', 'both'), default='both')
    parser.add_argument('--output-dir', type=Path, default=ROOT/'build/coop-touch-queue')
    args = parser.parse_args()
    if args.mode != 'normal' and os.environ.get('RPG_DETECT_LEAKS', '1') != '1':
        raise SystemExit('LeakSanitizer is mandatory; RPG_DETECT_LEAKS must be 1')
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    fixture = ROOT/'tests/package12_coop_controller.cpp'
    text = fixture.read_text()
    if text.count('int main(){') != 1:
        raise SystemExit('Controller fixture main signature drift')
    # Helpers and controller stay verbatim. Only the unused test entry point
    # becomes void, avoiding any warning suppression or duplicate main symbol.
    (output/'coop_controller_fixture.generated.h').write_text(
        text.replace('int main(){', 'void frozen_package12_controller_main(){'))
    sources = [fixture, ROOT/'tests/coop_touch_queue_regression.cpp',
               ROOT/'tools/test_coop_touch_queue_regression.py',
               ROOT/'firmware/RPG_POKET_2/CoopRuntime.cpp',
               ROOT/'firmware/RPG_POKET_2/CoopDevice.h',
               *sorted((ROOT/'firmware/RPG_POKET_2/Coop').glob('*.h'))]
    report = {'passed': False, 'mode': args.mode, 'cases': [],
              'sources_sha256': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in sources},
              'limits': 'Native simulated radio/touch only; physical cause, ESP32 build and full release qualification are separate.'}
    summary = output/'results.json'
    summary.write_text(json.dumps(report, indent=2)+'\n')
    modes = ('normal', 'sanitizers') if args.mode == 'both' else (args.mode,)
    compiler = shlex.split(os.environ.get('CXX', 'g++'))
    for mode in modes:
        executable = output/('regression-'+mode)
        command = [*compiler, *FLAGS, *(['-O2'] if mode == 'normal' else SAN_FLAGS),
                   '-DEXPECT_COALESCED=1', '-DRPG_ENABLE_COOP_TRAINING=1',
                   '-I'+str(output), '-I'+str(ROOT/'tests'), '-Itests/coop_fake',
                   'tests/coop_touch_queue_regression.cpp',
                   'firmware/RPG_POKET_2/CoopRuntime.cpp', '-o', str(executable)]
        env = dict(os.environ)
        if mode == 'sanitizers':
            env.update(SAN_ENV)
        with (output/(mode+'.log')).open('w') as log:
            for cmd in (command, [str(executable)]):
                line = '+ '+shlex.join(cmd)
                print(line, flush=True)
                log.write(line+'\n')
                log.flush()
                result = subprocess.run(cmd, cwd=ROOT, env=env, text=True,
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=240)
                print(result.stdout, end='', flush=True)
                log.write(result.stdout)
                log.flush()
                result.check_returncode()
        report['cases'].append({'mode': mode, 'passed': True,
                                'sanitizer_environment': SAN_ENV if mode == 'sanitizers' else None})
        summary.write_text(json.dumps(report, indent=2)+'\n')
    report['passed'] = True
    summary.write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: touch-queue regression '+args.mode+'; not physical or full release qualification')


if __name__ == '__main__':
    main()
