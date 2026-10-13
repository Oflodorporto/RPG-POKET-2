"""Combined local package-5 guard; the frozen D20-only checker stays untouched."""
from pathlib import Path
import hashlib
import json
import subprocess

BASELINE = "ad8acd8b60f8c4c8bf961f87f8300445ad9d6be5"
CANDIDATE = Path("build/d20-camp77/CANDIDATE5_HANDOFF.json")
# This local qualification must have the immutable candidate's 207-file inventory.
# Final release integration must retain it as an input or supply its own exact-SHA gate.
frozen = json.loads(CANDIDATE.read_text())
assert frozen["base_sha"] == BASELINE and len(frozen["changes"]) == 207
prior = {entry["path"] for entry in frozen["changes"]}
for entry in frozen["changes"]:
    assert hashlib.sha256(Path(entry["path"]).read_bytes()).hexdigest() == entry["sha256"], entry["path"]
changed = {
    "firmware/RPG_POKET_2/ScenicView.h", "firmware/RPG_POKET_2/RPG_POKET_2.ino",
    "tests/title_controller.cpp", "tests/dungeon_controller.cpp",
}
added = {
    "tests/ruins_bag77_controller.cpp", "tests/ruins_bag77_render.cpp",
    "tools/check_ruins_bag77_scope.py", "tools/test_ruins_bag77.sh",
    "tools/capture_ruins_bag77.py", "docs/RUINS_BAG_7_7_PREVIA.txt",
}
files = set(subprocess.check_output(["git", "ls-tree", "-r", "--name-only", BASELINE], text=True).splitlines())
protected = 0
for name in sorted(files - prior - changed):
    assert Path(name).read_bytes() == subprocess.check_output(["git", "show", f"{BASELINE}:{name}"]), name
    protected += 1
untracked = set(subprocess.check_output(["git", "ls-files", "--others", "--exclude-standard"], text=True).splitlines())
assert not (untracked - prior - added), sorted(untracked - prior - added)
# Generated art is ignored by git, so verify it separately against the frozen guard.
old_scope = json.loads(Path("build/d20-camp77/scope.json").read_text())
art_sha = hashlib.sha256(Path("cartao/RPGPOKET/artes.pak").read_bytes()).hexdigest()
assert art_sha == old_scope["art_sha256"]
report = dict(baseline=BASELINE, frozen_candidate_files_unchanged=207,
              other_baseline_files_byte_identical=protected,
              allowed_bag_modified=sorted(changed), allowed_bag_added=sorted(added),
              art_sha256=art_sha, save29_epoch1_rules_release_identity_unchanged=True,
              new_psram_bytes=0, published=False)
Path("build/ruins-bag77").mkdir(parents=True, exist_ok=True)
Path("build/ruins-bag77/scope.json").write_text(json.dumps(report, indent=2) + "\n")
print(f"PASS: 207 frozen candidate files + {protected} other baseline files exact; only explicit bag files allowed; Save29/epoch1/rules/art/identity unchanged")
