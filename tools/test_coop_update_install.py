#!/usr/bin/env python3
"""Focused real multi-TU installer gates; no SDK, network, flash or release claim.

Compile the production UpdateInstall.cpp separately, then link the untouched
current OTA fault harnesses. No .cpp inclusion or alternate installer is used.
The historical release77_ota harness is intentionally not rewritten: it pins
Save28/636 assets while this candidate uses Save30/666 assets.
"""
from pathlib import Path
import argparse
import importlib.util
import json
import os
import re
import shlex
import shutil
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('installer_equivalence', ROOT / 'tests/coop_update_install_equivalence.py')
EQUIVALENCE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(EQUIVALENCE)
CASES = ('coop_update_install_linkage', 'update_adapter', 'ota_recovery',
         'ota_art_readback', 'ota_sd_diagnostics', 'enemy_ota')
REAL_SHA = {'ota_recovery', 'ota_art_readback'}
FORBIDDEN = {'UpdateDevice.h', 'Menu.h', 'View.h', 'Save.h', 'AssetCatalog.h'}
CURRENT_LOG = None


def run(command, env=None, capture=False):
    rendered = '+ '+shlex.join(map(str, command))
    print(rendered, flush=True)
    result = subprocess.run(list(map(str, command)), cwd=ROOT, env=env,
                            text=True, capture_output=True, timeout=180)
    if CURRENT_LOG:
        with CURRENT_LOG.open('a') as stream:
            stream.write(rendered+'\n'+result.stdout+result.stderr)
    if not capture or result.returncode:
        print(result.stdout, end='', flush=True)
        print(result.stderr, file=sys.stderr, end='', flush=True)
    result.check_returncode()
    return result


def require(ok, message):
    if not ok:
        raise ValueError(message)


def symbols(path, defined=True):
    command = ['nm', '-C']
    if defined:
        command.append('--defined-only')
    command.append(path)
    output = run(command, capture=True).stdout
    result = []
    for line in output.splitlines():
        match = re.match(r'^\s*(?:[0-9a-fA-F]+\s+)?([A-Za-z?])\s+(.+)$', line)
        if match:
            result.append((match.group(1), match.group(2)))
    return result, output


def function_symbols(entries, name):
    return [(kind, symbol) for kind, symbol in entries
            if symbol.startswith(name+'(') and symbol.endswith(')')]


def verify_installer(path):
    entries, output = symbols(path, defined=False)
    require(function_symbols(entries, 'installUpdate') == [('T', 'installUpdate(bool)')],
            f'{path}: missing unique strong installer definition')
    for state in EQUIVALENCE.STATES:
        matches = [(kind, name) for kind, name in entries if name == state]
        require(len(matches)==1 and matches[0][0] in {'B', 'D'}, f'{path}: non-singleton state {state}')
    for helper in EQUIVALENCE.HELPERS:
        matches = function_symbols(entries, helper)
        require(len(matches)==1 and matches[0][0]=='U', f'{path}: expected external helper {helper}')
    dependency_file = path.with_suffix('.d')
    deps = dependency_file.read_text().replace('\\\n', ' ').split()[1:]
    require(not (FORBIDDEN & {Path(dep).name for dep in deps}),
            f'{path}: installer pulled sketch/gameplay headers')
    path.with_suffix('.nm.txt').write_text(output)


def verify_link(executable, sketch_object):
    entries, output = symbols(executable)
    for name in ('installUpdate', *EQUIVALENCE.HELPERS):
        matches = function_symbols(entries, name)
        require(len(matches)==1 and matches[0][0]=='T', f'{executable}: non-unique strong function {name}')
    for state in EQUIVALENCE.STATES:
        matches = [(kind, name) for kind, name in entries if name==state]
        require(len(matches)==1 and matches[0][0] in {'B', 'D'}, f'{executable}: non-singleton state {state}')
    sketch, _ = symbols(sketch_object)
    require(not function_symbols(sketch, 'installUpdate'), f'{sketch_object}: installer body leaked into sketch TU')
    require(not any(name in EQUIVALENCE.STATES for _, name in sketch),
            f'{sketch_object}: shared state defined in sketch TU')
    executable.with_suffix('.nm.txt').write_text(output)


