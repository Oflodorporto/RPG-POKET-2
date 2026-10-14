#!/usr/bin/env python3
"""Package12 focused native gates, keeping historical candidate gates frozen.

Run the package12 scope gate before this runner. A normal-only run is useful
locally but is not sanitizer or release qualification. This runner never
installs SDKs, flashes, pushes, publishes, or suppresses LeakSanitizer.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shlex
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
FLAGS = ['-std=c++17', '-UNDEBUG', '-Wall', '-Wextra', '-Werror',
         '-Wno-misleading-indentation', '-Wno-sign-compare',
         '-Wno-unused-function', '-Wno-switch', '-Wno-unused-parameter']
SAN_FLAGS = ['-O1', '-g', '-fno-omit-frame-pointer', '-no-pie', '-fsanitize=address,undefined']
SANITIZER_ENV = {
    'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1:strict_string_checks=1',
    'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1',
    'LSAN_OPTIONS': 'exitcode=23',
}
CASES = ('session', 'radio', 'input', 'device', 'pvp_exclusion', 'default_off')
FORBIDDEN_HEADERS = {'Persistence.h', 'Save.h', 'Dungeon.h', 'AssetCatalog.h',
                     'PersonalArt.h', 'EnemyArt.h', 'CoopView.h',
                     'CoopPortraitBridge.h', 'CoopEligibilityBridge.h',
                     'View.h', 'MenuView.h', 'Menu.h', 'UpdateDevice.h'}
LOG = None


def require(ok, message):
    if not ok:
        raise ValueError(message)


def run(command, env=None, quiet=False):
    line = '+ '+shlex.join(map(str, command))
    print(line, flush=True)
    result = subprocess.run(list(map(str, command)), cwd=ROOT, env=env,
                            text=True, capture_output=True, timeout=240)
    if LOG:
        with LOG.open('a') as stream:
            stream.write(line+'\n'+result.stdout+result.stderr)
    if not quiet or result.returncode:
        print(result.stdout, end='', flush=True)
        print(result.stderr, file=sys.stderr, end='', flush=True)
    result.check_returncode()
    return result.stdout


def extras(name):
    result = []
    if name in {'radio', 'device', 'input', 'controller'}:
        result += ['-Itests/coop_fake']
    if name in {'device', 'input', 'controller'}:
        result += ['-DRPG_ENABLE_COOP_TRAINING=1', 'firmware/RPG_POKET_2/CoopRuntime.cpp']
    if name == 'default_off':
        result += ['firmware/RPG_POKET_2/CoopRuntime.cpp']
    if name == 'pvp_exclusion':
        result += ['-Itests/club_fake', '-Ifirmware/RPG_POKET_2',
                   'firmware/RPG_POKET_2/ClubProtocol.cpp', 'firmware/RPG_POKET_2/ClubDuel.cpp']
    return result


def runtime_gates(compiler, output, default_off):
    global LOG
    LOG = output/'coop-runtime-boundary.log'
    LOG.write_text('Package12 runtime/default-off dependency and symbol gates\n')
    symbols = run(['nm', '-C', default_off], quiet=True)
    require(not re.search(r'coopSession|coopInput|coopDevice|coopruntime::|coopradio::|coopplatform::Session', symbols),
            'Cooperative runtime symbol leaked into host default-off executable')
    runtime = 'firmware/RPG_POKET_2/CoopRuntime.cpp'
    objects = []
    for name, definitions in [('host-default-off', []),
                              ('arduino-explicit-off', ['-DARDUINO=10819', '-DRPG_ENABLE_COOP_TRAINING=0'])]:
        obj = output/(name+'.o')
        run([*compiler, *FLAGS, '-O2', *definitions, '-c', runtime, '-o', obj])
        require(not run(['nm', '--defined-only', obj], quiet=True).strip(),
                name+': disabled runtime has definitions/static initialization')
        objects.append(obj)
    # No feature flag override: the production ARDUINO default must enable it.
    obj = output/'runtime-arduino-default-on.o'
    defs = ['-DARDUINO=10819', '-Itests/coop_fake']
    deps = run([*compiler, *FLAGS, *defs, '-MM', runtime], quiet=True)
    (output/'coop-runtime-dependencies.log').write_text(deps)
    dep_paths = deps.replace('\\\n', ' ').split()[1:]
    require(not (FORBIDDEN_HEADERS & {Path(p).name for p in dep_paths}) and
            not any('/font/' in p for p in dep_paths),
            'Legacy art/save/view headers leaked into runtime translation unit')
    run([*compiler, *FLAGS, '-O2', *defs, '-c', runtime, '-o', obj])
    defined = run(['nm', '-S', '-C', '--defined-only', obj], quiet=True)
    (output/'coop-runtime-symbols.log').write_text(defined)
    for name in ('coopSession', 'coopInput', 'coopDevice'):
        require(len(re.findall(r'\b[BbDd] '+name+r'$', defined, re.MULTILINE)) == 1,
                'Default-ON runtime must define exactly one '+name)
    require(not re.search(r' assetMeta$| artFallback$| font$| artMemory$|enemyArt::workspace|ruins7Art::', defined, re.MULTILINE),
            'Art storage leaked into the runtime object')
    run(['size', obj, *objects])
    print('PASS: host default OFF, Arduino explicit OFF empty; Arduino default ON singleton runtime; no art/save dependencies', flush=True)


def main():
    global LOG
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=('normal', 'sanitizers', 'both'), default='both')
    parser.add_argument('--output-dir', type=Path, default=ROOT/'build/package12-coop')
    args = parser.parse_args()
    if args.mode != 'normal':
        require(os.environ.get('RPG_DETECT_LEAKS', '1') == '1',
                'LeakSanitizer is mandatory; RPG_DETECT_LEAKS must be 1')
    compiler = shlex.split(os.environ.get('CXX', 'g++'))
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    summary = output/'results.json'
    report = {'package': 12, 'mode': args.mode, 'modes': ['normal', 'sanitizers'] if args.mode=='both' else [args.mode],
              'sanitized': args.mode!='normal', 'passed': False, 'cases': [],
              'sanitizer_environment': SANITIZER_ENV if args.mode!='normal' else None,
              'limits': 'Native fakes only; ESP32 compilation, hardware/RF/LCD and release qualification are separate gates.'}
    summary.write_text(json.dumps(report, indent=2)+'\n')
    LOG = output/'coop-config-and-snapshot.log'
    LOG.write_text('Package12 configuration and exact controller snapshot gates\n')
    run([sys.executable, 'tests/package12_coop_config.py'])
    run([sys.executable, 'tools/check_controller_snapshot.py'])
    passes = ('normal', 'sanitizers') if args.mode=='both' else (args.mode,)
    for mode in passes:
        for name in (*CASES, 'controller'):
            if name == 'default_off' and mode == 'sanitizers':
                continue
            folder = output/mode
            folder.mkdir(exist_ok=True)
            LOG = output/f'coop-{name}-{mode}.log'
            LOG.write_text(f'Package12 {name}: {mode}\n')
            source = 'tests/package12_coop_controller.cpp' if name=='controller' else f'tests/coop_training_{name}.cpp'
            executable = folder/name
            run([*compiler, *FLAGS, *(['-O2'] if mode=='normal' else SAN_FLAGS),
                 source, *extras(name), '-o', executable])
            env = dict(os.environ)
            if mode == 'sanitizers':
                env.update(SANITIZER_ENV)
            run([executable], env=env)
            report['cases'].append({'name': name, 'mode': mode, 'passed': True})
    # Even --mode sanitizers retains the required optimized default-off gate.
    default_off = output/'normal/default_off'
    if 'normal' not in passes:
        default_off.parent.mkdir(exist_ok=True)
        LOG = output/'coop-default_off-normal.log'
        LOG.write_text('Required host default-off gate\n')
        run([*compiler, *FLAGS, '-O2', 'tests/coop_training_default_off.cpp', *extras('default_off'), '-o', default_off])
        run([default_off])
        report['cases'].append({'name': 'default_off', 'mode': 'normal', 'passed': True})
    runtime_gates(compiler, output, default_off)
    for name in ('club_protocol', 'club_duel', 'pvp_reliability_save30'):
        LOG = output/f'coop-legacy-{name}.log'
        LOG.write_text('Unchanged existing PvP optimized gate\n')
        executable = output/name
        run([*compiler, '-std=c++17', '-O2', '-UNDEBUG', '-Ifirmware/RPG_POKET_2', '-Itests/club_fake',
             f'tests/{name}.cpp', 'firmware/RPG_POKET_2/ClubProtocol.cpp', 'firmware/RPG_POKET_2/ClubDuel.cpp', '-o', executable])
        run([executable])
        report['cases'].append({'name': name, 'mode': 'normal', 'passed': True})
    report['sources_sha256'] = {path: hashlib.sha256((ROOT/path).read_bytes()).hexdigest()
                               for path in ('firmware/RPG_POKET_2/CoopConfig.h',
                                            'firmware/RPG_POKET_2/CoopRuntime.cpp',
                                            'firmware/RPG_POKET_2/CoopDevice.h',
                                            'firmware/RPG_POKET_2/RPG_POKET_2.ino',
                                            'tests/package12_coop_controller.cpp',
                                            'tools/test_package12_coop.py',
                                            'tools/capture_package12_coop.py')}
    report['passed'] = True
    summary.write_text(json.dumps(report, indent=2)+'\n')
    LOG = None
    print(f'PASS: {len(report["cases"])} native executions; config/snapshot and runtime dependency/symbol gates ({args.mode})')
    if args.mode=='normal':
        print('Sanitizers NOT RUN; normal-only is not full qualification.')
    print(report['limits'])


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        raise SystemExit(f'FAIL: {error}')
