# Coopfix77 qualification contract

Candidate: `2026.10.15-coopfix77`, build `2026101501`, tag `v2026.10.15-coopfix77`.
Qualification branch: `feat/coopfix77-release`.
Parent and promotion lease: `d767538669137ca08a2dd53f3805e8ba1be006b8`, tree `e2963cbd9236ea3c2990884a8e4f088544beb613`.

## Status and limits

These files prepare a candidate; their existence is not passing release evidence.
Publication requires complete exact-source qualification, independent audit and
successful main promotion. Native radio/touch fakes do not establish the physical
cause of intermittent disconnects or prove a two-board fix. RF diagnosis and a
physical two-board retest remain separate. No new graphics or animation feature
is bundled into this narrowly scoped correction.

## Source and identity

`check_coopfix77_scope.py` freezes every old package12/SHOP/PvP gate and inventory,
all current documentation/media, and every unchanged source at the exact parent.
The reviewed delta is limited to input coalescing, admission protocol v2 with a
host-authoritative 30-second decision window, countdown presentation and terminal
return controls, their regression tests and the new release identity. Technical
admission timeout remains 12 seconds; the established link timeout is unchanged.
The two historical session/radio fixtures retain their assertions with the
reviewed protocol-count/decision-budget/pending-peer expectations updated for v2.
Both peers must install this admission protocol version. Campaign, SHOP, PvP5,
Save30/160, readers29+30, epoch1 and ration10 remain unchanged.

The new inventory must be regenerated only after final source review. Its digest
is pinned in the new gate. A candidate commit must have exactly the approved
parent; local preparation may run against that parent before the single candidate
commit exists. The README/state remain accurately on public coop77 until verified
publication and a separately reviewed documentation update.

## Aggregate and target gates

`tools/test_coopfix77_local.py` imports the frozen package12 task generator and
executes its original runner. It appends one group with both touch and admission
regressions, each normal plus ASan/UBSan/LSan. Exact inherited commands, preparation,
checkpoint log hashes, source/compiler/environment binding, and fixture ordering
are preserved. The unchanged scheduler runs every coop-prefixed group serially after the inherited parallel pool, so the appended invitation checker runs after the inherited renderer; an offline scheduler probe enforces this ordering. There are 153 groups including preparation. Evidence intentionally
remains under `build/package12-validation/ci-SHA`. Focused runs and LSan suppression
are forbidden. The old runner and all 152 inherited groups remain unchanged.

`tests/coopfix77_qualification_config.py` re-executes the frozen aggregate probes,
then rejects any new dropped/changed/reordered command, skipped workflow step,
compiler override, permission widening, lost artifact or altered publication gate.
The new target evidence retains every prior object, ODR, updater linkage, memory,
image-envelope and compiler-property check. Target builds use core3.3.12 and
esp-x32/2601, historical ESP32-S3 FQBN/partition/options, default ON without a
feature define and forced OFF with only `-DRPG_ENABLE_COOP_TRAINING=0`.
Build directories are external; binary export is false and output-dir is banned.

The default-ON image must fit the 3,145,728-byte OTA partition. ON/OFF growth caps
remain 262,144 image bytes and 16,384 RAM bytes, with all existing static object
budgets, link checks and estimated PSRAM bound7,638,272. Actual measured sizes must
be recorded from the new target builds; previous release sizes are not substituted.
All 58 existing cooperative frames still execute normally and under sanitizers,
with pixel and label parity and no-allocation assertions. Sixteen additional nested invitation frames cover countdown30/10/1/0 on both sides, initial connection, synchronization, expiry, refusal, busy and technical timeout; the new hotfix group verifies exact pixel/label parity and creates previews. Existing campaign,
clock, SHOP, narrative and PvP capture counts/budgets remain mandatory.

## Real-byte OTA lineage

The new manifest/verifier/OTA harness preserves all ten historical clients and
adds actual public coop77 commit `da68148210b3155b72e41deb08a46ab854d09a0e`, tree
`6c003c6df520658297b832f6abc6e61645defd8c`, build2026101406. Its public firmware is
2,954,432 bytes, SHA256 `fed875fd458fb1c23d7b0dca4a525e35314ad50a743547c4ffc3989e31e9fab3`;
manifest SHA256 `dd97852974419c5f08e33652367dbc2e9c89e522e3bba2805cc6d4fe8e8d0324`.
The historical split installer translation unit and current installer are both
compiled and tested. Synthetic fixture probes are explicitly distinguished from
actual release bytes. Normal and sanitizer runs preserve all manifest, readback,
rollback/failure, three-slot, Save29→30, Save30→30 and old-reader ration boundaries.

## Publication and evidence

The new qualification workflow is branch/repository-bound, read-only, and does
not persist checkout credentials. The existing publisher switches only to the
new gate/identity/evidence and retains contents:write plus actions:read. It repeats
full tests and builds at the exact qualified SHA, requires the new tag absent,
checks the exact workflow/branch/repository/event/SHA success, then freshly reads
main immediately before creating the release. This main-check/create sequence is
a preflight, not an atomic lock. Stop on any lease/source/identity drift.

Only firmware.bin, artes.pak and manifest.json become release assets. Uploaded
qualification evidence adds text/JSON/logs for the new native regressions without
uploading host executables, target ELF/object files or forced-OFF firmware. Public
assets and latest are downloaded, compared byte-for-byte, and passed through the
same verifier and normal/sanitizer real-byte OTA checks after publication.

## Integration commands

After final source/inventory freeze, run `bash tools/test_coopfix77.sh` on a host
supporting strict LeakSanitizer. Run the new read-only workflow for target
compilation and final-byte qualification; local configuration probes alone are
not a substitute. Preserve the source tree between qualification and promotion.
