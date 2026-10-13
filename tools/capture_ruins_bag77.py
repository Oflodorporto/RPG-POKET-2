"""Convert native 240x320 renders to review PNGs without changing the pixels."""
from pathlib import Path
from PIL import Image
root = Path("build/ruins-bag77")
for path in sorted(root.glob("ruins-bag-*.ppm")):
    image = Image.open(path)
    assert image.size == (240, 320)
    image.save(path.with_suffix(".png"))
    image.resize((720, 960), Image.Resampling.NEAREST).save(path.with_name(path.stem + "-3x.png"))
before_path = root / "ruins-before.ppm"
if before_path.exists():
    before, after = Image.open(before_path), Image.open(root / "ruins-bag-day.ppm")
    changed = []
    for y in range(320):
        for x in range(240):
            if before.getpixel((x, y)) != after.getpixel((x, y)):
                assert 10 <= x < 98 and 148 <= y < 192, (x, y)
                changed.append((x, y))
    assert changed
    print(f"PASS: {len(changed)} changed pixels, all inside Bolsa; original hub outside it exact")
print("PASS: native240x320 + exact3x PNG captures")
