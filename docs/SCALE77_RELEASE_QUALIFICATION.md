# scale77 qualification and publication boundary

Identity: `2026.10.13-scale77`, build `2026101307`, package `7.7 - Escala e contraste dos inimigos`.
This is a presentation-only follow-up in 7.7, not a 7.8 release. This document records the required gates, not a claim that CI, publication, or physical validation has passed.

## Frozen compatibility contract

- Save format remains reads `[29]`, writes `29`; reset epoch remains `1`.
- A device on published scenes77 can update by Wi-Fi. Its completed epoch1 marker must prevent any repeated reset, and its three Save29 records must survive unchanged.
- The pre-Save29 boundary is unchanged: touch77 or older requires one USB installation with `EraseFlash=none`, without a full-chip erase. No Save1..28 import or OTA compatibility is invented.
- The 666-entry pack is byte-identical to scenes77: payload 7,470,912 bytes, total 7,470,928 bytes, CRC32 `9b4bcfc0`, SHA256 `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`. There are no new art assets.
- Known PSRAM allocation is still 7,638,272 bytes, a static budget and not measured free heap.

The existing manifest installation/reset fields describe the generation boundary. `usb_required_before_save_format=29` does not demand another USB installation from scenes77. `reset_game_slots_once=[1,2,3]` with the same `save_reset_epoch=1` does not authorize a new reset.

## Unmodified historical clients

The host release suite pins both source commits and Git trees:

- Published touch77: commit `91c64a364a7c1c697b9cd91b7e72027fa03f2dee`, tree `1099a6cb0713f22ed805a8ca28f3978f77734727`.
- Published scenes77: commit `dd626414b62bf6f88638ffe5d542196230c51061`, tree `d1d9972abe0e792bd2adbf51f39cd720d354a074`.

`tools/test_scale77_release.py` stages each complete production-header include closure without rewriting it. It proves touch77 rejects the truthful manifest, then feeds the actual scale77 manifest and supplied binary/art bytes to the published scenes77 installer. Existing and missing art, SHA256 corruption, OTA write failure, missing Save29 membership, write-format downgrade, older build, and mismatched same-build catalog cases remain covered. Current-scale77 art repair and a clearly labeled future-build protocol fixture retain the existing regression coverage.

The scenes77 executable writes three Save29 records and its completed reset marker. The current-scale77 executable reads those exact bytes and round-trips the records identically. The complete persistence/reset include closure is additionally pinned byte-for-byte to scenes77. The production reset barrier is run with the completed marker; any write, erase, or slot lookup fails the test. Installer sentinels cover preservation of the records, marker, Wi-Fi and previous art. These host checks do not claim physical NVS or flash validation; the existing persistence/reset fault suites still run in the full suite.

## Qualification gates

1. Freeze geometry and run `bash tools/test_scale77.sh`, retaining the full native regression and ASan/UBSan suites.
2. Review native-resolution before/after captures for all 29 enemies in day/night, reported Vigia HP35 and Espectro HP42 cases, Guardiao, all four classes, and valid near/far dungeon views including Arconte, Thalvor and Eco do Veu. Skipped impossible viewpoints must be reported rather than fabricated.
3. After local review/release-owner visual approval, push only the qualification branch when authorized. `scale77-check.yml` runs only on `feat/scale77-release` with read-only repository permissions. It runs the full suite, compiles with ESP32 core 3.3.12 and the existing precise FQBN, packages files, verifies them, and runs the three historical/current host adapters normally and with ASan/UBSan.
4. CI rerenders the pinned scenes77 source and candidate with the same capture fixture/art. It retains the exact firmware/art/manifest plus 240x320 frames, contact/comparison sheets, source/art/fixture hashes, and skipped viewpoints. CI requires all relevant old scene and new comparison families.
5. After successful scenes77 home-device feedback and release-owner technical/visual approval, the release owner may separately authorize advancing main to the exact qualified source SHA. Until then, do not update main, create the version tag, publish a Release, or change latest.

Useful local development checks:

- `python3 tests/scale77_release_verifier.py --synthetic`
- `python3 tools/test_scale77_release.py --synthetic-fixture`
- `python3 tools/test_scale77_release.py --synthetic-fixture --sanitizers`

Those explicit modes use synthetic firmware with the real approved art. They do not qualify a compiled ESP32 binary and are never used by publication/qualification CI.

Real-artifact commands, after compilation:

- `python3 tools/release_manifest.py`
- `python3 tools/verify_scale77_release.py`
- `python3 tests/scale77_release_verifier.py`
- `python3 tools/test_scale77_release.py release`
- `python3 tools/test_scale77_release.py release --sanitizers`

The verifier rejects recycled scenes77 identity/build, development markers, mismatched source identity, wrong board/partition/chip, oversized firmware, any altered Save29/reset metadata, wrong URLs/hashes, unapproved art, and an incorrect PSRAM budget. Prior scenes77 verifiers and qualification records remain historical and unchanged.

## Exact-SHA publication

The existing main-push/manual-dispatch publication structure is retained. `release.yml` permits only main and the new scale77 identity. It requires a successful run of `scale77-check.yml` on `feat/scale77-release` for the exact `GITHUB_SHA`, reruns the full suite/build/release probes, refuses any existing version tag, and pins the new tag to the tested SHA. A new merge commit does not satisfy an earlier qualification; qualify the final source and fast-forward main only after the human gates above.

After publication it downloads all three immutable-tag files, checks byte equality and hashes against tested outputs, reruns the release/OTA checks, and waits for the public latest manifest to match. Failed or unrun public verification must remain reported as incomplete. Branch qualification does not publish or change latest.
