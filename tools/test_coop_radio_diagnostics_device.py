#!/usr/bin/env python3
"""Local Device diagnostics regression against real firmware and fake driver callbacks.

No hardware, flashing, remote writes, SDK installation or release publication.
ASan, UBSan and leak detection are mandatory in the sanitizer mode.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
FLAGS = ['-std=c++17', '-UNDEBUG', '-DRPG_ENABLE_COOP_TRAINING=1', '-Wall', '-Wextra', '-Werror',
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
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--output-dir', type=Path, default=ROOT/'build/coop-radio-diagnostics-device')
    args = parser.parse_args()
    if args.mode != 'normal' and os.environ.get('RPG_DETECT_LEAKS', '1') != '1':
        raise SystemExit('LeakSanitizer is mandatory; RPG_DETECT_LEAKS must be 1')
    source = args.source_root.resolve()
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    test = ROOT/'tests/coop_radio_diagnostics_device.cpp'
    fixture = source/'tests/coop_training_radio.cpp'
    files = [fixture, *sorted((source/'firmware/RPG_POKET_2/Coop').glob('*.h')),
             *sorted((source/'tests/coop_fake').rglob('*.h'))]
    report = {
        'passed': False, 'mode': args.mode, 'feature_enabled': 1, 'source_root': str(source), 'cases': [],
        'sources_sha256': {str(path.relative_to(source)): hashlib.sha256(path.read_bytes()).hexdigest()
                           for path in files},
        'regression_sha256': {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                              for path in (test, Path(__file__).resolve())},
        'limits': 'Native Device/Session callback and fake-driver simulation only. ESP32/RF, physical LCD and complete release qualification are separate gates.',
    }
    summary = output/'results.json'
    summary.write_text(json.dumps(report, indent=2)+'\n')
    compiler = shlex.split(os.environ.get('CXX', 'g++'))
    modes = ('normal', 'sanitizers') if args.mode == 'both' else (args.mode,)
    for mode in modes:
        executable = output/('regression-'+mode)
        command = [*compiler, *FLAGS, '-pthread', *(['-O2'] if mode == 'normal' else SAN_FLAGS),
                   '-I'+str(source/'tests'), '-I'+str(source/'tests/coop_fake'),
                   '-I'+str(source/'firmware/RPG_POKET_2'), str(test), '-o', str(executable)]
        env = dict(os.environ)
        if mode == 'sanitizers':
            env.update(SAN_ENV)
        case = {'mode': mode, 'passed': False,
                'sanitizer_environment': SAN_ENV if mode == 'sanitizers' else None}
        report['cases'].append(case)
        try:
            with (output/(mode+'.log')).open('w') as log:
                for stage, cmd in [('compile', command), ('run', [str(executable)])]:
                    line = '+ '+shlex.join(cmd)
                    print(line, flush=True)
                    log.write(line+'\n')
                    log.flush()
                    result = subprocess.run(cmd, cwd=source, env=env, text=True,
                                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                            timeout=240)
                    print(result.stdout, end='', flush=True)
                    log.write(result.stdout)
                    log.flush()
                    case[stage+'_returncode'] = result.returncode
                    result.check_returncode()
                    if stage == 'compile':
                        case['executable_sha256'] = hashlib.sha256(executable.read_bytes()).hexdigest()
            case['log_sha256'] = hashlib.sha256((output/(mode+'.log')).read_bytes()).hexdigest()
            case['passed'] = True
        except (subprocess.SubprocessError, OSError) as error:
            case['error'] = str(error)
            raise
        finally:
            summary.write_text(json.dumps(report, indent=2)+'\n')
    report['passed'] = True
    summary.write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: Device diagnostics '+args.mode+'; native evidence only')


if __name__ == '__main__':
    main()
