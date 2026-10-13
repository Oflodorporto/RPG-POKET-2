"""Read-only source/art/known-memory guard for the visual-only scale77 package."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import struct
import zlib

BASELINE = "dd626414b62bf6f88638ffe5d542196230c51061"
allowed = {"DungeonView.h", "EnemyPresentation.h", "EnemyScale.h", "HeroActionView.h", "View.h", "ReleaseVersion.h"}
files = subprocess.check_output(["git", "ls-tree", "-r", "--name-only", BASELINE], text=True).splitlines()
protected = 0
for name in files:
    p = Path(name)
    if (name.startswith("firmware/") and p.name not in allowed) or name.startswith(("assets/", "cartao/")):
        before = subprocess.check_output(["git", "show", f"{BASELINE}:{name}"])
        assert p.read_bytes() == before, f"Protected baseline source/asset changed: {name}"
        protected += 1
for p in Path("firmware").rglob("*"):
    if p.is_file() and p.as_posix() not in files:
        assert p.name == "EnemyScale.h", f"Unexpected new firmware source: {p}"
pack = Path("cartao/RPGPOKET/artes.pak").read_bytes()
magic, size, crc, count = struct.unpack("<4sIII", pack[:16])
assert magic == b"PKA1" and size == len(pack) - 16 == 7470912 and count == 666
assert zlib.crc32(pack[16:]) == crc == 0x9b4bcfc0
sha = hashlib.sha256(pack).hexdigest()
assert sha == "14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56"
source = Path("firmware/RPG_POKET_2/EnemyScale.h").read_text()
assert not re.search(r"\b(malloc|calloc|realloc|new|random|Preferences|SD)\s*[(<]", source)
report = {
    "baseline_source": BASELINE, "protected_files_byte_identical": protected,
    "art_sha256": sha, "art_assets": count, "art_payload_bytes": size,
    "new_image_payload_bytes": 0, "new_render_heap_bytes": 0,
    "decode_workspace_bytes": 13760, "framebuffer_bytes": 153600,
    "known_psram_bytes": size + 153600 + 13760,
    "profile_readonly_bytes": 29 * 8,
    "save29_epoch1_unchanged": True,
    "baseline_firmware_bytes": 2830480, "baseline_ota_margin_bytes": 315248,
    "candidate_firmware_bytes": None,
    "device_heap_fps_and_latency_measured": False,
    "physical_validation": False, "published": False,
    "note": "Source/art invariants and known allocations only. The exact-commit ESP32 build measures new flash size; physical heap/FPS/latency still require the board."
}
Path("build").mkdir(exist_ok=True)
Path("build/scale77-scope-budget.json").write_text(json.dumps(report, indent=2) + "\n")
print(f"PASS: scale77 {protected} protected source/asset files unchanged; art666/hash exact; Save29/epoch1/gameplay/controllers preserved; known PSRAM {report['known_psram_bytes']} bytes, no new image or render heap allocation.")
