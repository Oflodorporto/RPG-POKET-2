#!/usr/bin/env python3
"""Byte-compare frozen combat against the candidate's default/null/captured APIs.

The probe emits every real Game member (ABI padding cleared), all 160 production
Save29 bytes, and the exact action response after each operation. Output is
compared in bounded streaming chunks; no digest collision can hide a regression.
No device files, fixture snapshots, firmware source, or baseline files change.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASELINE = 'ad8acd8b60f8c4c8bf961f87f8300445ad9d6be5'
FIRMWARE = 'firmware/RPG_POKET_2'


def stage_baseline(root, revision):
    subprocess.run(['git', 'cat-file', '-e', revision + '^{commit}'], cwd=ROOT, check=True)
    pending, done = ['Persistence.h'], set()
    while pending:
        name = pending.pop()
        if name in done:
            continue
        contents = subprocess.check_output(['git', 'show', f'{revision}:{FIRMWARE}/{name}'], cwd=ROOT)
        target = root / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(contents)
        done.add(name)
        pending += re.findall(r'^#include\s+"([^"]+)"', contents.decode(), re.M)
    return done


def read_chunk(stream, length):
    chunks, total = [], 0
    while total < length:
        chunk = stream.read(length - total)
        if not chunk:
            break
        chunks.append(chunk)
        total += len(chunk)
    return b''.join(chunks)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path, help='Read headers from this checkout instead of staging the frozen commit')
    parser.add_argument('--baseline-revision', default=BASELINE)
    parser.add_argument('--seeds', type=int, default=64)
    parser.add_argument('--save-format', type=int, choices=[29, 30], default=29,
                        help='Explicit shared ABI; Save30 integration requires a procedural-only baseline')
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--default-only', action='store_true', help='Useful before the optional trace API is implemented')
    parser.add_argument('--report', type=Path, help='Write the verified result and transcript SHA-256 as JSON')
    args = parser.parse_args()
    if args.seeds < 1:
        parser.error('--seeds must be positive')
    if args.save_format == 30:
        if not args.baseline:
            parser.error('Save30 requires the independently frozen procedural-only --baseline')
        pinned = json.loads((ROOT / 'tests/fixtures/package6-procedural-baseline.json').read_text())
        for name, expected in pinned['headers'].items():
            actual = hashlib.sha256((args.baseline / FIRMWARE / name).read_bytes()).hexdigest()
            if actual != expected:
                raise AssertionError('Frozen procedural-only baseline differs: ' + name)
    compiler = os.environ.get('CXX', 'g++')
    if not shutil.which(compiler):
        raise SystemExit(f'BLOCKED: compiler {compiler} is unavailable')
    flags = ['-std=c++17', '-O2', '-UNDEBUG', '-Wall', '-Wextra', '-Wno-misleading-indentation']
    if args.sanitizers:
        flags = ['-std=c++17', '-O1', '-g', '-UNDEBUG', '-fsanitize=address,undefined',
                 '-fno-omit-frame-pointer', '-no-pie']
    flags += [f'-DCOMBAT_TRACE_SAVE_FORMAT={args.save_format}']
    env = dict(os.environ)
    env['ASAN_OPTIONS'] = 'detect_leaks=' + env.get('RPG_DETECT_LEAKS', '0')
    env['UBSAN_OPTIONS'] = 'halt_on_error=1'
    processes = []
    with tempfile.TemporaryDirectory(prefix='combat-trace-') as temp:
        temp = Path(temp)
        if args.baseline:
            baseline = args.baseline.resolve() / FIRMWARE
            baseline_identity = str(args.baseline.resolve())
        else:
            baseline = temp / 'baseline'
            baseline.mkdir()
            stage_baseline(baseline, args.baseline_revision)
            baseline_identity = args.baseline_revision
        modes = [('baseline', baseline, []), ('candidate-default', ROOT / FIRMWARE, [])]
        if not args.default_only:
            modes += [('candidate-null', ROOT / FIRMWARE, ['-DCOMBAT_TRACE_NULL=1']),
                      ('candidate-captured', ROOT / FIRMWARE, ['-DCOMBAT_TRACE_CAPTURE=1'])]
        logs = {}
        try:
            for name, headers, define in modes:
                executable = temp / (name + '.exe')
                subprocess.run([compiler, *flags, *define, '-I', str(headers),
                                str(ROOT / 'tests/combat_trace_differential.cpp'),
                                '-o', str(executable)], check=True)
                logs[name] = (temp / (name + '.log')).open('w+b')
                processes.append((name, subprocess.Popen([str(executable), str(args.seeds)],
                                  stdout=subprocess.PIPE, stderr=logs[name], env=env)))
            headers = [read_chunk(proc.stdout, 20) for _, proc in processes]
            if any(header != headers[0] for header in headers[1:]):
                raise AssertionError('Game ABI or selected save size changed: ' + repr(headers))
            if len(headers[0]) != 20 or headers[0][:8] != b'CTDIFF01':
                raise AssertionError('Probe did not produce its expected header')
            game_size, save_size, record_size = struct.unpack('<III', headers[0][8:])
            sha = hashlib.sha256(headers[0])
            offset = 0
            while True:
                chunks = [read_chunk(proc.stdout, record_size * 2048) for _, proc in processes]
                reference = chunks[0]
                for (name, _), chunk in zip(processes[1:], chunks[1:]):
                    if chunk != reference:
                        mismatch = next((i for i, pair in enumerate(zip(reference, chunk))
                                         if pair[0] != pair[1]), min(len(reference), len(chunk)))
                        row = mismatch // record_size
                        key = reference[row * record_size:row * record_size + 40]
                        context = struct.unpack('<10I', key) if len(key) == 40 else ()
                        raise AssertionError(f'{name} differs at record {offset // record_size + row}, '
                                             f'byte {mismatch % record_size}; '
                                             f'(group,modern,class,level,seed,variant,action,enemy,turn,seed_index)={context}')
                if not reference:
                    break
                sha.update(reference)
                offset += len(reference)
            if offset % record_size:
                raise AssertionError('Truncated final record')
            summaries = {}
            for name, proc in processes:
                code = proc.wait()
                logs[name].seek(0)
                summary = logs[name].read().decode()
                if code:
                    raise AssertionError(f'{name} failed with status {code}:\n{summary}')
                summaries[name] = summary.strip()
            if len(set(summaries.values())) != 1:
                raise AssertionError('Coverage counters differ: ' + repr(summaries))
            report = {'result': 'PASS', 'baseline': baseline_identity, 'modes': [m[0] for m in modes],
                      'seeds': args.seeds, 'sanitizers': args.sanitizers, 'game_bytes': game_size,
                      'save_format': args.save_format, 'save_bytes': save_size, 'records': offset // record_size,
                      'bytes_compared_per_mode': offset + 20, 'transcript_sha256': sha.hexdigest(),
                      'coverage': summaries['baseline']}
            if args.save_format == 30:
                report['baseline_header_sha256'] = pinned['headers']
                report['baseline_patch_sha256'] = pinned['procedural_patch_sha256']
            if args.report:
                args.report.parent.mkdir(parents=True, exist_ok=True)
                args.report.write_text(json.dumps(report, indent=2) + '\n')
            print(json.dumps(report, indent=2))
        finally:
            for _, proc in processes:
                if proc.poll() is None:
                    proc.kill()
                proc.wait()
            for log in logs.values():
                log.close()


if __name__ == '__main__':
    main()
