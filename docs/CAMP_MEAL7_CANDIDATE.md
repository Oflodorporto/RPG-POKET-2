# Camp meal and live location label, package7 candidate

Baseline: public package6 commit `3f9d3221b99e6b4ba55dc270a2a1d0a9155ea27b`.
This local candidate is not published.

## Corrections

- The old camp renderer sampled standing idle frame 0 regardless of the ration
  option. The new code-native cutout rig uses the same 16 race/class sources and
  their existing shirt/trouser masks. Original head, face, armor, cloak, boots and
  class equipment are retained. Legs are folded over a freestanding oak stool;
  weapons/shields are parked beside the seated hero. The selected ration moves
  from the lap to the mouth, pauses for visible bites, and returns to the lap.
- Setup reads only the volatile selected option and positive ration stock. Rest
  reads the committed `campStage == 3 && campRation`. No food appears on the roll
  or during an ambush. Daily rest reads that day's already-paid ration. Rendering
  never charges food, heals, saves, or advances the RNG.
- CampSetup was absent from the periodic repaint list. One sketch condition adds
  it to the existing 120 ms cadence so the preview and fire actually animate.
- Setup errors now occupy the second benefit line, above the ration control,
  rather than covering the seated hero. Missing ration/kit, long errors and the
  unchanged save-failure screen have focused pixel and state-invariance tests.
- All three original camp panels bake the word “Ruinas” into the artwork. The old
  transparent runtime label was printed over it. A fully opaque dedicated plaque
  now clears that entire area before printing the live location. Four regions,
  fallback art, and long labels are covered. Long labels end in an ellipsis.

The background raster, art pack, layout of touch targets, dice, recovery,
fatigue, inventory, saved schema, reset epoch, and controller action handlers are
unchanged. New per-frame heap allocations: zero. New art payload/PSRAM: zero.

## Files and integration

- `CampHeroView.h`: seated identity rig and meal action
- `CampView.h`: selected/committed meal wiring and placement
- `PanelView.h`: clean dynamic location plaque
- `DailyTravelView.h`: use the same identity-preserving meal rig at nightly rest
- `RPG_POKET_2.ino`: only CampSetup periodic repaint eligibility
- Focused renderer/controller tests and native capture tooling

The sketch hunk touches the same long paint predicate as the clock worker;
merge both intentionally. Controller fixture snapshots remain unchanged because
this predicate lies outside their captured controller body.

## Validation

Run `RPG_DETECT_LEAKS=0 bash tools/test_camp_meal7.sh --isolated` in this worker checkout.
Omit `--isolated` in the combined package7 tree: its strict single-worker scope
guard intentionally rejects unrelated worker changes. ASan and
UBSan remain enabled. LeakSanitizer is unsupported under the executor's ptrace
profile; leaving leak detection on fails at sanitizer shutdown. Render-time heap
wrappers assert on `new`, `malloc`, `calloc`, or `realloc` independently of LSAN.

The renderer covers 8,192 seated hero/color rasters, exact original head pixels,
all 16 silhouettes, meal phases, both live clothing masks, all ordinary camp
contexts, food/kit choices, fallback art, long labels, render bounds, art canaries,
and immutable save bytes. Production scene comparisons ensure hero pixels remain
unobscured by fire, bedroll, labels, and controls. The real controller covers
selection/cancel, one-time ration debit, rejected early taps, post-ambush rest,
resume, the unchanged 4,500 ms timer, and no duplicate recovery. Inherited D20,
camp/loot, daily-travel and daily-render regressions run in both normal and
ASan/UBSan modes.

Native preview outputs are in `build/screens/camp-identity77/`. PNGs and GIFs come
from actual 240×320 production renderer output. Contact sheets tile native scenes;
the separately labeled 3× close-up uses nearest-neighbor enlargement for review.
The GIF shows the 2,880 ms cosmetic action cycle, not a changed rest timer.

Device-target build size, physical device frame rate, touch latency and field
hardware validation remain integration checks, not claims of this worker.