def main():
    global CURRENT_LOG
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=('normal', 'sanitizers', 'both'), default='both')
    parser.add_argument('--output-dir', type=Path, default=ROOT/'build/coop-update-install')
    args = parser.parse_args()
    if args.mode in {'sanitizers', 'both'}:
        require(os.environ.get('RPG_DETECT_LEAKS', '1') == '1',
                'LeakSanitizer is mandatory; RPG_DETECT_LEAKS must be 1')
    compiler = os.environ.get('CXX', 'g++')
    require(shutil.which(compiler) and shutil.which('nm'), 'C++ compiler and nm are required')
    run(['pkg-config', '--exists', 'openssl'])
    EQUIVALENCE.main()
    run([sys.executable, 'tools/unpack_art.py'])
    run([sys.executable, 'tools/unpack_art.py', '--baseline-only', '--output', 'build/enemy_transition/old.pak'])
    passes = ('normal', 'sanitizers') if args.mode=='both' else (args.mode,)
    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    report = {'production_installer': 'firmware/RPG_POKET_2/UpdateInstall.cpp',
              'mode': args.mode, 'cases': [], 'limits': 'Host fakes only; no ESP32 link, size, hardware, or release qualification.'}
    for mode in passes:
        CURRENT_LOG = output_dir/f'update-install-{mode}.log'
        CURRENT_LOG.write_text(f'Focused installer multi-TU qualification: {mode}\n')
        folder = output_dir/'update-install'/mode
        folder.mkdir(parents=True, exist_ok=True)
        flags = ['-std=c++17', '-UNDEBUG']
        flags += ['-O2'] if mode=='normal' else ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
        includes = ['-Itests/update_fake', '-Ifirmware/RPG_POKET_2']
        common = [compiler, *flags, *includes, '-Wno-deprecated-declarations']
        cjson = folder/'cJSON.o'
        run([*common, '-c', 'tests/update_fake/cJSON.c', '-o', cjson])
        installers = {}
        for crypto in ('fake', 'sha256'):
            obj = folder/f'UpdateInstall_{crypto}.o'
            defs = ['-DRPG_TEST_REAL_SHA256'] if crypto=='sha256' else []
            run([*common, *defs, '-MMD', '-MF', obj.with_suffix('.d'), '-c',
                 'firmware/RPG_POKET_2/UpdateInstall.cpp', '-o', obj])
            verify_installer(obj)
            installers[crypto] = obj
        for name in CASES:
            crypto = 'sha256' if name in REAL_SHA else 'fake'
            obj = folder/(name+'.o')
            exe = folder/name
            run([*common, '-DRPG_TEST_VERIFY_DETAIL', '-c', f'tests/{name}.cpp', '-o', obj])
            libraries = ['-lcrypto'] if crypto=='sha256' else []
            run([compiler, *flags, obj, installers[crypto], cjson, *libraries, '-o', exe])
            verify_link(exe, obj)
            env = dict(os.environ)
            if mode=='sanitizers':
                env['ASAN_OPTIONS'] = 'detect_leaks=1:halt_on_error=1'
                env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
                env['LSAN_OPTIONS'] = 'exitcode=23'
            result = run([exe], env=env, capture=True)
            print(result.stdout, end='', flush=True)
            if result.stderr:
                print(result.stderr, file=sys.stderr, end='', flush=True)
            exe.with_suffix('.run.txt').write_text(result.stdout+result.stderr)
            report['cases'].append({'mode': mode, 'name': name, 'crypto': crypto, 'passed': True})
    (output_dir/'update-install-results.json').write_text(json.dumps(report, indent=2)+'\n')
    CURRENT_LOG = None
    print(f'PASS: {len(report["cases"])} executions; real separate installer objects; singleton states/helpers; clean dependency boundary')
    print(report['limits'])


if __name__=='__main__':
    try:
        main()
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        raise SystemExit(f'FAIL: {error}')
