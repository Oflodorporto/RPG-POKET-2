#!/usr/bin/env python3
"""Check cap-10 downgrade behavior against the exact pre-change header closure."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASELINE = 'dfc68c49f3264d4f2d3642266d44b82ca9b7c563'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitizers', action='store_true')
args = parser.parse_args()
output = ROOT / 'build' / 'ration-limit10-oldreader'
frozen = output / 'frozen'
frozen.mkdir(parents=True, exist_ok=True)
pending = ['Persistence.h']
seen = set()
while pending:
    name = pending.pop()
    if name in seen:
        continue
    assert '/' not in name and name.endswith('.h'), name
    seen.add(name)
    contents = subprocess.check_output(
        ['git', 'show', f'{BASELINE}:firmware/RPG_POKET_2/{name}'], cwd=ROOT)
    (frozen / name).write_bytes(contents)
    pending.extend(re.findall(r'^#include "([^"]+)"', contents.decode(), re.MULTILINE))
mode = 'san' if args.sanitizers else 'normal'
flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie'] if args.sanitizers else ['-O2']
executable = output / f'oldreader-{mode}'
subprocess.run(shlex.split(os.environ.get('CXX', 'g++')) +
               ['-std=c++17', '-UNDEBUG'] + flags +
               ['-I', str(frozen), str(ROOT / 'tests/ration_limit10_oldreader.cpp'), '-o', str(executable)], check=True, cwd=ROOT)
env = os.environ.copy()
env['ASAN_OPTIONS'] = 'detect_leaks=' + env.get('RPG_DETECT_LEAKS', '1')
env['UBSAN_OPTIONS'] = 'halt_on_error=1'
subprocess.run([str(executable)], check=True, cwd=ROOT, env=env)
print(f'Frozen {len(seen)} headers from {BASELINE}; candidate and historical source trees untouched.', flush=True)
