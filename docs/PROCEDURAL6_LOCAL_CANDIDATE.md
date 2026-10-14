# Package 6: procedural Ruins, local candidate

Base: `ad8acd8b60f8c4c8bf961f87f8300445ad9d6be5`. This is a local, unpublished
candidate, with generator revision R5. It does not include the separate combat-roll UI patch. Firmware
version/build, release workflows, reset epoch, NVS namespace, artwork and
controls have not been changed. Historical publication gates remain specific
to their released Save29 builds and are not publication authorization for this
candidate.

## Delivered scope

Only **new paid Ruins runs** use generator version 1. Both 9×9 floors are
procedural. Island's three floors and letter-expedition 5×5 maps remain fixed;
those families are not represented as completed procedural work. Their reward
eligibility and free-entry behavior are unchanged. In particular, no Island
anti-reroll restriction, new resource, extra progression, free dungeon camp or
new defeat exemption was introduced.

Existing Ruins, Island and letter expeditions loaded from Save29 preserve their
fixed geometry, coordinates, floor, enemy/loot/trap masks, resources and combat
RNG for the whole current attempt. Saving or taking stairs does not convert
that attempt. The next paid Ruins entry starts a new generated run.

## Generator and difficulty envelope

- Separate local xorshift generation PRNG; gameplay/combat/loot `randomState` is
  not advanced by entry, geometry queries, rendering or generation.
- Fixed-version, allocation-free generation. A derived single-entry cache is
  keyed by the persisted seed; querying another hero's seed safely rebuilds it.
- Exactly 33 traversable cells on each floor, matching the original maps. Both
  floors use randomized room graphs, spanning trees, two extra corridors and
  constrained encounter/pickup placement.
- Main routes are 12–16 steps versus 12 originally. The entrance floor still
  permits avoiding the second skeleton and the spectre only via a 20–24-step
  route, versus 20 originally. The first skeleton remains compulsory. All three
  lower-floor regular enemies remain compulsory before the boss.
- The same seven enemy identities, eight pickup identities, crystal entry cost,
  mask indices, enemy stats, combat rewards, potion amounts, seal and chest
  reward code remain unchanged. Geometry and pickup positions may change; no
  extra reward source is added. This is an explicit bounded challenge envelope,
  not a claim of statistically identical play difficulty.
- First encounters vary from 2–6 steps from the entrance. New procedural entry
  and stair arrivals face a passable neighbor without moving or spending RNG;
  legacy saved headings and stair behavior remain exact.
- Exit and stair anchors remain at their established coordinates. The seal is
  on the unlocked first floor; all lower-floor ordinary pickups remain outside
  the locked boss chamber. The boss and chest cannot bypass that door.
- Entrance floor: at most 256 graph/placement attempts. Lower floor: at most 64
  graph attempts, each with at most 32 bounded encounter/pickup-placement
  attempts. Failure uses the proven original floor as a valid fallback; its
  optional lower-floor placement pass is also bounded to 32 attempts.
- Every ordinary pickup remains behind the first compulsory guard. All
  entrance-floor pickups stay reachable while avoiding both later enemies;
  their avoidance-route distances are capped at the fixed map's distance+4.
  Lower-floor gold/life still need only the first enemy; mana requires the first
  two but not the third. Ordinary pickups remain outside the locked chamber.
  The first-generation prototype's weaker pickup-access checks were replaced
  with these exact baseline prerequisites before choosing R5.
- A new entry selects a different two-floor topology from the previous Ruins
  seed, with at most 64 candidate seeds. Exhausting this bound returns an error
  before charging or changing the hero. Normal generation has a valid fallback;
  failed novelty selection cannot charge a crystal or replay a reward.
- The cache is 198 bytes on the host ABI; no heap allocation is used for maps.
  Board stack, latency, free heap and FPS have not been measured. Cold generation
  in 10,000-seed host runs was approximately 0.4–0.6ms; this is indicative only.
  Rendering and cell queries reuse the immutable seed-keyed cache and do not
  rebuild a map per query. Entry/slot changes can rebuild the single cache.

## Exploration-diversity evidence

A separate 10,000-seed diagnostic records relative exits, turns and encounter
identities along the first 3/6/8 steps of a shortest exit path. It removes absolute
coordinates and normalizes reflection, so rotated/mirrored drawings alone do
not inflate the count. This is a route-discovery proxy, not a human-playtest or
full perceptual metric. Compared with the initial local candidate:

| Measure | Entrance floor | Lower floor |
| --- | --- | --- |
|Distinct first 6-step signatures|5 → 270|48 → 252|
|Distinct first 8-step signatures|13 → 1,274|219 → 1,225|
|Identical consecutive first 8-step signatures|9.83% → 0.29%|1.38% → 0.26%|

