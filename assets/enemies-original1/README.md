# Original enemies: compact PKA1 integration source

This directory retains the exact indexed exports and RGB565 reference outputs
from `RPG_POKET_29_Inimigos_Autorais.zip`. It does not regenerate artwork. The
original source archive, high-resolution masters, earlier generation attempts
and preview animations remain untouched outside this repository. Their original
filenames, checksums, prompts and authorship/provenance remain available here.

## Rebuild and validate

From the repository root, with Python 3.10+ and no Python dependencies:

```sh
python tools/prepare_enemy_art.py
python tools/prepare_enemy_art.py --check
python tools/unpack_art.py
python tests/enemy_asset_catalog.py
```

The first command regenerates both catalog headers and
`integration_manifest.json`. The second validates them without rewriting. The
third reconstructs `cartao/RPGPOKET/artes.pak` from the unchanged original archive
chain and the compact replacement. The test suite additionally uses a C++17
compiler with AddressSanitizer/UndefinedBehaviorSanitizer for a real
`makeFallback()` execution with allocation canaries. LeakSanitizer is disabled
in that test because it cannot run in the traced cloud executor; address and
undefined-behavior checks stay enabled.

To materialize the self-contained native codec fixtures:

```sh
python tools/prepare_enemy_art.py --check --extract-reference build/enemy_art_source
```

The destination is a source-package root. Indexed, palette, reference RGB565LE
and single-frame PNG files are under `exports/enemies/NN_slug/` and
`exports/effects/family/`. The original roster and effect manifests are copied;
per-enemy `manifest.json` records are derived from the complete original roster.
`enemies.tsv` lists all 29 IDs/slugs, anchors, family, palette ID and four ordered
pose IDs. `effects.tsv` lists all seven families, palette ID and eight frame IDs.
The Python helper `prepare_enemy_art.extract_reference(directory)` provides the
same operation. No reference extraction is needed for a normal package rebuild.

Transition-test fixtures can be emitted without checking out an old branch:

```sh
python tools/unpack_art.py --baseline-only --output build/old-artes.pak
python tools/unpack_art.py --output build/new-artes.pak
```

The baseline is the 488-entry catalog at
`33c0d5d75d0e96c45f593a5c7bb2e56eb0dc5080`. `baseline/catalog.json`
contains the exact original `AssetMeta` records, archive hashes and original
catalog hash. `baseline/fallback.bin.gz` holds only the old fallback bytes. The
old large C++ header is not duplicated. Rebuilds never interpret the regenerated
catalog as the baseline and fail if any retained original archive changes.

## Preserved provenance and compact source

- `provenance.json`: full original ZIP filename/SHA-256, compact export bundle
  hash/member checksums, and hashes of retained source files
- `source/`: byte-for-byte original contracts, inventories, budget, validation,
  checksums, roster/effect manifests, generation prompts and technical export
  scripts
- `exports.tar.gz`: deterministic tar/gzip containing 552 exact export files:
  172 indexed planes, 36 palettes, 172 reference RGB565LE frames and 172
  single-frame PNGs; gzip/tar timestamps and ownership are normalized
- `integration_manifest.json`: deterministic package identity, budget, every
  final asset's content hash, source mapping and all 172 reference frame hashes

`source/` is a frozen historical delivery. Statements such as
`runtime_integrated: false` describe the original handoff, not the integration
branch. Neither source handoff nor this catalog test claims hardware validation.
The original artist provenance states built-in image generation from original
text briefs for bodies and original procedural pixel effects for the shared
contact VFX. It does not assert legal exclusivity or clearance.

The exact compact archive can also be reproduced from the original extracted
source directory (or the native-test reference extraction):

```sh
python tools/prepare_enemy_art.py --repack-source /path/to/RPG_POKET_Inimigos_Autorais
```

Every input and the resulting normalized gzip bytes must match the frozen
checksums; a differing result is rejected rather than silently replacing the
source. Adding `--check` verifies reproduction without rewriting the archive.

The original archive is identified by SHA-256 in `provenance.json`; obtain that
same retained delivery to reproduce masters/previews using its original export
scripts and requirements. Normal firmware/package rebuilding uses only this
repository and the Python standard library. No private storage IDs or URLs are
required or published.

## Slot ownership and format

At the archived baseline, the reused slots were exclusively enemy bodies:

- 92–122: Goblin, Wolf, Skeleton and Guardian, defined in `SpriteArt.h`
- 401–416: Javali, Espectro, Saqueador and Sentinela, `RegionalArt.h`
- 459–463: five island enemy bodies, `IslandArt.h`
- 480–487: four world enemies' two poses, `WorldEnemyArt.h`

