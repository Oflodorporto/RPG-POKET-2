"""Verify the exact final dungeon/interface correction firmware/art pair, including downloaded copies."""
from pathlib import Path
import hashlib
import json
import re
import struct
import sys
import zlib

root = Path(sys.argv[1] if len(sys.argv) == 2 else "release")
assert len(sys.argv) <= 2
header = Path("firmware/RPG_POKET_2/ReleaseVersion.h").read_text()
version = re.search(r'FW_VERSION="([^"]+)"', header).group(1)
build = int(re.search(r'FW_BUILD=(\d+)u', header).group(1))
package = re.search(r'FW_PACKAGE="([^"]+)"', header).group(1)
assert version == "2026.10.13-dungeon77" and build == 2026101304
assert package == "7.7 - Dungeon e interface de combate" and len(version) < 40
raw = (root / "manifest.json").read_bytes()
assert 0 < len(raw) <= 4096
m = json.loads(raw)
assert m["version"] == version and m["build"] == build and m["package_name"] == package
assert m["schema"] == 2 and m["board"] == "Waveshare-29667"
assert m["partition"] == "app3M_fat9M_16MB"
assert m["save_format"] == 7 and m["writes_save_format"] == 28
assert m["reads_save_formats"] == list(range(1, 29))
assert m["installation"] == "confirmed-device-ota"
base = f"https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v{version}/"
for key, name in (("firmware", "firmware.bin"), ("art", "artes.pak")):
    data = (root / name).read_bytes()
    assert m[key]["url"] == base + name
    assert len(data) == m[key]["bytes"]
    assert hashlib.sha256(data).hexdigest() == m[key]["sha256"]
    print(f"PASS: {name} {len(data)} bytes SHA256 {m[key]['sha256']}")
fw = (root / "firmware.bin").read_bytes()
assert 24 <= len(fw) <= 3145728 and fw[0] == 0xE9
assert len(fw) - 2677248 <= 65536, "Exceeds the qualified +64 KiB growth budget"
assert struct.unpack_from("<H", fw, 12)[0] == 9, "Expected ESP32-S3 image"
assert version.encode() + b"\0" in fw
assert b"2026.10.13-dungeon77-dev" not in fw and b"visual77-combined-dev" not in fw and b"enemy77-dev" not in fw
art = (root / "artes.pak").read_bytes()
magic, size, crc, count = struct.unpack("<4sIII", art[:16])
assert magic == b"PKA1" and size == len(art) - 16 == 6661440
assert zlib.crc32(art[16:]) == crc == 0x0B84D93F
assert count == m["art"]["assets"] == 636
assert m["art"]["payload_crc32"] == f"{crc:08x}"
assert m["art"]["sha256"] == "5416f6d507601bc131751e89106332f291f8d1d09642faa264a9b61215dd43f1"
catalog = Path("firmware/RPG_POKET_2/AssetCatalog.h").read_text()
assert int(re.search(r'ART_BYTES=(\d+)', catalog).group(1)) == size
assert int(re.search(r'ART_CRC=0x([a-f0-9]+)', catalog).group(1), 16) == crc
assert int(re.search(r'ART_COUNT=(\d+)', catalog).group(1)) == count
workspace = int(re.search(r'WORKSPACE_BYTES=(\d+)', Path("firmware/RPG_POKET_2/EnemyArt.h").read_text()).group(1))
assert workspace == 13760
assert m["minimum_psram_bytes"] == size + 153600 + workspace == 6828800
assert m["minimum_psram_bytes"] < 8 * 1024 * 1024
print(f"PASS: final 7.7 identity; Save28/1..28; 636 paired assets; OTA margin {3145728-len(fw)}; known PSRAM 6828800 (static estimate, not free heap)")
print(raw.decode(), end="")
