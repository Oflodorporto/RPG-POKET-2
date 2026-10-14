# Clock7 visibility: local review candidate

Date: 2026-10-14. Base: published commit `e775b5d1854174b45404ca0cd3305c1e24610861`.

Status: standalone local correction for review. No commit, push, tag, release, upload, ESP32 build, or physical-device flash. This has not been integrated into package/story 8; that combination requires its own review and qualification.

## What changed

The user confirmed that the orbital sun moves on the device, but its brightness/pulse was not perceptible. The old daytime sky also remained the same from 07:00 to 18:00. The earlier 24-hour accelerated preview did not demonstrate how visible the pulse would be at normal speed.

Only two production headers change:

- `ClockCycle7.h`: retain the 4.096-second power-of-two pulse and 200 ms cadence, ease its endpoints with integer smoothstep, and interpolate distinct morning/noon/afternoon sky colors using displayed local civil time.
- `ClockView.h`: breathe the gilded body between radii 6 and 9 pixels and its blended halo between radii 8 and 11 pixels; increase gold/moon luminance contrast while preserving a recognizable sun/crescent. Integer fixed-point geometry avoids trig, square root, heap and new artwork.

The original 24-hour orbit, local-time adapter, period labels, manual/offline clock behavior, night stars/meteor, idle/touch/letter routing and date/time text remain unchanged. There is no astronomical sunrise assumption or location lookup. Hardware idle backlight remains at the existing 10%; no power policy changes.

The complete effect remains within the current partial region, rows 40–183 (144 rows). The halo is clipped by its own circular radius, not by the transfer band. No larger framebuffer or transfer region is needed. Every frame restores the sky/art before drawing the current medallion, so an expanding/shrinking body leaves no trails.

## Visibility evidence

At the feedback time 08:33:13, independently rendered minimum/maximum pulse frames differ at:

- Published base: 156 pixels; mean absolute RGB channel change on changed pixels 24.50/255.
- Final candidate: 346 pixels; mean absolute RGB channel change on changed pixels 79.08/255.

These are host-render differences, not measured display luminance. Growth/shrinkage supplies a visual cue in addition to brightness. Visibility at the device's 10% backlight still needs the user's physical test.

## Verified previews

Generate with `RPG_DETECT_LEAKS=0 bash tools/test_clock_cycle7.sh`.

The main review files are under `build/clock-cycle7/visibility-preview/`:

- `Relogio-pulso-manha-velocidade-real-12s.gif`: genuine firmware renderer, 08:33:13 through 08:33:24, 60 captures at 200 ms, exactly 12,000 ms. Two identical adjacent images are coalesced by the GIF encoder: 59 stored frames, with timing retained. No time/pulse acceleration. The display is enlarged 2x with nearest-neighbor; the caption is outside its 240×320 content.
- `Relogio-luz-manha-meiodia-tarde-noite.png`: actual 240×320 renders at 08:33:13, 12:00:00, 17:00:00 and 23:00:00, at the same pulse phase.
- `Relogio-pulso-inicio-e-pico.png`: minimum/maximum pulse at the same displayed time, enlarged 2x.
- `capture-manifest.json`: SHA-256, dimensions, duration and size for each preview.

Existing accelerated 24-hour captures remain separately labeled as accelerated. They are not the principal visibility demonstration. No preview simulates or claims to reproduce physical 10% backlight.

## Focused validation on final production bytes

Command: `RPG_DETECT_LEAKS=0 bash tools/test_clock_cycle7.sh`.

- 86,400 local-second positions: cardinal points, midnight continuity, complete maximum-halo bounds, civil periods, malformed/unset clock, bounded deterministic pulse/meteor, millis wrap.
- Every second of the day: adjacent RGB565 sky channel steps are at most one quantization level, including palette-key boundaries; morning/noon/afternoon tones differ.
- 1,728 differential full/partial comparisons (3,456 renders), including pulse minima/maxima, all 24 hours, online/offline, automatic/manual and world-cycle flags.
- Zero render heap allocations; byte-identical game/RNG/menu/view state; no art-RAM dependency; existing ornamental boundary unchanged.
- 1,445 existing clock-render cases, including all five art states, independent text wells, no-clock fallback, letter routes, date/second rollover and bounds.
- Optimized plus ASan/UBSan builds for logic, new render and existing render suites. LeakSanitizer only was disabled for this ptrace environment; allocation wrappers remain active.
- Actual `tickClock()` with all 27 supported offsets, offline/manual advance, midnight/year rollover through millis wrap, deferred gameplay phase and unset restart.
- Actual sketch `paint()`: full entry/status/date/letter redraw, exact 144-row partial framebuffer equality, touch checkpoints and direct-LCD fallback.
- Actual idle/tap controller: title/game/settings return, guild letter, repeated idle, wrap and unchanged save/RNG state.
- Existing controller snapshots and approved clock-art hashes pass.
- Preview duration and manifest validated. `git diff --check` passes.

The host compiler still emits the existing `TitleView.h` inlining/`-Wstringop-overread` warning in the render harness. The same warning is reproduced on the untouched published base; it is outside these two production edits. The sanitizer tests pass. No ESP32 toolchain, device FPS, battery behavior, free-device-heap or combined release claim is made.

## Memory and immutable scope

The approved indexed clock artwork remains exactly 77,312 bytes. Its `ClockArtData.h` SHA-256 remains `c9e627cfe0bdf2e434e32d8db2d33366a20fb9185f630b03aa7e7db00e1a7e45`.

No changed raster assets, SD I/O, persistent fields, saves, game rules, gameplay RNG, `DeviceSettings.h`, sketch integration or release identifiers. Production changes are restricted to the two headers listed above.

A matched `-Os` host-only memory probe on base/candidate reports unchanged State 40 bytes, MenuState 888 bytes, Game 184 bytes, 480-byte stack scanline, and unchanged ELF `.data`/`.bss`. The candidate's GNU `size` aggregate text count is 128 bytes larger; the ELF `.text` section grows by 88 bytes, `.rodata` by 64, and `.eh_frame` shrinks by 24. Host stack estimates keep `drawClock` at 656 bytes and increase its medallion helper from 96 to 128 bytes. These are host compiler observations, not an ESP32 binary/stack measurement. Three extra static palette keys add 48 bytes of source-level constant data; no dynamic allocation is introduced.

Final production SHA-256:

- `ClockCycle7.h`: `60b20739ab48449ae49e6ba05b8b955fce5ae2a823685f0d4d136a2bbc46c616`
- `ClockView.h`: `e06e1e8edbd6d80cb1f5fb78a4f99bcf1d8c35bd7a5dab7b7713ef939f1dd772`

Review the normal-speed preview before considering publication. Any later package-8 integration must apply this small patch explicitly, resolve its own context, and rerun combined qualification rather than reusing this standalone result.
