"""Verify the non-publishing 7.7 candidate and the unchanged 7.6 OTA contract."""
from pathlib import Path
import hashlib
import json
import re
import struct
import zlib

header = Path("firmware/RPG_POKET_2/ReleaseVersion.h").read_text()
version = re.search(r'FW_VERSION="([^"]+)"', header).group(1)
build = int(re.search(r'FW_BUILD=(\d+)u', header).group(1))
package = re.search(r'FW_PACKAGE="([^"]+)"', header).group(1)
assert version == "2026.10.12-visual77-dev" and build == 2026101204
assert build > 2026101203 and len(version) < 40
assert re.fullmatch(r"[0-9a-z.-]+", version)
base_bytes = 2780464
binary = Path("build/RPG_POKET_2.ino.bin").read_bytes()
art = Path("cartao/RPGPOKET/artes.pak").read_bytes()
raw_manifest = Path("release/manifest.json").read_bytes()
assert 0 < len(raw_manifest) <= 4096
manifest = json.loads(raw_manifest)
assert manifest["version"] == version and manifest["build"] == build
assert manifest["package_name"] == package and manifest["schema"] == 2
assert manifest["board"] == "Waveshare-29667"
assert manifest["partition"] == "app3M_fat9M_16MB"
assert manifest["save_format"] == 7 and manifest["writes_save_format"] == 28
assert manifest["reads_save_formats"] == list(range(1, 29))
assert manifest["installation"] == "confirmed-device-ota"
assert 24 <= len(binary) <= 3145728 and binary[0] == 0xE9
assert struct.unpack_from("<H", binary, 12)[0] == 9, "Expected ESP32-S3 image"
assert version.encode() + b"\0" in binary
assert len(binary) - base_bytes <= 65536, "Exceeds first-slice +64 KiB code budget"
base_url = f"https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v{version}/"
for key, name, content in (("firmware", "firmware.bin", binary), ("art", "artes.pak", art)):
    assert Path("release", name).read_bytes() == content
    assert manifest[key]["url"] == base_url + name
    assert manifest[key]["bytes"] == len(content)
    assert manifest[key]["sha256"] == hashlib.sha256(content).hexdigest()
magic, size, crc, count = struct.unpack("<4sIII", art[:16])
assert magic == b"PKA1" and size == len(art)-16 == 6991936
assert crc == zlib.crc32(art[16:]) and count == manifest["art"]["assets"] == 488
assert manifest["art"]["payload_crc32"] == f"{crc:08x}"
assert manifest["minimum_psram_bytes"] == size + 153600 == 7145536
assert hashlib.sha256(art).hexdigest() == "bf8c7a085791805b3f9a72ab4f9f6877b9c46a0cb842e25f073029d053a524f8"
report = {"candidate_version": version, "candidate_build": build,
          "baseline_source": "26a43145a8cd7e45e8716b1f1a34b358c41d47f3",
          "baseline_binary_bytes": base_bytes, "candidate_binary_bytes": len(binary),
          "growth_bytes": len(binary)-base_bytes, "max_growth_bytes": 65536,
          "ota_margin_bytes": 3145728-len(binary), "art_sha256": hashlib.sha256(art).hexdigest(),
          "art_changed": False, "additional_image_payload_bytes": 0,
          "physical_validation": False, "published": False}
Path("build/visual77-budget.json").write_text(json.dumps(report, indent=2)+"\n")
print("PASS: 7.7 partial-dev identity, manifest/hash and 7.6 OTA/Save28 contract; " + json.dumps(report))
