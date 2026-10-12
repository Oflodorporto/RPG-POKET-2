"""Verify prepared/downloaded 7.6 assets without changing or publishing them."""
from pathlib import Path
import hashlib
import json
import re
import struct
import sys
import zlib

root = Path(sys.argv[1] if len(sys.argv) == 2 else "release")
header = Path("firmware/RPG_POKET_2/ReleaseVersion.h").read_text()
version = re.search(r'FW_VERSION="([^"]+)"', header).group(1)
build = int(re.search(r'FW_BUILD=(\d+)u', header).group(1))
package = re.search(r'FW_PACKAGE="([^"]+)"', header).group(1)
raw = (root / "manifest.json").read_bytes()
m = json.loads(raw)
assert 0 < len(raw) <= 4096
assert version == "2026.10.12-fatigue76" and build == 2026101203
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
assert struct.unpack_from("<H", fw, 12)[0] == 9, "Expected ESP32-S3 image"
assert version.encode() + b"\0" in fw and b"fatigue76-dev" not in fw
art = (root / "artes.pak").read_bytes()
magic, size, crc, count = struct.unpack("<4sIII", art[:16])
assert magic == b"PKA1" and size == len(art) - 16 and zlib.crc32(art[16:]) == crc
assert count == m["art"]["assets"] == 488 and m["art"]["payload_crc32"] == f"{crc:08x}"
assert m["art"]["sha256"] == "bf8c7a085791805b3f9a72ab4f9f6877b9c46a0cb842e25f073029d053a524f8"
assert m["minimum_psram_bytes"] == size + 153600 == 7145536
assert len(art) + 153600 < 8 * 1024 * 1024
print(f"PASS: final identity; Save28/1..28; 488 unchanged arts; OTA margin {3145728-len(fw)}; fixed PSRAM 7145536")
print(raw.decode(), end="")
