#!/usr/bin/env python3
"""Package12 capture gate: reuse the frozen renderer with mandatory LSan.

The historical candidate wrapper is deliberately unchanged. This wrapper pins
Coop ON for every g++ compile and ASan/UBSan/LSan for every child,
validates both capture sets and pixel parity,
and records the enforcement settings alongside the existing source identities.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import os
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
FEATURE_NAME = 'RPG_ENABLE_COOP_TRAINING'
FEATURE_DEFINE = '-D'+FEATURE_NAME+'=1'
SANITIZER_ENV = {
    'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1:strict_string_checks=1',
    'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1',
    'LSAN_OPTIONS': 'exitcode=23',
}


def feature_command(command):
    """Pin the real renderer ON; reject conflicting or hidden flag overrides."""
    command = list(map(str, command))
    compiler = Path(command[0]).name
    if compiler != 'g++' and not compiler.startswith('g++-'):
        return command
    pinned = [command[0], FEATURE_DEFINE]
    index = 1
    while index < len(command):
        value = command[index]
        # The frozen wrapper never needs response files. Reject one rather
        # than permit an unexamined later -D/-U to override this gate.
        if value.startswith('@'):
            raise ValueError('Renderer compiler response files are not permitted')
        if value in ('-D', '-U') and index+1 < len(command) and FEATURE_NAME in command[index+1]:
            if value != '-D' or command[index+1] != FEATURE_NAME+'=1':
                raise ValueError('Renderer requires '+FEATURE_DEFINE)
            index += 2
            continue
        if FEATURE_NAME in value:
            if value != FEATURE_DEFINE:
                raise ValueError('Renderer requires '+FEATURE_DEFINE)
        else:
            pinned.append(value)
        index += 1
    return pinned


def strict_run(command, **kwargs):
    # Replace any inherited or historical suppression; never fall back to
    # detect_leaks=0 if this host cannot run LeakSanitizer (including ptrace).
    env = dict(kwargs.pop('env', os.environ))
    env.update(SANITIZER_ENV)
    return subprocess.run(feature_command(command), check=True, timeout=180, env=env, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT/'build/package12-coop-render')
    parser.add_argument('--art', type=Path, required=True)
    parser.add_argument('--mode', choices=('normal', 'both'), default='both')
    args = parser.parse_args()
    if args.mode == 'both' and os.environ.get('RPG_DETECT_LEAKS', '1') != '1':
        raise ValueError('LeakSanitizer is mandatory; RPG_DETECT_LEAKS must be 1')
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    manifest_path = out/'manifest.json'
    manifest_path.write_text(json.dumps({'package': 12, 'passed': False, 'sanitized': False,
                                         'pixel_parity_verified': False, 'mode': args.mode}, indent=2)+'\n')
    spec = importlib.util.spec_from_file_location('frozen_coop_capture', ROOT/'tools/capture_coop_training.py')
    legacy = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(legacy)
    legacy.run = strict_run
    sys.argv = [str(ROOT/'tools/capture_coop_training.py'), '--out', str(args.out), '--art', str(args.art)]
    if args.mode == 'normal':
        sys.argv.append('--no-sanitize')
    legacy.main()
    manifest = json.loads(manifest_path.read_text())
    manifest['passed'] = False
    manifest_path.write_text(json.dumps(manifest, indent=2)+'\n')
    normal = {p.name for p in (out/'screens').glob('*.ppm')}
    if len(normal) != 58 or len(normal) != len(manifest['captures']):
        raise ValueError('Missing or unexpected normal captures')
    if args.mode == 'both':
        sanitized = {p.name for p in (out/'sanitized').glob('*.ppm')}
        if normal != sanitized:
            raise ValueError('Normal and sanitized capture sets differ')
        for name in normal:
            if (out/'screens'/name).read_bytes() != (out/'sanitized'/name).read_bytes():
                raise ValueError('Normal/sanitizer pixel mismatch: '+name)
        # Labels are executable renderer output too; catch typography drift.
        for file in (out/'screens').glob('*-labels.tsv'):
            if file.read_bytes() != (out/'sanitized'/file.name).read_bytes():
                raise ValueError('Normal/sanitizer label mismatch: '+file.name)
    manifest.update(package=12, passed=True, mode=args.mode, feature_enabled=1,
                    compile_definitions=[FEATURE_DEFINE],
                    sanitizer_environment=SANITIZER_ENV if args.mode=='both' else None,
                    pixel_parity_verified=args.mode=='both',
                    gate_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    manifest_path.write_text(json.dumps(manifest, indent=2)+'\n')
    print(f'PASS: {len(normal)} actual-render captures; normal/sanitizer parity '+('verified with ASan/UBSan/LSan' if args.mode=='both' else 'not run (normal-only)'))


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        raise SystemExit(f'FAIL: {error}')
