# Local PvP presentation candidate

This is a dependent, unpublished candidate on top of the separately frozen PvP reliability work. No branch, tag, release, remote state, production art pack, saves, economy, RNG, PvE, or touch geometry was changed by the presentation implementation.

## Changes

- Stable idle pose 0; no cyclic attack poses while waiting.
- Full remote class/race/shirt/trousers identity from the versioned session profile.
- HP and MP values/bars and persistent guard status for both combatants.
- Visible mana errors, exact action MP prices, and availability styling using the existing Duel.canPlay contract.
- A dedicated 1,100 ms committed-action visual record. Local unacknowledged predictions do not become confirmed-hit animations. Duplicate events do not restart playback; a newer committed event replaces old cosmetic playback.
- Existing PvE hero poses and action effects, adapted by a PvP-only mirrored/clipped canvas. Damage and actual capped healing feedback. The effect ends at neutral pose without moving the HUD or controls.
- Explicit preparation, ACK wait, remote turn, and surrender-request status.

The four action hitboxes remain 102×40 at x=14/124 and y=220/270 on the original 240×320 canvas. Background pixels outside the HUD overlays remain the existing tracked background.

## Files

- firmware/RPG_POKET_2/ClubUi.h: bounded presentation event; existing resultUnconfirmed preserved.
- firmware/RPG_POKET_2/ClubBattleView.h: PvP-only rendering adapter and HUD.
- firmware/RPG_POKET_2/View.h: include and narrow ClubBattle dispatch; existing reliability result and legacy-version labels preserved.
- tests/pvp_presentation_render.cpp: real renderer + real ClubRadio callbacks + paired Duel simulation.
- tools/test_pvp_presentation.sh: optimized and ASan/UBSan focused runner.
- tools/capture_pvp_presentation.py: packages unmodified renderer pixels, with comparison labels only outside the 240×320 frames.

Protocol/appearance fields, confirmation revisions, actual healing, and ClubRadio event/cadence wiring are part of the combined local candidate.

## Evidence

See screens/render-results.txt, run.log, sanitizer-run.log, and capture-manifest.json. The tracked pack is loaded directly and checked against production ART_BYTES and ART_CRC. Input SHA-256: 14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56.

previews/duas-perspectivas.png compares the same two players. previews/feedback-e-confirmacao.png shows mana rejection, persistent guard, and a still-unacknowledged command. previews/matriz-classes-racas.png shows all class/race pairs. Twelve class/action GIFs show both perspectives; each source frame is rendered by firmware code with nominal 60 ms timing. They are not hand-drawn previews or measured hardware FPS.

## Limits

- Terminal settlement and persistence remain immediate. A lethal hit may transition directly to the result screen without playing its final action animation. There is no artificial persistence delay or result hold.
- A new committed action may replace an older animation; presentation never queues or blocks protocol progress. The existing predicted resource numbers may already be visible while the HUD explicitly says Confirmando acao, but a confirmed hit effect waits for acknowledgement/proof.
- Idle is deliberately static. Visual feedback uses existing gestures; there are no new art assets or new image-generation outputs.
- ClubVisual alone is 16 bytes on this 64-bit native host. Across the combined reliability/protocol/presentation candidate, measured host sizes change as follows: ClubUi 560→640 bytes (+80), Session 440→488 (+48), Duel 88→104 (+16), Packet 44→52 (+8), Peer 40→44 (+4); the eight-entry radio queue adds 64 bytes. Do not describe the 16-byte visual record as the whole candidate RAM increase. The temporary effect adapter uses a 656-byte host ViewState and a 184-byte host Game for appearance. No heap/framebuffer/PSRAM allocation is added by the renderer; wrapped malloc/calloc/realloc calls are asserted zero inside rendering. None of these host sizes measures ESP32 stack, heap, or free PSRAM.
- No full aggregate, ESP32 firmware build, two-board RF test, physical touch/legibility, on-device FPS/latency, stack high-water mark, heap/PSRAM, SD, or power-loss measurement was performed. ASan/UBSan uses detect_leaks=0 because this sandbox's LeakSanitizer/ptrace combination was already known incompatible; no LeakSanitizer pass is claimed.

Reproduce only the focused checks:

    tools/test_pvp_presentation.sh /path/to/validated/artes.pak /path/to/output
