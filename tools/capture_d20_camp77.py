"""Capture native production D20 and map-tent previews, including a real rolling GIF."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw
ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--output", type=Path, default=Path("build/d20-camp77/previews"))
args = parser.parse_args()
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=True)
files = sorted((ROOT/"firmware/RPG_POKET_2").glob("*.h"))
files += [ROOT/"tests/d20_camp77_capture.cpp"]
def hashes(): return {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
before = hashes()
exe = out/"d20_camp77_capture"
subprocess.run(["g++", "-std=c++17", "-O0", "-g0", "-UNDEBUG", str(ROOT/"tests/d20_camp77_capture.cpp"), "-o", str(exe)], cwd=ROOT, check=True)
subprocess.run([str(exe), str(out)], cwd=ROOT, check=True)
assert hashes() == before, "Renderer changed during capture; rerun after source is frozen"
frames = sorted(out.glob("*.ppm"))
for path in frames:
    im = Image.open(path)
    assert im.size == (240, 320)
    im.save(path.with_suffix(".png"))
rolling = [Image.open(path) for path in sorted(out.glob("roll-*.ppm"))]
assert len(rolling) == 16
rolling[0].save(out/"d20-rolling.gif", save_all=True, append_images=rolling[1:], duration=[120]*15+[1600], loop=0, disposal=2)
sheet = Image.new("RGB", (5*112,4*128), "#07131e")
labels = ImageDraw.Draw(sheet)
for result in range(1,21):
    x,y=((result-1)%5)*112,((result-1)//5)*128
    im=Image.open(out/f"face-{result}.png").crop((66,85,174,196))
    sheet.paste(im,(x+2,y+1))
    labels.text((x+39,y+113),str(result),fill="#e6c174")
sheet.save(out/"d20-all-faces.png")
art=ROOT/"cartao/RPGPOKET/artes.pak"
report=dict(native_dimensions=[240,320], frames=len(frames), host_only=True,
            source_hashes=before, art_sha256=hashlib.sha256(art.read_bytes()).hexdigest(),
            animation_ms=1800, result_hold_ms=1600, png_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.glob("*.png")})
(out/"capture-report.json").write_text(json.dumps(report,indent=2)+"\n")
print(f"PASS: {len(frames)} production native frames, all20faces and real16-frame rolling GIF at {out}")
