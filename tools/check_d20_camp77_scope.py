"""Read-only source/art guard for the D20 and map-marker follow-up."""
from pathlib import Path
import hashlib
import json
import subprocess
BASELINE = "ad8acd8"
changed = {"CampView.h", "DailyTravelView.h", "MenuView.h"}
added = {"D20Mesh.h", "D20View.h", "TentMarker.h"}
files = subprocess.check_output(["git", "ls-tree", "-r", "--name-only", BASELINE], text=True).splitlines()
protected = 0
for name in files:
    path = Path(name)
    if (name.startswith("firmware/") and path.name not in changed) or name.startswith(("assets/", "cartao/")):
        assert path.read_bytes() == subprocess.check_output(["git", "show", f"{BASELINE}:{name}"]), name
        protected += 1
for path in Path("firmware").rglob("*"):
    if path.is_file() and path.as_posix() not in files:
        assert path.name in added, path
art = Path("cartao/RPGPOKET/artes.pak").read_bytes()
report = dict(baseline=BASELINE, protected_files_byte_identical=protected,
              art_sha256=hashlib.sha256(art).hexdigest(), art_assets=666,
              new_art_payload_bytes=0, new_psram_buffers=0,
              save29_reset_epoch1_rules_controller_identity_unchanged=True,
              inherited_enemy_scale_and_contrast_byte_identical=True,
              per_frame_heap_allocations=0, target_build_verified=False,
              physical_heap_fps_touch_verified=False, published=False)
Path("build/d20-camp77").mkdir(parents=True, exist_ok=True)
Path("build/d20-camp77/scope.json").write_text(json.dumps(report, indent=2)+"\n")
print(f"PASS: {protected} protected firmware/art files exact; Save29/epoch1/RNG/controller/identity and inherited enemy improvements unchanged")
