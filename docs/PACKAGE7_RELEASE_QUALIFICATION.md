# Package7 qualification, not publication

Candidate: `2026.10.14-immersion77`, build `2026101401`, package `7.7 - Imersao nas Ruinas`.

The feature integration was independently frozen at tree `e877f7c08a0f0333e398dfec08f37279825e2c2f` on package6 `3f9d3221b99e6b4ba55dc270a2a1d0a9155ea27b`. The tracked inventory pins all 871 files; the package7 scope checker pins the inventory digest and a bounded packaging-only exception list. Historical package5/package6 tools stay byte-identical.

## Scope

- Medieval Ruins background, title, NPC and two pages of four actions, preserving action dispatch.
- Personalized seated meal animation on setup/rest screens and the narrow camp error-line correction.
- Local-time sun/moon clock with partial repaint, no synthetic game time.
- PvE fatigue damage penalty once per action: −1 at 71–89 and −2 at 90–100, without new PvP rules.
- Lore audit is separate; no new narrative rewrite is included.

Save30 reads29/30 and epoch1 stay unchanged. No new slot reset. The approved 666-asset SD package remains exactly 7,470,928 bytes, SHA256 `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`. New UI atlas storage is 18,144 constant flash bytes; the background replaces an existing same-sized slot. No new PSRAM allocation. Known PSRAM use is 7,638,272 bytes, a static allocation bound rather than a measured free-heap result.

## Qualification branch

Only pushes to `feat/package7-release` run `.github/workflows/package7-check.yml`. It has `contents: read`, explicit Bash fail-fast/pipefail, and no publication action. A single aggregate entrypoint covers the complete inherited normal/sanitizer suites, all four modules and integration/cadence/real adapters: 127 checkpointed targets, plus package7 identity/manifest/gate negatives and historical OTA paths. CI enables ASan, UBSan and LSan (`RPG_DETECT_LEAKS=1`); a local run with leak checks disabled does not replace this gate.

The workflow compiles the exact source for ESP32-S3, core 3.3.12, 16 MiB flash, app3M_fat9M_16MB, OPI PSRAM, no full erase. It checks compiled identity, 3 MiB OTA fit, pack/catalog/PSRAM contract, and real compiled bytes through OTA host adapters. Actual production render captures for each feature and inherited scenes are uploaded with source/size/hash/log/checkpoint evidence.

OTA covers exact historical production header closures: touch77 must refuse; scenes77, scale77 and camp77 migrate Save29→30; explore77 preserves Save30→30. All three slots, epoch marker, seed/run, both floors, geometry, masks, positions and next RNG are compared. Loading and installation must not write/migrate saves. The next ordinary journal save is checked. Newly saved package7 Save30 records are then read back through the exact explore77 decoder/journal, across either or both journal sides, and an ordinary next Save30 write is checked without losing seed, masks or RNG. This save-codec compatibility does not authorize or bypass firmware OTA downgrade refusal. Real Save30 rollback to old Save29 codecs fails closed. Corrupt firmware/art and write faults cannot select a new boot image prematurely.

The local `--synthetic-fixture` mode is explicitly host-only. It never substitutes for the mandatory post-ESP32 real-byte checks. Only four generated release paths may be untracked regular non-symlink files. Committing any generated output or using a symlink is rejected, as is any other source/output change.

## Publication boundary

There is one clear release workflow, `.github/workflows/release.yml`, prepared for package7. It accepts only a main push and requires a successful completed push qualification from the same repository, `feat/package7-release`, `.github/workflows/package7-check.yml`, and exact SHA. No new repository variables or manual-only dispatch dependency. It reruns qualification and real-byte checks, rejects an existing tag, then verifies public downloads and latest against the produced bytes.

This prepared workflow does not authorize updating main, creating a tag/release, or publishing. Those actions require the user's separate approval. The initial candidate used package6's README-only follow-up `02a94ca192b8ef915a452527444f8160b90c26aa` as parent. The publication retry is a strict child of the qualified but unpublished `dcacdce1fe3e36f461a92bfc7378e54e23f96e26`, with both trees, direct ancestry, original README bytes and a seven-file packaging-only retry scope pinned. No main history is rewritten.

ESP32 builds use the dedicated `build/esp32-package7` build path with binary export explicitly disabled, then copy only the expected `.ino.bin` into the existing manifest input path. This prevents Arduino savehex hooks from exporting files into the firmware source directory. Both workflows enforce the unchanged strict source inventory after compilation; no generated-source exception is allowed. The compiler version is recorded in the build log.

No physical USB/flash, TLS, power-loss, PSRAM free-heap/FPS or touch-latency verification is claimed by host tests or a successful target build.
