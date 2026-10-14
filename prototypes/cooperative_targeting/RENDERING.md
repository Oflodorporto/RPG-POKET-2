# Local touch-flow rendering

The renderer reads `output/coop-ui-targeting/ui-trace.jsonl` and reuses the original, CRC-verified game artwork, race masks, background, frame, and firmware bitmap font. It does not import the old renderer, change firmware, execute game actions, or modify snapshots.

Run from the workspace root:

```sh
python coop-ui-targeting-work/prototypes/cooperative_targeting/render_touch_flow.py
python coop-ui-targeting-work/prototypes/cooperative_targeting/test_render_touch_flow.py
```

## Outputs

- `target-native.png` and `boss-native.png`: 320 × 240 native pixels.
- `target-2x.png` and `boss-2x.png`: exact nearest-neighbor 640 × 480 expansions.
- `coop-touch-targeting-2x.png`: labelled comparison sheet; the boss is explicitly a separate scenario.
- `coop-touch-flow-local.gif`: 13 sequential UI snapshots, Attack → enemy 2 → pending → confirmation, then experimental Focus → cancel. The separate boss fixture is excluded. Presentation pacing is illustrative, 14.4 seconds total, and is not measured latency or device FPS.
- `ui-state-goldens.png`: all 14 trace snapshots, including the separately labelled boss fixture.
- `render-checks.json`: bounds and exhaustive per-pixel hit-rectangle checks.
- `text-bounds-goldens.json`: exact native text placements and region containment.
- `render-manifest.json`: source, font, artwork, trace, and output hashes.

The native UI uses the original 5 × 8 bitmap glyphs at one native pixel per glyph pixel, with six-pixel advances. The surrounding explanatory sheets use DejaVu Sans. No new artwork is generated.

## Geometry and state handling

The action buttons, sprites, arena, stats, and header use the agreed landscape layout. Outlines are drawn at the exact half-open C++ hit rectangles. HP captions sit just inside those rectangles so the border never cuts text. Neither hitboxes nor sprite boxes move between modes.

The trace's `eligible_slots` is authoritative for target outlines. Dead, unused, or non-enemy slots are rejected. No dead sprites are shown. Pending UI keeps the unchanged confirmed client snapshot and disables controls. An illustrative touch marker comes only from the trace's `click` or coordinate-valued `pressed` field. Each button marker is shown in its real down state, before release changes the menu, so Habilidade and Cancelar never point at a blank or replaced button. HP, MP, turn order, local player, revision, and event values are taken from the snapshot.

For the five displayed modes, button labels follow the local controller: action menu; experimental Focus submenu; HP/MP item submenu; selected action and cancel; all-disabled pending. The current demonstration includes no item action. Renderer-only tests exercise the item submenu and a zero-MP Focus state without adding invented events to the delivered trace.

Validation checks all 76,800 native pixels separately for action, three-enemy, and boss geometry (230,400 classifications). The additional QA script compares all rectangles with `CoopTouchUi.h`, verifies exact target-outline pixels, unchanged pending view and cancel state hashes, exact 2× expansions, manifest hashes, and decoded GIF dimensions/frame count/duration.

Scope: a local prototype only. No installation, production update, radio transport, hardware touch-controller test, or hardware memory/frame-rate claim.
