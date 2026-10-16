#!/usr/bin/env python3
"""Fresh host-only dependencies derived from the reviewed benchmark lock."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
from urllib.request import urlopen
from urllib.parse import urlparse

HERE = Path(__file__).resolve().parent
DEPS = HERE.parent / 'ci-acceleration/dependencies'
LOCK_SHA = 'd03e64207ca2c45c81f01ecf64ff8faf230f6552e12c7d96d552d68491c2fa78'


def wheels():
    raw = (DEPS / 'downloads.lock.json').read_bytes()
    if hashlib.sha256(raw).hexdigest() != LOCK_SHA:
        raise ValueError('Reviewed dependency lock changed')
    lock = json.loads(raw)
    rows = [x for x in lock['artifacts'] if x['path'].startswith('wheels/')]
    if len(rows) != 2 or {Path(x['path']).name.split('-')[0].lower() for x in rows} != {'pillow', 'pyyaml'}:
        raise ValueError('Expected exact two-wheel closure')
    for row in rows:
        if Path(row['path']).parts != ('wheels', Path(row['path']).name) or not row['path'].endswith('.whl'):
            raise ValueError('Unsafe wheel path')
        url = urlparse(row['url'])
        if url.scheme != 'https' or url.hostname != 'files.pythonhosted.org' or url.username or url.password:
            raise ValueError('Wheel must use pinned official PyPI download')
    expected = ''.join(name + '==' + version + ' --hash=sha256:' + next(x['sha256'] for x in rows if Path(x['path']).name.lower().startswith(name+'-')) + '\n'
        for name, version in [('pillow', '11.3.0'), ('pyyaml', '6.0.3')])
    if (DEPS / 'requirements.lock.txt').read_text() != expected:
        raise ValueError('Requirements are not the exact reviewed wheel projection')
    return lock, rows


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--workspace', type=Path, required=True)
    args = p.parse_args()
    start = time.monotonic_ns()
    lock, rows = wheels()
    if sys.version.split()[0] != lock['context']['python_version']:
        raise ValueError('Python patch differs from reviewed lock; do not improvise setup')
    root = args.workspace.absolute()
    if root != root.resolve():
        raise ValueError('Canonical fresh workspace required')
    root.mkdir(mode=0o700)
    downloads = root / 'wheels'; downloads.mkdir()
    for row in rows:
        path = downloads / Path(row['path']).name
        with urlopen(row['url'], timeout=120) as response, path.open('xb') as out:
            count = 0; digest = hashlib.sha256()
            while block := response.read(1024*1024):
                count += len(block)
                if count > row['bytes']:
                    raise ValueError('Oversized wheel')
                digest.update(block); out.write(block)
        if count != row['bytes'] or digest.hexdigest() != row['sha256']:
            raise ValueError('Wheel byte identity mismatch')
    downloaded = time.monotonic_ns()
    subprocess.run([sys.executable, '-m', 'venv', str(root / 'venv')], check=True)
    with (root / 'install.log').open('xb') as log:
        subprocess.run([str(root/'venv/bin/python'), '-m', 'pip', 'install', '--no-index', '--no-deps',
            '--no-cache-dir', '--no-compile', '--only-binary=:all:', '--require-hashes',
            '--find-links', str(downloads), '-r', str(DEPS/'requirements.lock.txt')], stdout=log, stderr=subprocess.STDOUT, check=True)
    end = time.monotonic_ns()
    report = {'schema': 1, 'status': 'installed-fresh-host-wheels-only', 'lock_sha256': LOCK_SHA,
        'python': sys.version.split()[0], 'wheels': rows, 'download_bytes': sum(x['bytes'] for x in rows),
        'download_and_verify_ns': downloaded-start, 'venv_install_ns': end-downloaded, 'total_setup_ns': end-start,
        'cache_restored': False, 'arduino_or_esp32_installed': False}
    (root/'setup.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
