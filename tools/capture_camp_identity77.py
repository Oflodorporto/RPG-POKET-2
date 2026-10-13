"""Convert the production camp identity test captures to native PNGs and sheets."""
from pathlib import Path
import argparse
import hashlib
import json
from PIL import Image, ImageDraw

parser = argparse.ArgumentParser()
parser.add_argument("root", nargs="?", default="build/screens/camp-identity77")
parser.add_argument("--before", type=Path, help="optional pre-subtitle native captures")
args = parser.parse_args()
root = Path(args.root)
files = []
for source in sorted(root.glob("*.ppm")):
    image = Image.open(source)
    assert image.size == (240, 320), source
    target = source.with_suffix(".png")
    image.save(target)
    files.append(dict(file=target.name, width=240, height=320,
                      sha256=hashlib.sha256(target.read_bytes()).hexdigest()))
assert len(files) == 16, "Run build/tests/camp_identity77_render first"
labels = ["Humano / Mago: vermelho, azul", "Elfo / Paladino: roxo, branco",
          "Anao / Guerreiro: verde, amarelo", "Orc / Barbaro: amarelo, roxo"]
for status in ("ready", "fallback"):
    sheet = Image.new("RGB", (960, 680), "#121820")
    draw = ImageDraw.Draw(sheet)
    for identity, label in enumerate(labels):
        for row, page in enumerate(("setup", "rest")):
            image = Image.open(root / f"{page}-race{identity}-class{identity}-{status}.png")
            sheet.paste(image, (identity * 240, row * 340 + 20))
            draw.text((identity * 240 + 4, row * 340 + 3), label, fill="white")
    name = "camp-identity-contact.png" if status == "ready" else "camp-identity-fallback-contact.png"
    sheet.save(root / name)
comparisons = []
if args.before:
    for source in sorted(args.before.glob("*ready.ppm")):
        before = Image.open(source)
        after = Image.open(root / source.name)
        assert before.size == after.size == (240, 320)
        changes = 0
        for y in range(320):
            for x in range(240):
                if before.getpixel((x, y)) != after.getpixel((x, y)):
                    assert source.name.startswith("rest") and 14 <= x < 226 and 81 <= y < 98
                    changes += 1
        comparisons.append(dict(file=source.name, changed_pixels=changes,
                                every_change_within_subtitle_strip=True))
    assert len(comparisons) == 8
report = dict(native_previews=files, subtitle_only_comparison=comparisons,
              notes="Production render at 240x320 without rescaling. Existing idle pose. "
                    "Coarse fallback source omits human-paladin trousers and human-warrior shirt masks.")
(root / "preview-manifest.json").write_text(json.dumps(report, indent=2) + "\n")
print(f"PASS: {len(files)} native 240x320 PNGs and two unscaled contact sheets; "
      f"{len(comparisons)} optional before/after strip comparisons.")
