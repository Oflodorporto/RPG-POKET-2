# scenes77 qualification and publication

Identity: `2026.10.13-scenes77`, build `2026101306`, package `7.7 - Cenarios e viagem diaria`.

## Installation boundary

The first Save29 installation from touch77 or older requires USB with `EraseFlash=none`.
All three game slots undergo the approved one-time epoch1 reset. The reset excludes Wi-Fi settings and SD art. It must not become a per-build or per-boot reset. No Save1..28 import is claimed. `Save.h` remains a frozen historical fixture/shared packed-field helper; production uses the explicit Save29 persistence codec.

The published touch77 source is commit `91c64a364a7c1c697b9cd91b7e72027fa03f2dee`, tree `1099a6cb0713f22ed805a8ca28f3978f77734727`. The release test stages its complete include closure unchanged and proves it rejects the truthful Save29 manifest before installing anything. The new parser requires membership of the format the running firmware writes, currently29. Future OTA must read29; it need not claim1..28.

## Local and CI gates

1. Run `bash tools/test_scenes77.sh`. This includes the full native suite, ASan/UBSan, deterministic art checks, production persistence/reset/daily travel and renderer/controller regressions.
2. Compile using ESP32 core3.3.12 and the exact workflow FQBN. No upload or erase is performed.
3. Run `python3 tools/release_manifest.py`, `python3 tools/verify_scenes77_release.py`, and `python3 tests/scenes77_release_verifier.py`.
4. Run `python3 tools/test_scenes77_release.py release` and repeat with `--sanitizers`.
5. Inspect the rendered screens and the verified output size/memory estimates. Hardware validation remains separate.

`--synthetic-fixture` and verifier `--synthetic` are explicit development-only modes. They exercise parser/installer/verifier code against synthetic firmware and real approved art. Neither mode qualifies a compiled binary; publication workflows never pass these flags.

The real-SHA256 host installer tests cover the old client's refusal, current-version art repair, future29 OTA with existing or missing art, corrupted firmware/art, OTA write failure, missing29, write-format downgrade and same-build catalog mismatch. Save29 byte sentinels and previous art remain unchanged across OTA cases. These sentinels are not an NVS model; the persistence/reset suites cover NVS behavior independently.

The verifier pins the666-entry pack to SHA256 `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`, payload7,470,912 bytes, CRC32 `9b4bcfc0`, and the known7,638,272-byte PSRAM allocation estimate. It checks both downloadable hashes, URLs, release identity inside the ESP32-S3 image, OTA partition size, current save generation, reset metadata, and source catalog. Negative probes include a self-consistent old636-entry pack paired with new firmware.

## Exact-commit publication

`scenes77-check.yml` runs only on `feat/scenes77-release`, compiles and tests without publication, and retains the qualified files and screen artifacts. `release.yml` runs only on main, requires a successful scenes77 qualification for its exact `GITHUB_SHA`, reruns the full checks/build, refuses an existing tag, and pins the new tag to that source SHA. A merge commit with a different SHA does not satisfy the gate: qualify the final source commit and fast-forward main to it.

Publication is a separate authorized step. These scripts do not publish from a developer machine. The release owner must approve the final gate before any push/publication. After publishing, the workflow downloads firmware.bin, artes.pak and manifest.json from the immutable tag, compares all three byte-for-byte to tested outputs, reruns verification and OTA host probes, and polls the public latest manifest until it matches. A failed download/latest check must be reported as incomplete verification, not successful completion.

## Physical limits

No claim is made about actual USB transfer, NVS behavior on physical flash, TLS on the device, power-loss recovery on physical hardware, display/touch comfort, or measured runtime free heap. Full-chip erase is not an accepted fallback. The approved software reset is scoped to game slots; firmware release does not authorize deleting other device data.
