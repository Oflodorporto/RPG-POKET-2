# Ruins hub redesign: package 7 local candidate

Base: `3f9d3221b99e6b4ba55dc270a2a1d0a9155ea27b` (qualified package 6 source).
This work is local preparation. It does not change the release identity, publish,
push, reset slots, or modify the Save30 codec.

## Visual design

The new built-in image-generation output is a continuous ruined medieval city:
collapsed houses, ivy, towers, a broken gate and warm torchlight. The existing
blue/gold art is its reference. The generated city contains no UI or gameplay
text. Original ornate Ruins title lettering, gold/blue button edges and authored
icons are reused in a compact flash atlas; labels and progress remain live.
Iria uses her actual in-game portrait. The inventory backpack gets a small
RGB565 contrast lift, confined to its 24×24 footprint.

Two pages contain exactly four equal 112×44 action targets. The always-visible
objective remains a separate shortcut. The city gate is fully visible across
the 240×64 window at y112–175; no controls or labels interrupt that window in
the default state. Transient feedback/contract progress can occupy y157–172.

- Page 1: Conversar (all city NPCs), Explorar, Cripta, Guardiao
- Page 2: Acampar, Bolsa, Loja, Mapa
- Bottom: full-width page control with page number and destination label
- Header: original authored title, live player level and gold shown once

The guardian still requires three wins. The existing crystal purchase costs
300 gold; dungeon confirmation consumes exactly one crystal. Opening either
screen is read-only. The code does not invent a new level lock. All combat,
exploration, camping, store, inventory, objective and return rules are unchanged.

## Files and reproduction

- `assets/ruins7/ruins-city-generated.png.gz`: full generated source (1086×1448)
- `assets/ruins7/prompt.txt`: full built-in generation prompt and source reference
- `assets/ruins7/provenance.json`: source hashes, memory budget and asset provenance
- `ScenicArt.h`: the same existing 240×320 indexed Ruins slot, replaced in place
- `Ruins7Art.h`: original authored title/frame/icon crops, not generated labels
- `ScenicView.h`: shared hit rectangles, page mapping and renderer
- Sketch: only Ruins objective hit target and paged input dispatch change
- Controller snapshot fixtures and old bag tests follow the intended new layout

From the repository root:

    python3 tools/unpack_art.py
    python3 tools/prepare_ruins7_art.py
    python3 tools/prepare_ruins7_ui.py
    python3 tools/check_ruins7_scope.py
    bash tools/test_ruins7.sh

`prepare_scenic_ui.py` also reapplies the new Ruins background after rebuilding
the three untouched original scenic panels. The UI preparation uses the existing
NPC art pack and original reference archives, so unpack the art first.

`check_ruins7_scope.py` is an isolated-module scope guard against the exact base.
After combining independently authorized package7 rule changes, run an integrated
scope review instead of treating this exact-base guard as an integration gate.
The older package5/package6 release gates deliberately do not qualify package7.

## Verification

Final local focused pass: 13 optimized suites and 5 ASan/UBSan suites, with leak
detection disabled in this container. The final renderer/controller cases cover:

- 960 actual native framebuffer states per render pass: both pages, all four
  world periods/classes, blocked/ready/defeated guardian, max quantities and
  messages; same UI with SD art ready or missing and the PSRAM art pointer null
- 529 actual controller scenarios per pass; 153,600 exhaustive pixel hit checks
- 876 bag/equipment/potion scenarios, three slots/four classes, held input,
  pending outcomes and failed-save retry, preserving return to Ruins page 2
- 96 package6 integrated procedural-entry/combat receipt/reload scenarios
- 16 real UI renders per allocation test with no malloc/calloc/realloc calls
- Existing title, dungeon, touch, Carvalho, post-victory, layout, save/reset tests
- Exact controller snapshot parity; source scope and unchanged art-pack hash
- Actual 240×320 captures and exact nearest-neighbor 3× review images

The full combined-package aggregate, ESP32 cross-compile/OTA binary budget,
physical LCD/touch testing, device heap/FPS and publication are separate gates.
No `arduino-cli` is available in this local executor. This module is not itself
an authorized or qualified release.

## Memory and transfer budget

- New city replaces the existing 77,312-byte flash background slot
- Reused authored UI adds exactly 18,144 bytes of read-only flash data
- No new heap allocation or PSRAM art allocation; existing SD pack remains 666
  assets and byte-identical (7,470,928 bytes including header)
- SD SHA-256: `14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`
- Actual final firmware growth must be measured after the combined ESP32 build;
  the flash-data figure is not a binary-size estimate

## Review artifacts

`build/ruins7/ruins7-two-pages-medieval-render.png` is the approved real game
preview, not the generated source image or a mockup. The two 240×320 framebuffer
panels are enlarged exactly 3× with a 16px gutter. Day/night native PNGs and
blocked/ready/defeated guardian captures are produced by `capture_ruins7.py`.
The earlier flat-panel preview is superseded and must not be used.
