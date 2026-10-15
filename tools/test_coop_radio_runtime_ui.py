#!/usr/bin/env python3
"""Active coop UI/controller integration; no device, network or publication.

Normal mode is host evidence only. Optional sanitizer mode requires ASan,
UBSan and LSan, with no leak suppression or environment fallback.
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
    parser.add_argument('--mode', choices=('normal', 'sanitizers', 'both'), default='normal')
    parser.add_argument('--output-dir', type=Path, default=ROOT/'build/coop-radio-runtime-ui')
    args = parser.parse_args()
    if args.mode != 'normal' and os.environ.get('RPG_DETECT_LEAKS', '1') != '1':
        raise SystemExit('LeakSanitizer is mandatory; RPG_DETECT_LEAKS must be 1')
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    fixture = ROOT/'tests/package12_coop_controller.cpp'
    source = fixture.read_text()
    if source.count('int main(){') != 1:
        raise SystemExit('Controller fixture main signature drift')
    # Reuse helpers and actual controller verbatim. Its unused entry point is
    # void because renaming an int main with implicit return is otherwise UB.
    (output/'coop_controller_fixture.generated.h').write_text(
        source.replace('int main(){', 'void frozen_package12_controller_main(){'))
    sources = [ROOT/'tests/coop_radio_runtime_ui.cpp', Path(__file__).resolve(),
               fixture, ROOT/'tests/title_controller.cpp', ROOT/'tests/SceneCanvas.h',
               ROOT/'tests/ControllerStore29.h', ROOT/'firmware/RPG_POKET_2/View.h',
               ROOT/'firmware/RPG_POKET_2/CoopUi.h', ROOT/'firmware/RPG_POKET_2/CoopDevice.h',
               ROOT/'firmware/RPG_POKET_2/CoopRuntime.cpp',
               *sorted((ROOT/'firmware/RPG_POKET_2/Coop').glob('*.h')),
               *sorted(p for p in (ROOT/'tests/coop_fake').rglob('*') if p.is_file())]
    report = {'passed': False, 'mode': args.mode, 'feature_enabled': 1, 'cases': [],
              'sources_sha256': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in sources},
              'limits': 'Native simulated radio/input and art-independent runtime dispatch only; not hardware, authored-art or release qualification.'}
    summary = output/'results.json'
    summary.write_text(json.dumps(report, indent=2)+'\n')
    compiler = shlex.split(os.environ.get('CXX', 'g++'))
    for mode in (('normal', 'sanitizers') if args.mode == 'both' else (args.mode,)):
        executable = output/('runtime-ui-'+mode)
        command = [*compiler, *FLAGS, *(['-O2'] if mode == 'normal' else SAN_FLAGS),
                   '-DRPG_ENABLE_COOP_TRAINING=1', '-I'+str(output), '-Itests', '-Itests/coop_fake',
                   'tests/coop_radio_runtime_ui.cpp', 'firmware/RPG_POKET_2/CoopRuntime.cpp',
                   '-o', str(executable)]
        env = dict(os.environ)
        if mode == 'sanitizers':
            env.update(SAN_ENV)
        try:
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
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
            report['cases'].append({'mode': mode, 'passed': False, 'error': str(error),
                                    'sanitizer_environment': SAN_ENV if mode == 'sanitizers' else None})
            summary.write_text(json.dumps(report, indent=2)+'\n')
            raise
        report['cases'].append({'mode': mode, 'passed': True,
                                'sanitizer_environment': SAN_ENV if mode == 'sanitizers' else None})
        summary.write_text(json.dumps(report, indent=2)+'\n')
    report['passed'] = True
    summary.write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: active runtime/controller UI '+args.mode+'; not hardware or release qualification')


if __name__ == '__main__':
    main()
