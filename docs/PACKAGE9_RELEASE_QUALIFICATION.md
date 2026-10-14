# Approved clock visibility correction: integrated qualification

Candidate identity: `2026.10.14-clock77`, build `2026101403`, firmware generation 7.7. This is a narrowly scoped clock correction after the separately published story77 release. The candidate, its branch and any preflight host results do not by themselves prove publication or physical-device validation.

## Source scope

The two approved production edits are `ClockCycle7.h` and `ClockView.h`; `ReleaseVersion.h` changes packaging identity only. All story77 narrative, gameplay, inventory, PvP, pet, persistence, RNG and device integration sources are frozen unchanged. The approved production patch is SHA-256 `eac634fdb033f77a124b44f2d4c06dc7f5c4de099ac3d7928cac4e4478d3ac7c`. Full clock test/capture patch provenance is SHA-256 `5727bf959c51f257dcdc38db828f73a66bdbecc7946a6378d182d7ad20cc51a7`.

The new package9 inventory freezes the complete inherited source tree plus those explicit clock changes. Packaging exceptions are enumerated, never a directory exemption. Missing/extra tracked or untracked inputs, source symlinks, ignored compiler exports and any production file outside the inventory are rejected. Generated release binaries may only be regular untracked files at their four named paths. Historical package8 gates and fixtures remain unchanged.

## Required software gates

1. Verify the final public story77 binary, art and manifest, its successful release run, exact source/tag and subsequent README-only main commit. Pin the complete explore77 -> immersion77 -> story77 -> clock77 ancestry and all public byte hashes before the candidate is frozen.
2. Run the inherited 130-task aggregate with its expanded clock logic/render cases, full narrative/controller/render checks, real clock adapters, save migration/rollback, combat differential and all ordinary/sanitizer variants. CI must use ASan/UBSan and LeakSanitizer. Focused local preflight is explicitly not the full qualification.
3. Produce the actual-render 12-second normal-speed pulse GIF and morning/noon/afternoon/night comparison alongside inherited captures. The approved clock visual remains isolated from narrative rendering.
4. Compile the exact candidate SHA for ESP32-S3 with core 3.3.12, 16 MB QIO flash, 8 MB OPI PSRAM, 3 MB OTA partitions and EraseFlash=none. Build outside the sketch using `--build-path build/esp32-package9 --export-binaries=false`, copy only the named application binary and immediately rerun the complete source guard before packaging. No `--output-dir` or sketch-local build exemption is permitted.
5. Verify ESP32-S3 header, compiled identity, 3 MB limit, firmware SHA, the unchanged 666-asset pack and manifest consistency. Run negative verifier probes and real-byte OTA on ordinary and sanitizer builds.
6. Stage exact published client/header closures for touch77, scenes77, scale77, camp77, explore77, immersion77 and story77. Save28/touch77 must refuse the direct update. The three Save29 clients migrate lazily without load-time writes; all Save30 clients preserve canonical records across all three slots, home, legacy/generated floors, sequence/CRC-only ordinary save and exact historical read-back. Seed, run, geometry, masks, position, next RNG and epoch1 remain unchanged. This tests save-codec read-back, not permission to downgrade firmware.
7. Record the exact Git commit/tree, source inventory, task checkpoints, LSan mode, captures, compiled budget and artifact hashes. A separate authorized main push may publish only that exact successful qualification SHA. The publisher repeats the guards/aggregate/build/OTA gates before creating one unique tag.
8. Download the published triple independently, compare with publisher bytes, verify it again, rerun the OTA fixture against these public bytes, and confirm the latest manifest. Rebuilds from the same source can have different binary hashes, so candidate hashes must never be represented as public hashes. Update README only after publication succeeds.

## Immutable budget and compatibility

- Art: 666 assets, 7,470,912-byte payload / 7,470,928-byte file; CRC32 `9b4bcfc0`; SHA-256 `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`.
- Save30 writes, Save29/30 reads, reset epoch1: no new reset or save fields.
- Existing framebuffer 153,600 bytes and decode workspace 13,760 bytes; known static PSRAM bound 7,638,272 bytes, unchanged. This is not measured free heap.
- Clock art 77,312 bytes, unchanged. Existing 200 ms cadence and 144-row partial transfer are preserved. Backlight remains at its existing 10% idle setting.
- Host-only matched `-Os` observation: unchanged State/Game/MenuState, `.data`/`.bss` and scanline buffer; medallion helper stack estimate +32 bytes. GNU size aggregate text rises 128 bytes, comprising `.text` +88, `.rodata` +64 and `.eh_frame` -24. These figures are not ESP32 measurements. Target firmware size is recorded only after compilation.

## Physical validation remains distinct

The user approved the 12-second actual-render preview. Host render comparisons, software OTA adapters and an ESP32 link do not measure physical visibility at 10% backlight, battery use, frame rate, touch latency, free heap, USB flashing, real TLS/SD/OTA or power-loss behavior. No device flash is performed by this process. The release is for the owner's physical test after its software gates pass.
