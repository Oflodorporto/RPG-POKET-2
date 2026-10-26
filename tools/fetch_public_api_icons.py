#!/usr/bin/env python3
"""Verify retained Game Icons sources offline; --fetch reacquires a locked snapshot.

No API key, package installation, execution of downloaded code, or runtime use.
Only the reviewed 16-icon subset and original SVGs are retained, never caches.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
import urllib.request
import xml.etree.ElementTree as ET

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'assets/public-api-icons1'
MAX_SNAPSHOT = 10 * 1024 * 1024


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def json_bytes(value):
    return (json.dumps(value, ensure_ascii=False, indent=2) + '\n').encode()


def validate_body(body):
    """Allow paths only, so rasterization cannot request network or execute SVG code."""
    require(isinstance(body, str) and 0 < len(body) <= 100000, 'Invalid SVG body size')
    require('<!' not in body and '<?' not in body, 'SVG declarations forbidden')
    root = ET.fromstring('<svg>' + body + '</svg>')
    require(len(root) > 0, 'Empty SVG')
    require(not (root.text or '').strip(), 'Unexpected SVG text')
    for node in root:
        require(node.tag == 'path' and not len(node), 'Only flat SVG paths are supported')
        require(set(node.attrib) == {'fill', 'd'}, 'Unexpected SVG attributes')
        require(node.attrib['fill'] == 'currentColor', 'Unexpected SVG paint')
        require(bool(re.fullmatch(r'[MmZzLlHhVvCcSsQqTtAaEe0-9.,+\-\s]+', node.attrib['d'])),
                'Invalid SVG path data')
        require(not (node.text or '').strip() and not (node.tail or '').strip(), 'Unexpected SVG text')
    return body


def load_lock(base=BASE):
    lock = json.loads((base / 'source-lock.json').read_text())
    require(lock['schema_version'] == 1, 'Unknown lock version')
    require(lock['iconify']['collection_license'] == 'CC-BY-3.0', 'Unreviewed collection license')
    require(len(lock['icons']) == 16, 'Unexpected pack size')
    for source in ('iconify', 'game_icons'):
        require(bool(re.fullmatch('[0-9a-f]{40}', lock[source]['commit'])), 'Unpinned upstream commit')
    require(lock['iconify']['snapshot_url'] ==
            f"https://raw.githubusercontent.com/iconify/icon-sets/{lock['iconify']['commit']}/json/game-icons.json",
            'Unexpected collection origin')
    require(lock['game_icons']['license_url'] ==
            f"https://raw.githubusercontent.com/game-icons/icons/{lock['game_icons']['commit']}/license.txt",
            'Unexpected license origin')
    slugs = set()
    for index, row in enumerate(lock['icons']):
        slug = row['slug']
        require(bool(re.fullmatch('[a-z0-9]+(?:-[a-z0-9]+)*', slug)), 'Unsafe icon slug')
        require(slug not in slugs and row['id'] == index, 'Duplicate or unstable icon ID')
        slugs.add(slug)
        require(bool(re.fullmatch('[a-z0-9-]+/' + re.escape(slug) + r'\.svg', row['upstream_path'])),
                'Unsafe original path')
        require(row['upstream_url'] ==
                f"https://raw.githubusercontent.com/game-icons/icons/{lock['game_icons']['commit']}/{row['upstream_path']}",
                'Unexpected original origin')
        require(row['source_page'] == 'https://game-icons.net/1x1/' + row['upstream_path'][:-4] + '.html',
                'Unexpected attribution page')
        require(row['license'] == 'CC-BY-3.0' and row['license_url'] == 'https://creativecommons.org/licenses/by/3.0/',
                'Unreviewed icon license')
        require(bool(row['author'].strip()) and bool(row['title'].strip()), 'Missing individual credit')
    return lock


class NoRedirects(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        raise ValueError('Unexpected redirect; review and update the source lock manually')


def download(url, limit):
    require(url.startswith('https://raw.githubusercontent.com/'), 'Unapproved download host')
    req = urllib.request.Request(url, headers={'User-Agent': 'RPG-POKET-offline-assets/1'})
    with urllib.request.build_opener(NoRedirects).open(req, timeout=45) as response:
        data = response.read(limit + 1)
    require(len(data) <= limit, 'Oversized upstream response')
    return data


def subset_bytes(snapshot, lock):
    require(sha(snapshot) == lock['iconify']['snapshot_sha256'], 'Changed upstream snapshot')
    source = json.loads(snapshot)
    require(source['prefix'] == 'game-icons' and source['width'] == source['height'] == 512,
            'Unexpected collection geometry')
    require(source['info']['license']['spdx'] == 'CC-BY-3.0', 'Unexpected upstream license')
    subset = {'prefix': 'game-icons', 'width': 512, 'height': 512,
              'icons': {row['slug']: source['icons'][row['slug']] for row in lock['icons']}}
    return json_bytes(subset)


def verify_files(lock, files):
    raw = files['sources/iconify-subset.json']
    require(sha(raw) == lock['iconify']['subset_sha256'], 'Changed icon subset')
    subset = json.loads(raw)
    require(subset['prefix'] == 'game-icons' and subset['width'] == subset['height'] == 512,
            'Unexpected subset geometry')
    require(set(subset['icons']) == {row['slug'] for row in lock['icons']}, 'Missing or extra icon')
    for row in lock['icons']:
        icon = subset['icons'][row['slug']]
        require(set(icon) == {'body'}, 'Unsupported icon transform/metadata')
        body = validate_body(icon['body'])
        require(sha(body.encode()) == row['body_sha256'], 'Changed icon geometry')
        require(sha(files[f"sources/original/{row['slug']}.svg"]) == row['original_sha256'], 'Changed original SVG')
    require(sha(files['licenses/CC-BY-3.0-upstream.txt']) == lock['game_icons']['license_sha256'],
            'Changed upstream license')
    return subset


def retained_files(lock, base=BASE):
    names = ['sources/iconify-subset.json', 'licenses/CC-BY-3.0-upstream.txt']
    names += [f"sources/original/{row['slug']}.svg" for row in lock['icons']]
    return {name: (base / name).read_bytes() for name in names}


def fetch(lock, base=BASE):
    """Validate everything before replacing any retained source. No cache is persisted."""
    files = {'sources/iconify-subset.json': subset_bytes(download(lock['iconify']['snapshot_url'], MAX_SNAPSHOT), lock)}
    files['licenses/CC-BY-3.0-upstream.txt'] = download(lock['game_icons']['license_url'], 100000)
    for row in lock['icons']:
        files[f"sources/original/{row['slug']}.svg"] = download(row['upstream_url'], 100000)
    verify_files(lock, files)
    for name, data in files.items():
        path = base / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fetch', action='store_true', help='Explicit online reacquisition; all hashes stay pinned')
    args = parser.parse_args()
    lock = load_lock()
    if args.fetch:
        fetch(lock)
    verify_files(lock, retained_files(lock))
    print('PASS: 16 locked icons; individual attribution; source hashes; path-only SVG; ' +
          ('reacquired from pinned official snapshots' if args.fetch else 'offline verification, no network'))


if __name__ == '__main__':
    main()