A baseline reference scan confirmed consumers in arena, dungeon, enemy-info and
bestiary rendering; no non-enemy consumer of these slots was found. The automated
catalog test rejects surviving direct RGB565 casts into the repurposed planes
and the legacy regional/world body accessors. Hero effects (123–146), island
textures/props (464–479), NPC story portraits and all other non-enemy entries
remain at their original asset IDs with byte-identical data and fallbacks.

Flattened body index = `enemy_id * 4 + pose`, ordered idle, anticipation, attack,
hurt. The first 60 frames reuse the ranges above in ascending order. Remaining
bodies occupy 488–543, FX 544–599, body palettes 600–628, FX palettes 629–635.
Effect order is slash, bite, charge, claw, magic, pulse, crush.

PKA1 and `AssetMeta` are unchanged. Body planes are stored as 40×86 bytes,
representing 80×86 pixels. FX planes are 16×32 bytes, representing 32×32 pixels.
Every palette is 16×1 RGB565LE words. Indices are high nibble first; zero maps to
transparent `0xF81F`. The generated `EnemyArtCatalog.h` provides typed metadata
and compile-time assertions for all mappings, bounds and non-overlap.

All compact planes use one zero-byte fallback with `tw=th=elem=1`. Palettes use
their complete real 32-byte palette as fallback. `makeFallback()` retains its
original loops and storage bounds. Missing art therefore yields transparent
body/effect planes, never a raw RGB565 interpretation or invented placeholder.

## Deterministic identity and budget

- Old payload: 6,991,936 bytes, CRC32 `53dcc3a5`, 488 entries
- Retired enemy body payload: 759,360 bytes / 60 entries
- Compact replacement: 428,864 bytes / 208 entries
- New payload: 6,661,440 bytes, CRC32 `0b84d93f`, 636 entries
- New complete PKA1: 6,661,456 bytes
- New complete SHA-256: `5416f6d507601bc131751e89106332f291f8d1d09642faa264a9b61215dd43f1`
- All 428 non-enemy payloads/fallbacks are byte-identical
- All 172 frame decodes match supplied RGB565 references pixel-for-pixel
- All 116 body pose PNG hashes are distinct
- Payload + 153,600-byte framebuffer + 13,760-byte decoder = 6,828,800 bytes
- Theoretical headroom within 8 MiB = 1,559,808 bytes

The PSRAM figures exclude other allocations and are not device measurements.
Frame time, largest available allocation, Wi-Fi/update load and touch latency
still require the hardware acceptance checks documented by the integration.

## Paired loader transition gate

```sh
python tools/test_enemy_transition.py
python tools/test_enemy_transition.py --sanitizers
```

The runner reconstructs the old package from retained archives and generates the
new package, then compiles `tests/enemy_art_transition.cpp` twice. Both builds use
byte-identical copies of the actual production `AssetStore.h`, `DeviceSettings.h`
and their dependency headers. Only `AssetCatalog.h` changes: the first is
reconstituted from archived 488-entry metadata/fallback bytes, the second is the
new generated catalog. There is no hand-written imitation of the loader logic.
The SD/network/preferences interfaces are the repository's existing test fakes.

There are 29 executed scenario gates per catalog, 58 in total:

- Own real package loads with byte-exact payload; the other real package is
  rejected before payload reads
- Each header identity field is rejected independently; short header and
  physically truncated file are rejected
- First, middle and last payload chunk short reads restore full fallback
- Three independent payload corruptions fail CRC and restore fallback; in the
  new catalog these target a body, a shared FX plane and a body palette
- No card, no file, generic own/wrong pack and only the other versioned filename
- Own versioned filename wins when both versions are present
- Corrupt own versioned pack recovers from a valid own generic installation pack
  without premature cleanup; corrupt/wrong combinations remain fallback
- Successful versioned boot deletes other CRC-named packs and the generic pack
  while preserving save bytes, partial downloads and unrelated files
- Null art allocation returns the existing distinct `Memory` status without SD
  access; it does not imply menu/update recovery after main art allocation fails

The same cleanup assertions run in both directions. Thus an old firmware boot
can remove a staged new art file, and a new firmware boot can remove old art.
The gate does not claim full-art rollback retention. CRC-paired selection and
safe fallback are the tested behavior.

All ordinary scenario checks include allocation canaries and unchanged Save28
encoding; simulated SD save bytes are also preserved. Short-read coverage is
representative first/middle/last payload chunks, not exhaustive fault injection
at every one of the roughly 1,600 chunk boundaries. This keeps the paired tests
bounded while exercising initialization, partial-copy restoration and the last
short chunk with the real CRC/loader loops. `--sanitizers` adds ASan/UBSan;
`RPG_DETECT_LEAKS=1` enables LeakSanitizer where the host supports it (default 0).
These tests do not replace OTA installer, interrupted boot, device memory or
latency qualification.
