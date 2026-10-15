#!/usr/bin/env python3
"""Bounded invitation regression against actual firmware/controller headers.

Local-only native qualification. No SDK installation, flashing, remote writes or
publication. --expect-baseline reproduces the historical 12-second admission
timeout against a separately supplied, frozen public source tree.
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
    parser.add_argument('--output-dir', type=Path, default=ROOT/'build/coop-invite-countdown')
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--expect-baseline', action='store_true')
    args = parser.parse_args()
    if args.mode != 'normal' and os.environ.get('RPG_DETECT_LEAKS', '1') != '1':
        raise SystemExit('LeakSanitizer is mandatory; RPG_DETECT_LEAKS must be 1')
    source = args.source_root.resolve()
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    fixture = source/'tests/package12_coop_controller.cpp'
    fixture_text = fixture.read_text()
    if fixture_text.count('int main(){') != 1:
        raise SystemExit('Controller fixture main signature drift')
    # Preserve the existing helpers and production controller verbatim. The
    # unused fixture entry point is only renamed and given a void return type.
    (output/'coop_invite_controller_fixture.generated.h').write_text(
        fixture_text.replace('int main(){', 'void frozen_invite_controller_main(){'))
    test = ROOT/'tests/coop_invite_countdown_regression.cpp'
    files = [fixture, source/'tests/title_controller.cpp',
             source/'firmware/RPG_POKET_2/CoopRuntime.cpp',
             source/'firmware/RPG_POKET_2/CoopDevice.h',
             *sorted((source/'firmware/RPG_POKET_2/Coop').glob('*.h'))]
    report = {
        'passed': False, 'mode': args.mode, 'baseline': args.expect_baseline,
        'source_root': str(source), 'cases': [],
        'sources_sha256': {str(p.relative_to(source)): hashlib.sha256(p.read_bytes()).hexdigest()
                           for p in files},
        'regression_sha256': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                              for p in (test, Path(__file__).resolve())},
        'limits': 'Native copied-frame and controller simulation only; ESP32/RF/LCD and complete release qualification are separate gates.',
    }
    summary = output/'results.json'
    summary.write_text(json.dumps(report, indent=2)+'\n')
    modes = ('normal', 'sanitizers') if args.mode == 'both' else (args.mode,)
    compiler = shlex.split(os.environ.get('CXX', 'g++'))
    for mode in modes:
        executable = output/('regression-'+mode)
        command = [*compiler, *FLAGS, *(['-O2'] if mode == 'normal' else SAN_FLAGS),
                   '-DEXPECT_INVITE_COUNTDOWN='+str(int(not args.expect_baseline)),
                   '-DRPG_ENABLE_COOP_TRAINING=1', '-I'+str(output),
                   '-I'+str(source/'tests'), '-I'+str(source/'tests/coop_fake'),
                   str(test), str(source/'firmware/RPG_POKET_2/CoopRuntime.cpp'),
                   '-o', str(executable)]
        env = dict(os.environ)
        if mode == 'sanitizers':
            env.update(SAN_ENV)
        with (output/(mode+'.log')).open('w') as log:
            for cmd in (command, [str(executable)]):
                line = '+ '+shlex.join(cmd)
                print(line, flush=True)
                log.write(line+'\n')
                log.flush()
                result = subprocess.run(cmd, cwd=source, env=env, text=True,
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
    label = 'historical 12-second reproduction' if args.expect_baseline else '30-second invitation regression'
    print('PASS: '+label+' '+args.mode+'; native evidence only')


if __name__ == '__main__':
    main()