The same run validated 20,000 floor topologies and all healing-access gates,
with two valid entrance fallbacks and no lower-floor fallback. Repeating this
diagnostic against the frozen R5 headers reproduced every signature count and
first-guard distribution. Whole-map
rotation was rejected as the principal solution because rotating the starting
heading too produces the same first-person decisions. Larger graph variation
and relocated, budget-constrained encounters produce the diversity here.

Generator 1 is still unpublished. R5 is the proposed final implementation of
that version. Superseded development-only Save30/version 1 fixtures are not map-
compatible with R5 and must not be used as migration fixtures. No earlier
prototype was built for a device, delivered or published, as confirmed during
package review. Published Save29 active runs remain fixed and exact.

## Save30 migration and rollback

The record remains 160 bytes in `pkt2_v29`; the two journal slots, lifecycle
markers, CRC position 156, reset epoch 1, epoch offset 144 and slot offset 152 are
unchanged. The eight bytes explicitly reserved by Save29 are assigned only
under format 30:

| Bytes | Save30 field |
| --- | --- |
|143|active generator version; 0 means fixed/no active procedural run|
|148–151|32-bit last/current Ruins seed|
|153–155|24-bit Ruins run sequence|

Save29's reserved-zero checks remain strict. Unknown format/generator versions
fail closed. Seed and sequence must either both be zero or both be nonzero;
active procedural version 1 is valid only inside a Ruins run. The sequence wraps
from 16,777,215 to 1; it is not strictly monotonic. The last seed/sequence survive
exit and defeat, while the active generator version clears.

Loading a Save29 record is read-only. The next ordinary save writes canonical
Save30 to the inactive journal side. Entry cost, new seed, generator version,
position and cleared masks are one CRC-protected atomic record. Interrupted
writes resume either the complete old checkpoint or complete new checkpoint;
acknowledgement loss uses the existing idempotent retry logic. No reset or
three-slot migration sweep is performed.

**Downgrade is data-preserving but not playable for migrated slots.** Actual
released Save29 firmware blocks a slot if either journal side contains Save30,
even if the other side is older valid Save29. It does not silently resume the
older side and does not reset the data. Reinstalling compatible Save30-capable
firmware is required to open that migrated slot. This impact must be disclosed
before any eventual release. The source remains local and unpublished.

The frozen Save1–28 helper codec now rejects all procedural metadata rather than
silently dropping it. Historical fixed-map fixtures explicitly request fixed
entry; separate tests exercise the real controller's default procedural entry.

## Verification

Run `tools/test_procedural6.sh` for the focused optimized and ASan/UBSan gates,
and `tools/test_native.sh` for the aggregate regression suite. The focused
suite includes 100,000 topology seeds (10,000 under sanitizers), full walking/
loot/stair/combat-result/reload/death flows, frozen Save29 geometry, format
validation, atomic journal faults, the actual controller's entry/retry/reboot,
menu/bag/idle pauses, camp exclusion and full 240×320 renderer snapshots.

Independent compatibility review uses the exact released Save29 headers and
actual production NVS adapter with SDK-style typed blobs, commits, short reads,
I/O failures, acknowledgement loss and power cuts. It separately checks all
three slots and preserves non-target keys, Wi-Fi and the completed reset epoch.

## Remaining before publication

- Integrate/review the separate combat-roll UI patch and the current published
  package5 base; rerun all relevant tests on that final source.
- Choose a new version/build and a Save30-aware release/manifest qualification
  gate. A compatible update must advertise reads 29/30 and writes 30.
- Build the ESP32 firmware and measure its actual size/partition margin. No
  Arduino/ESP32 toolchain is installed in this candidate's execution container.
- Physical-device validation: load each existing save, pause/restart in a run,
  finish a legacy run, enter two new runs, check masks/costs and defeat, and check
  frame performance. Native tests do not replace this verification.
- Procedural Island and letter-expedition families remain future work. A free
  Island procedural reroll design requires explicit reward-policy review before
  changing its current eligibility; this candidate makes no such decision.

## Extension points, not delivered generators

Keep generation pure and version-dispatched per family, with family selected
by the existing dungeon/event flags. Future implementations must retain version 1
for every active Ruins save rather than reinterpret its seed. Stable object
indices remain the persisted mask contract; a new layout must move objects,
not change what an existing mask bit means.

Island requires its three-floor lever/stair ordering, second sealed passage,
eight enemy identities, six pickups and three persisted traps to remain valid.
Its zero-cost entry and existing repeat-run rewards require a separate policy
review before introducing topology-based rerolls. No cooldown, reward lockout
or entrance charge has been added here.

Letter expeditions require tier-dependent one/two-floor maps, the existing
clear-all-enemies stair gate, final lever, event progress and abandonment/defeat
rules to commit with their seed. The existing event tier and accepted letter
must remain stable over reload. Their generator is not implemented here.
