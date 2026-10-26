#!/usr/bin/env python3
"""Optional source-to-mask preprocessing. Normal rebuilds use audited mask exports.

Requires the already-installed Inkscape and Pillow, only for --render. No network.
Only a strict path-only sanitized SVG is passed to Inkscape; originals are preserved.
"""
import argparse
import gzip
import json
import os
from pathlib import Path
import subprocess
import tempfile

from fetch_public_api_icons import BASE, json_bytes, load_lock, retained_files, sha, verify_files


def render(base=BASE):
    from PIL import Image
    lock = load_lock(base)
    subset = verify_files(lock, retained_files(lock, base))
    exports = {}
    with tempfile.TemporaryDirectory(prefix='poket-icon-raster-') as temp:
        temp = Path(temp)
        env = dict(os.environ, INKSCAPE_PROFILE_DIR=str(temp / 'inkscape-profile'))
        version = subprocess.run(['inkscape', '--version'], check=True, capture_output=True, text=True, env=env).stdout.strip()
        for row in lock['icons']:
            slug = row['slug']
            source = ('<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 512 512">' +
                      subset['icons'][slug]['body'].replace('currentColor', '#ffffff') + '</svg>')
            svg, png = temp / 'input.svg', temp / 'output.png'
            svg.write_text(source)
            subprocess.run(['inkscape', str(svg), '--export-type=png', '--export-width=128',
                            '--export-height=128', '--export-area-page', '--export-filename=' + str(png)],
                           check=True, capture_output=True, env=env, timeout=30)
            with Image.open(png) as im:
                assert im.size == (128, 128)
                mask = im.convert('RGBA').getchannel('A').tobytes()
            compressed = gzip.compress(mask, mtime=0)
            path = base / 'sources/masks' / (slug + '.mask8.gz')
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(compressed)
            exports[slug] = {'file': 'sources/masks/' + path.name, 'width': 128, 'height': 128,
                             'body_sha256': row['body_sha256'], 'alpha_sha256': sha(mask),
                             'file_sha256': sha(compressed), 'decoded_bytes': len(mask)}
    record = {'schema_version': 1, 'renderer': version, 'method': 'white paths on transparent page; retain 128x128 alpha8',
              'source_subset_sha256': lock['iconify']['subset_sha256'], 'exports': exports}
    (base / 'raster-lock.json').write_bytes(json_bytes(record))
    print('Rendered 16 path-only masks; review previews before accepting any changed raster lock')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--render', action='store_true', required=True, help='Explicitly regenerate reviewed raster exports')
    parser.parse_args()
    render()
