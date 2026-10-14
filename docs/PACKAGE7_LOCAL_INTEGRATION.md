# Package7 local integration

Base commit: `3f9d3221b99e6b4ba55dc270a2a1d0a9155ea27b`.
Base tree: `34a32a69aa98ac7e8ba807a04ca065b438148dec`.
This is preparation only, with no configured remote and no publication authority.
The inherited release identity is historical and must not be used to build this
candidate. A distinct identity and independent qualification are still required.

## Approved inputs

SHA-256 digests of the exact local candidate patches:

- Ruins: `1ddf4c1030e976689e92ec6ff2b0934d42f87fe633232aa06ad687ae7d2dec83`
- Clock: `191625245e8127d1a8bd9196f4c0024802bbe2a0dff44e8bb86a0b323be57ce2`
- Camp, including approved error-line correction: `234b630dff98f801d29511cb6cebf3a27fc9cf592693bff6bbabdf3f3cb5bd2a`
- Fatigue: `8d886505c3b6839a2d697d7539757e8c2bb7fa4a88e8091371485f75cd9ce31a`

The camp correction was applied incrementally over its first frozen patch using
`801485c72e790dc698ceb97258c90b8d62c8752326ae6769c3eeba7cb5b13d7f`.

## Deliberate merges

The sketch's shared repaint predicate retains CampSetup in the normal 120 ms
cadence and the clock's separate 200 ms animation cadence. Clock partial paints
still update rows 40–183 only; entry, dirty/status/date/letter and non-buffered
paths are full-frame. An unset clock with a guild letter uses the candidate's
480 ms fallback cadence. The exact merged predicate is compiled in a new test.

ScenicView's Ruins redesign and home-only fatigue panel applied independently.
The exact production controller body equals the intentionally updated title and
dungeon snapshots. No other controller action, save codec, reset, PvP, RNG,
travel or camp rule changes were introduced by integration.

The only new gameplay rule is the approved once-per-offensive-PvE-action fatigue
deduction: zero through 70, minus one at 71–89, minus two at 90–100; applied after
normal damage/defense/save computation and before the target-HP cap. Positive
damage has minimum one, while zero and misses stay zero. Enemy actions and PvP
are unchanged. The trace records the actual deduction without new RNG calls.

## Local validation and resuming

Use `RPG_DETECT_LEAKS=0 python3 tools/test_package7_local.py --focused --run-id focused-final --jobs 2`
before freezing and running the aggregate. Then use the same command without
`--focused` and a distinct `--run-id aggregate-final`.

Each named target records its commands, log hash and outcome in
`build/package7-validation/<run-id>/checkpoints.json`. Re-running an interrupted
command with the same run ID skips only successful targets. The exact source
manifest, compiler, sanitizer mode and target plan must match. Source changes
require a new run ID. Native tests and ASan/UBSan remain enabled with assertions.
Leak detection is disabled only for this ptrace executor; CI must test it.

The aggregate keeps the full inherited native and sanitizer target sets, adds
the package7 modules, all package6 procedural/receipt integration, D20/camp,
Save29/30 migration/fault review, OTA reliability, unchanged combat differential,
and PvP/protocol tests. Historical publication/identity gates are retained but
are not used as package7 qualification gates. Successful test logs and captured
render evidence are separate build artifacts, not a claim of firmware approval.

## Resource scope

Ruins replaces its existing 77,312-byte flash background and adds 18,144 bytes of
read-only authored title/frame/icon data. Clock keeps its original raster and
adds small flash lookup tables plus the paint-path byte. Camp adds no raster
payload. The existing 666-asset SD pack and known PSRAM allocations stay fixed.
Game remains 184 bytes and Save30 remains 160 bytes, reset epoch 1. There are no
new rendering heap allocations. Actual ESP32 code/flash growth, free heap, FPS,
power, touch latency and hardware appearance remain unmeasured here.

Narrative/lore review is separate and contributes no code or text changes here.
