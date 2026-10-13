#!/usr/bin/env python3
"""Capture native enemy screenshots from any compatible checkout, without editing it.

Example (run from the candidate checkout):
  python3 tools/capture_scale77.py --source ../scenes77-publish \
      --art cartao/RPGPOKET/artes.pak --output build/scale77/before
  python3 tools/capture_scale77.py --source . \
      --art cartao/RPGPOKET/artes.pak --output build/scale77/after \
      --compare build/scale77/before

Each screen remains exactly 240x320. Contact sheets add caption margins; they never
resize the game pixels. These are host previews, not ESP32 or physical-LCD tests.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import textwrap

from PIL import Image, ImageChops, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
WIDTH, HEIGHT = 240, 320
CAPTION, GAP, HEADER = 42, 8, 42
FONT = ImageFont.truetype("DejaVuSans.ttf", 11)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git_value(source: Path, *arguments: str) -> str | None:
    result = subprocess.run(["git", "-C", str(source), *arguments],
                            capture_output=True, text=True, check=False)
    return result.stdout.strip() if result.returncode == 0 else None


def records(folder: Path) -> list[dict[str, str]]:
    with (folder / "captures.tsv").open(newline="") as stream:
        return list(csv.DictReader(stream, delimiter="\t"))


def screen(folder: Path, row: dict[str, str]) -> Image.Image:
    path = folder / row["file"]
    with Image.open(path) as image:
        if image.size != (WIDTH, HEIGHT):
            raise ValueError(f"Expected 240x320 game pixels: {path}: {image.size}")
        return image.convert("RGB")


def caption(draw: ImageDraw.ImageDraw, xy: tuple[int, int], text: str, width: int) -> None:
    x, y = xy
    # Captions live below the unmodified game frame, never on top of it.
    for index, line in enumerate(textwrap.wrap(text, width=width)[:3]):
        draw.text((x, y + index * 12), line, font=FONT, fill="#e4e6ed")


def display_caption(row: dict[str, str]) -> str:
    classes = ("Mago", "Cavaleiro", "Guerreiro", "Bárbaro")
    cities = ("Carvalho", "Ruínas", "Mares", "Aurora")
    parts = row["title"].split(" / ")
    if row["group"].startswith("dungeon-"):
        modes = ("Ruínas", "Ilha", "Expedição Carvalho", "Expedição Ruínas",
                 "Expedição Mares", "Expedição Aurora", "Expedição Carvalho")
        distance = "Perto (1 casa)" if row["distance"] == "1" else "Longe (2 casas)"
        floor = int(next(part[5:] for part in parts if part.startswith("floor"))) + 1
        return (f"{parts[1]} | {distance} | {modes[int(row['mode'])]} | "
                f"Andar {floor} | {classes[int(row['class'])]}")
    period = "Dia" if row["period"] == "day" else "Noite"
    return f"{parts[0]} | {cities[int(row['city'])]} | {period} | {classes[int(row['class'])]}"


def group_title(group: str) -> str:
    return {"outdoor-day": "Combate externo: dia", "outdoor-night": "Combate externo: noite",
            "dungeon-1tile": "Dungeon: inimigos perto (1 casa)",
            "dungeon-2tile": "Dungeon: inimigos longe (2 casas)",
            "reported-vigia-hp35": "Vigia Ossudo: HP 35",
            "reported-espectro-hp42": "Espectro: HP 42",
            "reported-guardiao": "Guardião: quatro classes",
            "dungeon-chefes": "Chefes: Arconte, Thalvor e Eco do Véu"}.get(group, group)


def sheets(folder: Path, rows: list[dict[str, str]], columns: int,
           compare: Path | None, label: str, compare_label: str) -> list[str]:
    destination = folder / ("comparisons" if compare else "sheets")
    destination.mkdir(exist_ok=True)
    reference = {row["file"]: row for row in records(compare)} if compare else {}
    groups = sorted({row["group"] for row in rows})
    groups.append("dungeon-chefes")
    result = []
    pairs = 2 if compare else 1
    tile_width = WIDTH * pairs + (GAP if compare else 0)
    tile_height = HEIGHT + CAPTION
    # At most four native rows per image: keep sheets practical to inspect.
    capacity = columns * 4
    for group in groups:
        if group == "dungeon-chefes":
            selected = sorted((row for row in rows if row["group"].startswith("dungeon-")
                               and int(row["enemy_id"]) in (8, 24, 28)),
                              key=lambda row: (int(row["enemy_id"]), int(row["distance"])))
            if len(selected) != 6:
                raise ValueError("Missing near/far views of Arconte, Thalvor or Eco do Véu")
        else:
            selected = [row for row in rows if row["group"] == group]
        for part, start in enumerate(range(0, len(selected), capacity), 1):
            page = selected[start:start + capacity]
            row_count = math.ceil(len(page) / columns)
            image = Image.new("RGB", (columns * (tile_width + GAP) + GAP,
                                      row_count * (tile_height + GAP) + HEADER + GAP), "#151923")
            draw = ImageDraw.Draw(image)
            title = (f"{group_title(group)} | {compare_label} / {label}" if compare
                     else f"{group_title(group)} | {label}")
            draw.text((GAP, 7), title, font=FONT, fill="white")
            draw.text((GAP, 23), "240x320 nativos, sem redimensionar. Em cada par: Antes à esquerda, Depois à direita."
                      if compare else "240x320 nativos, sem redimensionar. Prévia gerada pelo renderizador do jogo.",
                      font=FONT, fill="#b4bdce")
            for index, row in enumerate(page):
                x = GAP + index % columns * (tile_width + GAP)
                y = HEADER + index // columns * (tile_height + GAP)
                if compare:
                    if row["file"] not in reference:
                        raise ValueError(f"Missing baseline frame: {row['file']}")
                    old = reference[row["file"]]
                    if {k: v for k, v in row.items() if k != "title"} != {
                            k: v for k, v in old.items() if k != "title"}:
                        raise ValueError(f"Baseline/candidate fixture mismatch: {row['file']}")
                    image.paste(screen(compare, old), (x, y))
                    image.paste(screen(folder, row), (x + WIDTH + GAP, y))
                else:
                    image.paste(screen(folder, row), (x, y))
                caption(draw, (x, y + HEIGHT + 3), display_caption(row), 75 if compare else 37)
            path = destination / f"{group}-{part:02d}.png"
            image.save(path)
            result.append(str(path.relative_to(folder)))
    return result


def mobile_sheet(folder: Path, compare: Path) -> str:
    """Three honest encounter pairs, native pixels in a narrow phone-friendly sheet."""
    samples = (
        ("dungeon-original-spawn02-foe05-1tile.ppm", "Espectro: dungeon, perto (1 casa)"),
        ("dungeon-original-spawn04-foe09-1tile.ppm", "Vigia Ossudo: dungeon, perto (1 casa)"),
        ("reported-guardiao-day-class0.ppm", "Guardião: combate externo"),
    )
    width, header, row_height = 2 * WIDTH + 3 * GAP, 56, HEIGHT + 32 + GAP
    image = Image.new("RGB", (width, header + len(samples) * row_height), "#151923")
    draw = ImageDraw.Draw(image)
    title_font = ImageFont.truetype("DejaVuSans.ttf", 14)
    draw.text((GAP, 7), "Escala e contraste dos inimigos", font=title_font, fill="white")
    draw.text((GAP, 34), "Antes", font=title_font, fill="#b4bdce")
    draw.text((WIDTH + 2 * GAP, 34), "Depois", font=title_font, fill="white")
    for index, (filename, label) in enumerate(samples):
        y = header + index * row_height
        # Use the completed native PNGs; do not rescale or crop the game view.
        for x, root in ((GAP, compare), (WIDTH + 2 * GAP, folder)):
            path = root / "png" / Path(filename).with_suffix(".png")
            with Image.open(path) as frame:
                if frame.size != (WIDTH, HEIGHT):
                    raise ValueError(f"Non-native screenshot: {path}")
                image.paste(frame.convert("RGB"), (x, y))
        draw.text((GAP, y + HEIGHT + 7), label, font=title_font, fill="#e4e6ed")
    destination = folder / "comparisons/revisao-inimigos-mobile.png"
    destination.parent.mkdir(exist_ok=True)
    image.save(destination)
    return str(destination.relative_to(folder))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--source", type=Path, default=ROOT, help="Checkout whose production renderer is used")
    parser.add_argument("--art", type=Path, default=ROOT / "cartao/RPGPOKET/artes.pak",
                        help="Actual art pack (may be shared with the immutable baseline)")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--compare", type=Path, help="Prior capture folder for matching before/after sheets")
    parser.add_argument("--label", default="Depois")
    parser.add_argument("--compare-label", default="Antes")
    parser.add_argument("--columns", type=int, default=3, help="Native screenshot columns per single-source sheet")
    parser.add_argument("--sheets-only", action="store_true",
                        help="Regenerate captions/sheets from a completed capture without compiling or rendering")
    args = parser.parse_args()
    source, art, output = args.source.resolve(), args.art.resolve(), args.output.resolve()
    compare = args.compare.resolve() if args.compare else None
    render_header = source / "tests/render.cpp"
    if not render_header.is_file() or not art.is_file():
        parser.error("--source must contain tests/render.cpp and --art must be an existing pack")
    if args.columns < 1 or args.columns > 6:
        parser.error("--columns must be between 1 and 6")
    if compare and (compare == output or not (compare / "captures.tsv").is_file()):
        parser.error("--compare must be a different, completed capture directory")
    if args.sheets_only:
        report_path = output / "capture-report.json"
        if not report_path.is_file():
            parser.error("--sheets-only requires a completed capture-report.json")
        report = json.loads(report_path.read_text())
        rows = records(output)
        for row in rows:
            if sha256(output / row["file"]) != report["frame_sha256"][row["file"]]:
                raise ValueError(f"Capture pixels changed since provenance was recorded: {row['file']}")
        report["sheets"] = sheets(output, rows, args.columns, None, args.label, args.compare_label)
        report["comparisons"] = (sheets(output, rows, min(2, args.columns), compare,
                                       args.label, args.compare_label) if compare else [])
        if compare:
            report["comparisons"].append(mobile_sheet(output, compare))
        report_path.write_text(json.dumps(report, indent=2) + "\n")
        print(f"PASS: regenerated Unicode-caption sheets without compiling. Output: {output}")
        return
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    if not compiler or not shutil.which(compiler[0]):
        parser.error("Host C++ compiler unavailable")
    output.mkdir(parents=True, exist_ok=True)
    executable = output / "scale77_capture"
    fixture = ROOT / "tests/scale77_capture.cpp"
    source_paths = [render_header, *sorted((source / "firmware/RPG_POKET_2").glob("*.h")),
                    *sorted((source / "firmware/RPG_POKET_2").glob("*.ino"))]
    source_hashes = {str(path.relative_to(source)): sha256(path) for path in source_paths}
    fixture_hash = sha256(fixture)
    command = [*compiler, "-std=c++17", "-O0", "-g0", "-UNDEBUG",
               f'-DSCALE77_RENDER_HEADER="{render_header}"', str(fixture), "-o", str(executable)]
    print("Compile:", shlex.join(command), flush=True)
    subprocess.run(command, cwd=ROOT, check=True)
    subprocess.run([str(executable), str(output), str(art)], cwd=ROOT, check=True)
    if source_hashes != {str(path.relative_to(source)): sha256(path) for path in source_paths}:
        raise RuntimeError("Renderer source changed during capture; rerun after editing has stopped")
    if sha256(fixture) != fixture_hash:
        raise RuntimeError("Capture fixture changed during compilation; rerun this capture")
    rows = records(output)
    if not rows or len({row["file"] for row in rows}) != len(rows):
        raise ValueError("Empty or duplicate capture manifest")
    for period in ("day", "night"):
        ids = {int(row["enemy_id"]) for row in rows if row["group"] == f"outdoor-{period}"}
        if ids != set(range(29)):
            raise ValueError(f"Incomplete 29-enemy {period} coverage")
    # Individual screenshots are native PNGs; PPMs remain available as raw output.
    png_dir = output / "png"
    png_dir.mkdir(exist_ok=True)
    frame_hashes = {}
    for row in rows:
        path = png_dir / Path(row["file"]).with_suffix(".png")
        screen(output, row).save(path)
        frame_hashes[row["file"]] = sha256(output / row["file"])
    contact_sheets = sheets(output, rows, args.columns, None, args.label, args.compare_label)
    paired_sheets = sheets(output, rows, min(2, args.columns), compare,
                           args.label, args.compare_label) if compare else []
    if compare:
        paired_sheets.append(mobile_sheet(output, compare))
    report = dict(source=str(source), source_head=git_value(source, "rev-parse", "HEAD"),
                  source_status=git_value(source, "status", "--short"),
                  source_sha256=source_hashes,
                  capture_fixture_sha256=fixture_hash, art_path=str(art), art_sha256=sha256(art),
                  frames=len(rows), pixel_dimensions=[WIDTH, HEIGHT], host_only=True,
                  coverage={group: sum(row["group"] == group for row in rows)
                            for group in sorted({row["group"] for row in rows})},
                  compiler_command=command, comparison_source=str(compare) if compare else None,
                  sheets=contact_sheets, comparisons=paired_sheets, frame_sha256=frame_hashes)
    if compare:
        old_report_path = compare / "capture-report.json"
        if old_report_path.is_file():
            old_report = json.loads(old_report_path.read_text())
            if old_report["art_sha256"] != report["art_sha256"]:
                raise ValueError("Before/after art packs differ; comparison must preserve authored art")
            if old_report["capture_fixture_sha256"] != report["capture_fixture_sha256"]:
                raise ValueError("Before/after fixture changed; recapture the baseline with this fixture")
        report["changed_frames"] = 0
        for row in rows:
            difference = ImageChops.difference(screen(compare, row), screen(output, row))
            report["changed_frames"] += bool(difference.getbbox())
            top, bottom = (34, 172) if row["group"].startswith("dungeon-") else (86, 192)
            if (difference.crop((0, 0, WIDTH, top)).getbbox() is not None or
                    difference.crop((0, bottom, WIDTH, HEIGHT)).getbbox() is not None):
                raise ValueError(f"Fixed HUD pixels changed outside the combat band: {row['file']}")
        report["unchanged_hud_frames"] = len(rows)
    (output / "capture-report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"PASS: {len(rows)} native screenshots; {len(contact_sheets)} sheets; "
          f"{len(paired_sheets)} comparison sheets. Report: {output / 'capture-report.json'}")


if __name__ == "__main__":
    main()
