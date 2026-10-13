#!/usr/bin/env python3
"""Run unchanged real loaders against retained old and compact new PKA1 fixtures."""
from pathlib import Path
import argparse
import gzip
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import prepare_enemy_art as art
from unpack_art import reconstruct_legacy_pack


def old_catalog():
    baseline = json.loads((art.ASSETS / 'baseline/catalog.json').read_text())
    fallback = gzip.decompress((art.ASSETS / 'baseline/fallback.bin.gz').read_bytes())
    lines = ['#pragma once', '#include <stdint.h>', '#include <stddef.h>',
             'struct AssetMeta {uint32_t offset;uint16_t w,h;uint32_t fallback;uint8_t tw,th,elem;};',
             'constexpr uint32_t ART_BYTES=6991936,ART_CRC=0x53dcc3a5;',
             'constexpr unsigned ART_COUNT=488;', 'constexpr AssetMeta assetMeta[]={']
    lines += ['{' + ','.join(map(str, row)) + '},' for row in baseline['metadata']]
    lines += ['};', 'constexpr uint8_t artFallback[]={']
    lines += [','.join(map(str, fallback[i:i + 40])) + ',' for i in range(0, len(fallback), 40)]
    return '\n'.join(lines + ['};', art.CATALOG_TRAILER])


def copy_loader_dependencies(destination):
    """Stage unmodified production headers; only the generated catalog varies."""
    original = ROOT / 'firmware/RPG_POKET_2'
    pending, copied = ['AssetStore.h', 'DeviceSettings.h', 'SpriteArt.h', 'PersonalArt.h'], set()
    while pending:
        name = pending.pop()
        if name in copied:
            continue
        source = original / name
        text = source.read_text()
        target = destination / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        assert target.read_bytes() == source.read_bytes()
        copied.add(name)
        pending += re.findall(r'^#include\s+"([^"]+)"', text, re.M)
    return sorted(copied)



def published_touch77_readback(temporary, compiler, sanitizers, env):
    """Retain the exact published Save28 client's full636 read-back coverage."""
    import test_release77_ota as headers
    from test_scenes77_release import OLD_COMMIT, OLD_TREE

    tree = subprocess.check_output(['git', 'rev-parse', OLD_COMMIT + '^{tree}'],
                                   cwd=ROOT, text=True).strip()
    assert tree == OLD_TREE, 'Published touch77 source identity unavailable'
    staged = temporary / 'published-touch77'
    copied = headers.stage_headers(staged, OLD_COMMIT)
    source = temporary / 'published-readback.cpp'
    source.write_bytes(headers.source_bytes('tests/ota_art_readback.cpp', OLD_COMMIT))
    manifest = headers.source_bytes('tests/update_fake/visual77-manifest.json', OLD_COMMIT)
    assert manifest == (ROOT / 'tests/update_fake/visual77-manifest.json').read_bytes()
    fixture = temporary / 'published-manifest.json'
    fixture.write_bytes(manifest)
    flags = ['-std=c++17', '-O1', '-g', '-UNDEBUG', '-DRPG_TEST_VERIFY_DETAIL',
             '-Wno-deprecated-declarations']
    if sanitizers:
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
    executable = temporary / 'published-readback'
    subprocess.run([compiler, *flags, '-I', str(staged), '-I', str(staged / 'fake'),
                    str(source), str(staged / 'fake/cJSON.c'), '-lcrypto',
                    '-o', str(executable)], check=True)
    # new.pak here is the retained original636 pack, not the scenes77/666 pack.
    subprocess.run([str(executable), str(temporary / 'new.pak'), str(fixture)],
                   check=True, env=env)
    print(f'PASS: exact published touch77 {OLD_COMMIT}, tree {OLD_TREE}; '
          f'unchanged original read-back test and {len(copied)} production headers; '
          '16 real-SHA256 gates on the original636 pack. Host adapters only.')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitizers', action='store_true', help='Use ASan and UBSan; leak checking follows RPG_DETECT_LEAKS (default 0)')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    if not shutil.which(compiler):
        raise SystemExit(f'BLOCKED: C++ compiler {compiler} unavailable; no transition gates ran')
    result = art.build()
    art.verify_generated(result)
    old = reconstruct_legacy_pack()
    counts = []
    with tempfile.TemporaryDirectory(prefix='.transition-test-', dir=art.ASSETS) as temporary:
        temporary = Path(temporary)
        staged = temporary / 'firmware'
        copied = copy_loader_dependencies(staged)
        (temporary / 'old.pak').write_bytes(old)
        (temporary / 'new.pak').write_bytes(result.pack)
        for label, header in [('old', old_catalog()), ('new', result.catalog)]:
            (staged / 'AssetCatalog.h').write_text(header)
            executable = temporary / ('transition_' + label)
            flags = ['-std=c++17', '-O1', '-g', '-UNDEBUG']
            if args.sanitizers:
                flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
            command = [compiler, *flags, '-I', str(staged), '-I', str(ROOT / 'tests/settings_fake'),
                       str(ROOT / 'tests/enemy_art_transition.cpp'), '-o', str(executable)]
            subprocess.run(command, check=True)
            other = 'new' if label == 'old' else 'old'
            env = dict(os.environ)
            if args.sanitizers:
                env['ASAN_OPTIONS'] = env.get('ASAN_OPTIONS', '') + ':detect_leaks=' + env.get('RPG_DETECT_LEAKS', '0')
                env['UBSAN_OPTIONS'] = 'halt_on_error=1'
            run = subprocess.run([str(executable), str(temporary / (label + '.pak')), str(temporary / (other + '.pak')), label],
                                 check=False, capture_output=True, text=True, env=env)
            print(run.stdout, end='')
            if run.returncode:
                print(run.stderr, end='', file=sys.stderr)
                run.check_returncode()
            counts.append(int(re.search(r': (\d+) paired artifact gates;', run.stdout).group(1)))
        published_touch77_readback(temporary, compiler, args.sanitizers, env)
    print(f'PASS: {sum(counts)} paired package/SD-loader gates across old and new catalogs; {len(copied)} actual production dependency headers staged unchanged except catalog.')
    print('Coverage: representative first/middle/last chunk short reads; not exhaustive all-boundary fault injection. No hardware, OTA installer or full-art rollback-retention claim.')


if __name__ == '__main__':
    main()
