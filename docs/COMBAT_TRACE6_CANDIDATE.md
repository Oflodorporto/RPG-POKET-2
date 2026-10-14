# Package 6 combat dice candidate

Local-only candidate based on ad8acd8b60f8c4c8bf961f87f8300445ad9d6be5. No commit, push, release, OTA publication, balancing change or save-format change.

## Behavior

- The real resolver optionally emits a fixed 248-byte RAM receipt. Game, Save29, RNG state and save record layout are unchanged.
- Capture covers the weapon engine's actual 1d3−1 variation, d100 critical/dodge tests, aggregated Extra Attack, technique scaling, equipment/attribute/proficiency/forge modifiers, rage/sacred/brutal/smite bonuses, spell dice and each ray's separate hit test, enemy save/DC, one-per-spell Evocation INT, half damage, guard/resistance, HP caps, potion and healing caps, and drain MP.
- Potential (“Bruto”), post-mitigation damage (“final”), and actual HP loss are distinct. The presentation does not roll or sample RNG.
- Real dice progressively reveal during the existing short action animation. Pular only dismisses presentation. Dados opens paged receipts for the last hero and last enemy actions; those remain available after the animation and on normal, campaign, event and dungeon-boss outcomes.
- Viewing details suspends the pending enemy dispatcher. Already-resolved damage is already committed and saved; the screen explicitly says it is consulting an applied result. Closing restores the remaining timer. No roll, save, item use, reward or turn is replayed.
- Skip controls consume repeated taps; closing adds a brief 180 ms input quarantine to prevent the Back tap becoming an action or outcome acknowledgement. Save-error screens cannot be bypassed.
- Raw dice are not persisted. Reload clears receipts and displays an honest saved summary; it never reconstructs random values. A missing actor tab is labelled as having no record.
- Existing Grimoire formulas/costs remain; active Evocation now explicitly displays INT once before halving. Weapons retain their real engine arithmetic, rather than implying a new d20 attack rule.

## Verification

Commands:

- python3 tools/test_combat_trace.py --seeds 64 --report build/combat-trace/differential-final.json
- python3 tools/test_combat_trace.py --seeds 16 --sanitizers --report build/combat-trace/differential-sanitized-final.json
- bash tools/test_combat_trace_ui.sh
- python3 tools/check_controller_snapshot.py

The differential runner freezes baseline headers from the exact commit and compares byte streams (not only hashes): all 172 Game bytes with ABI padding normalized, all 160 Save29 bytes and exact responses after every operation, for baseline/default/null/captured APIs. It exercises modern/legacy classes and levels, every action, invalid/rejected actions, powers, all enemy IDs/intents, guard/rage/turn effects and multi-turn outcomes.

Additional regression checks: title_controller, dungeon_controller, dungeon77_postvictory, dungeon77_layout_controller, daily_travel29_combat, save29, class_progression, class_powers, martial_powers, paladin_smite, champion_progression, arcane_progression, barbarian_progression, touch77_controller, and hero77_controller. Logs are under build/combat-trace.

Receipt arithmetic is independently reconstructed in combat_trace_semantics (147,579 receipts). Controller tests cover receipt/skip/tab/page/back spam, exact hardware admission logic, enemy timing, normal/boss lethal hits, all-miss rays, resource/reward persistence, save retry, reload and uint32 clock wrap. Render tests check bounded labels, controls not covering result headers, no state mutation, and actual 240×320 screenshots.

LeakSanitizer is unsupported under this executor's ptrace environment; ASan+UBSan run with leak detection disabled. No heap allocation was added to production receipt capture/rendering. Hardware display/touch and a firmware device build have not been performed here; these are native resolver/controller/render tests.

## Exact example

Real mage level-3 seed 1, Burning Hands: 4+2+4 = 10; enemy d20 16 + 2 = 18 against DC 13; successful save halves to 5; applied HP loss 5. Screenshot: build/screens/combat6/fire-real-4-2-4-save-half.png. This reproduces the fire half-damage example; it is not a fabricated animation sequence.

## Integration

Apply to the package-6 integration tree, preserving package-5 controller changes. Rules edits are restricted to optional trace outputs and resolver capture; procedural work owns Game/Dungeon/Persistence changes. Controller fixtures must be regenerated from the final .ino controller region after resolving any merge conflict, then tools/check_controller_snapshot.py rerun. Do not apply the old differential baseline comparison unmodified to a procedural rules change that intentionally modifies Game/Save; run it on this isolated trace candidate, and adapt integration invariants as needed.
