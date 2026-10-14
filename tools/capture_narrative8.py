#!/usr/bin/env python3
"""Capture package 8 from the real native renderer, then index every unique page.

No firmware/source/assets are changed. All generated evidence lives under --output.
"""
from __future__ import annotations
import argparse
import hashlib
import html
import json
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]

def run(cmd, log):
    result = subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.write('$ ' + ' '.join(map(str, cmd)) + '\n' + result.stdout + '\n')
    log.flush()
    if result.returncode:
        print(result.stdout, file=sys.stderr)
        raise SystemExit(result.returncode)
    if result.stdout.strip():
        if cmd[0] == 'g++':
            print(f'Native build passed; {result.stdout.count("warning:")} existing/source warnings retained in run.log')
        else:
            print(result.stdout.strip())

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/narrative8-render')
    parser.add_argument('--skip-render', action='store_true', help='Only rebuild PNGs/indexes from existing successful render output')
    args = parser.parse_args()
    out = args.output.resolve()
    for directory in ('ppm', 'png', 'contact-sheets'):
        (out / directory).mkdir(parents=True, exist_ok=True)
    source_files = ['tests/narrative8_render.cpp', 'tools/capture_narrative8.py'] + [str(p.relative_to(ROOT)) for p in (ROOT / 'firmware/RPG_POKET_2').glob('*.h')] + ['firmware/RPG_POKET_2/RPG_POKET_2.ino']
    hashes = {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in source_files}
    if not args.skip_render:
        with (out / 'run.log').open('w') as log:
            pack = out / 'artes.pak'
            if not pack.exists():
                run([sys.executable, 'tools/unpack_art.py', '--output', str(pack)], log)
            run(['g++', '-std=c++17', '-O2', '-UNDEBUG', '-Wall', '-Wextra', 'tests/narrative8_render.cpp', '-o', str(out / 'narrative8_render')], log)
            run([str(out / 'narrative8_render'), str(out), str(pack)], log)
        changed = [p for p in source_files if hashlib.sha256((ROOT / p).read_bytes()).hexdigest() != hashes[p]]
        if changed:
            raise SystemExit('Sources changed during capture; rerun for a stable snapshot: ' + ', '.join(changed))
        (out / 'source-sha256.json').write_text(json.dumps(hashes, indent=2)+'\n')
    summary = json.loads((out / 'summary.json').read_text())
    frames = [json.loads(line) for line in (out / 'frames.jsonl').read_text().splitlines()]
    unique = [frame for frame in frames if frame['unique']]
    assert len(unique) == summary['unique_previews']
    for frame in unique:
        dest = out / frame['image']
        with Image.open(out / 'ppm' / (dest.stem + '.ppm')) as image:
            assert image.size == (240, 320)
            image.save(dest, optimize=True)
    expected_images = {Path(frame['image']).name for frame in unique}
    for stale in (out / 'png').glob('*.png'):
        if re.fullmatch(r'\d{4}-.+\.png', stale.name) and stale.name not in expected_images:
            stale.unlink()
    expected_ppms = {Path(name).with_suffix('.ppm').name for name in expected_images}
    for stale in (out / 'ppm').glob('*.ppm'):
        if re.fullmatch(r'\d{4}-.+\.ppm', stale.name) and stale.name not in expected_ppms:
            stale.unlink()
    font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 12)
    sheets = []
    for start in range(0, len(unique), 20):
        batch = unique[start:start+20]
        sheet = Image.new('RGB', (5 * 256, 4 * 376), '#e4e1d9')
        draw = ImageDraw.Draw(sheet)
        for i, frame in enumerate(batch):
            x, y = (i % 5) * 256 + 8, (i // 5) * 376 + 8
            with Image.open(out / frame['image']) as im:
                sheet.paste(im, (x, y))
            label = f"{Path(frame['image']).stem}\n{frame['scenario'][:32]}\nPage {frame['page']}/{frame['pages']}"
            draw.multiline_text((x, y+324), label, font=font, fill='#222222', spacing=2)
        name = f'contact-sheets/{start//20+1:03d}.jpg'
        sheet.save(out / name, quality=92)
        sheets.append(name)
    for stale in (out / 'contact-sheets').glob('*.jpg'):
        if re.fullmatch(r'\d{3}\.jpg', stale.name) and str(stale.relative_to(out)) not in sheets:
            stale.unlink()
    groups = Counter(frame['group'] for frame in unique)
    links = ' '.join(f'<a href="{sheet}">Sheet {i+1}</a>' for i, sheet in enumerate(sheets))
    cards = []
    for frame in unique:
        labels = '\n'.join(label['text'] for label in frame['labels'])
        cards.append(f'<article data-group="{html.escape(frame["group"])}"><a href="{frame["image"]}"><img width="240" height="320" src="{frame["image"]}" loading="lazy"></a><h3>{html.escape(Path(frame["image"]).stem)}</h3><p>{html.escape(frame["scenario"])}<br>Page {frame["page"]}/{frame["pages"]}</p><details><summary>Exact rendered text</summary><pre>{html.escape(labels)}</pre></details></article>')
    (out / 'index.html').write_text('<!doctype html><meta charset="utf-8"><title>RPG POKET · Package 8 native narrative pages</title><style>body{background:#ece9e0;color:#252623;font:15px system-ui;margin:24px}main{display:grid;grid-template-columns:repeat(auto-fill,minmax(256px,1fr));gap:20px}article{background:white;padding:12px;overflow-wrap:anywhere}img{image-rendering:pixelated;display:block;margin:auto}h3{font-size:14px}pre{white-space:pre-wrap}a{color:#355740}nav{line-height:2.2}</style><h1>Package 8 narrative · native 240 × 320</h1><p>Real View.h, firmware font, and validated artes.pak. Every page rendered; duplicate text/layout captured once. Game and Save30 snapshots unchanged before/after every frame. Every printed glyph checked for screen bounds and final visibility.</p><p>'+html.escape(json.dumps(summary, ensure_ascii=False))+'</p><nav>'+links+'</nav><main>'+''.join(cards)+'</main>')
    (out / 'README.txt').write_text('PACKAGE 8 NATIVE NARRATIVE EVIDENCE\n\nOpen index.html for all unique pages at 240x320, or contact-sheets/ for review.\nframes.jsonl maps every rendered state/page to its image with exact positioned labels.\nqueries.jsonl records narrative and actual encounter text. summary.json contains counts.\nsource-sha256.json identifies the rendered source snapshot. run.log has exact build commands.\n\nDeduplication uses complete text, coordinates, size, and color. It never skips assertions or state coverage.\nDungeon fixtures position a valid hero beside real spawns; victories use begin/finish/dungeonResolve and production Save30 roundtrips. Campaign and letter fixtures use real offer/start/accept/resolve transitions.\nNo aggregate suite, remote build, commit, upload, or publication is performed.\n\nUnique pages by group:\n'+'\n'.join(f'{k}: {v}' for k,v in sorted(groups.items()))+'\n')
    print(f"Indexed {len(unique)} native PNGs and {len(sheets)} contact sheets: {out / 'index.html'}")

if __name__ == '__main__':
    main()
