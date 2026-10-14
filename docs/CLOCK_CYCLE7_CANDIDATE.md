# Clock cycle7: local candidate, not published

Base: exact public commit `3f9d3221b99e6b4ba55dc270a2a1d0a9155ea27b`, tree `34a32a69aa98ac7e8ba807a04ca065b438148dec`.

## Behavior

- Retains the approved medieval clock artwork, hour/minute/second/date labels, touch return, guild letter route and physical idle backlight behavior.
- A small gilded medallion follows the existing elliptical rim once per local day: midnight below, 06:00 left, noon above and 18:00 right. It displays the moon during the game's night phase and the sun otherwise. A restrained 4.096-second breathing light never switches the medallion off.
- The original face gains a softly blended, textured sky, painted dawn/day glow above the digits, sparse slow-breathing night stars, and one faded 1.4-second shooting star per 97 seconds of uptime when it is night.
- The face reads the existing local `menu.clockTime` produced by `tickClock()`. It does not read UTC as local time, change the saved offset (default UTC−3), request location, synchronize a second clock, or change manual/offline clock rules.
- Phases use the existing `worldClock::period()` bands: night 20:00–04:59, dawn 05:00–06:59, day 07:00–17:59 and dusk 18:00–19:59. This is a game/civil-clock visualization, **not astronomical sunrise or sunset**. The world's day/night setting continues to govern world scenery/gameplay; an accurate idle clock still represents its displayed local time.
- The face uses the actual displayed local hour even while gameplay defers `menu.worldPeriod` updates. No gameplay period is mutated by rendering.
- Unset/invalid time keeps the original static face and existing `--:--`, date placeholder and time/Wi-Fi notice. There is no fictional moving sun or meteor. A valid manual/system clock keeps moving without Wi-Fi, subject to the existing clock's limitations.
- Animation runs only on the idle clock page. Leaving it pauses its rendering; returning immediately renders the current time without an animation catch-up loop. Midnight/year and `millis()` wrap are covered. The pulse/twinkle periods are powers of two to avoid a rollover discontinuity.

## Budget and ownership

Production changes are limited to `ClockCycle7.h`, `ClockView.h` and two narrow sketch areas: `paint()` and the periodic redraw predicate. There are no Save30, game-resource, turn, RNG, controller routing, clock-setting, release-identity or asset-pack edits.

The same 77,312-byte flash clock artwork, 666 external assets and existing frame buffer are retained. There is no new raster asset, heap allocation, PSRAM buffer, SD read or runtime trigonometry. New small lookup tables live in flash; a single `wasClock` byte distinguishes entry from pulse-only frames. The existing 480-byte stack scanline is reused.

On buffered devices, only pulse frames restore and transfer rows 40–183: 144 of 320 rows, or 69,120 pixel bytes instead of 153,600. The entire orbit spans y=42–180, and all sky effects/text remain inside the band. Entry, time/date/status updates, guild-letter presentation and other pages still redraw fully. Touch polling remains between 16-row SPI stripes. The direct-LCD fallback still performs complete frames. Clock animation is capped at a 200 ms interval; normal gameplay cadence is unchanged.

The previously estimated 7,638,272 bytes of known PSRAM allocation do not grow. Device free heap, actual FPS, power impact and ESP32 binary size remain unmeasured here; they require combined firmware qualification and hardware testing. No battery telemetry was present in this baseline's clock; none has been invented.

## Validation

Run `RPG_DETECT_LEAKS=0 bash tools/test_clock_cycle7.sh` in a ptrace-based host, or leave the environment variable unset on a normal host/CI for leak detection.

- Exact approved clock art source/export/reference hashes and controller snapshots
- All 86,400 local-second positions; cardinal positions and wrap continuity
- Valid/malformed/unset time, monotonic cadence wrap, bounded pulse/meteor and deterministic phase
- 3,072 full/partial differential frame comparisons across every hour, online/offline flags, automatic/manual flags and day-cycle flags
- Zero render heap allocations and byte-identical game/RNG/menu/view state; no art-RAM dependency
- Existing 1,445 clock-render cases: all art availability modes, fallback, text wells, letter routes and date/second rollover
- Optimized and ASan/UBSan builds for both new rendering/logic suites and the existing clock suite
- Actual `tickClock()` integration across all 27 configured offsets; manual/offline advance, midnight/year rollover over a `millis()` wrap, deferred world period and unset restart
- Actual sketch `paint()` extraction: exact full-versus-partial framebuffer equality, complete entry/status/date/letter redraws, touch checkpoints, return and direct-LCD fallback
- Unchanged actual idle controller: entry, repeated idle, title/game/settings return, guild letter, time wrap and unchanged save/game bytes

LeakSanitizer cannot run under this execution environment's ptrace; the successful local sanitizer configuration disables only leak detection. Explicit allocation wrappers remain enabled for render checks. Arduino CLI/core is not installed here, so the ESP32 compile and whole-package integration are deferred to combined qualification. No push, release or firmware flash has been performed.

## Captures

`tools/capture_clock_cycle7.py` packages frames produced by the actual C++ renderer, without simulating the interface:

- `Relogio-medieval-noite-velocidade-real.gif`: 10 seconds at real speed, 5 FPS, including the shooting star
- `Relogio-medieval-ciclo-24h-acelerado.gif`: explicitly accelerated 24-hour orbit in 19.2 seconds (4,500×)
- `clock-local-phases.png`: dawn/day/dusk/night contact sheet
- `clock-day-realtime.gif`: real-speed daytime pulse
- `clock-unset.png`, `clock-letter.png`: fallback and guild-message presentation

All captures are host render previews, not photographs or a claim of measured device performance.
