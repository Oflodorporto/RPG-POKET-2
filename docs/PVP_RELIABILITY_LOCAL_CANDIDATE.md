# PvP reliability: local candidate, not released

Base: `9ca94294daf185ac80603e3ba409f0201eef72fb`, 2026-10-14. No remote branch, tag, publication, ESP32 build or broad CI run was made. This candidate precedes a separate presentation/appearance layer. No changes to Save30, epoch 1, persistent format, price/reward rules, campaign combat, classes, damage formulas or RNG.

## Corrected behavior

- A received terminal command, matching final MoveAck, or matching terminal receipt proves the terminal state. `terminalConfirmed` latches it. Cancel/Lost before or after the proof, including reordering across receive batches within the original command deadline, cannot turn that proved win into a loss. After closure only ACKs/receipts from the same admitted peer/session are accepted; no new commands or post-settlement outcome changes.
- An unconfirmed predicted lethal action never pays a win. A new, separately retried `Result` message carries revision, winner and a canonical state digest. It lets a terminal receiver confirm an accepted command even if every final MoveAck is lost. The digest is not authentication or an anti-cheat authority.
- Result receipts run every 400 ms for up to ten seconds, including on the result screen. Duplicate commands/receipts do not apply damage or pay/save a reward again. Durable settlement is at-most-once: reboot before the terminal win is saved may still recover the existing conservative loss. A not-yet-persisted victory is not guaranteed across power loss. The existing eleven-second result-radio grace period remains.
- Desistir works during the opponent's turn and while a prior local command awaits ACK. It sends a retried surrender intent, rather than predicting the outcome locally. The intent is rebased after an in-flight ordinary command. It ends only upon a matching ACK/receipt or the existing ten-second communication-failure path.
- Simultaneous surrender requests are ordered by the already agreed active turn: that player's request is accepted first. This prevents two independent local wins. The other player can therefore win after requesting surrender too; this is a command-ordering race, not an automatic draw/refund. Review this explicit policy before release.
- An opponent who remains connected but idle has no invented turn deadline. The voluntary surrender path removes the UI trap without adding an automatic time-limit win.
- An interrupted, unconfirmed result is labeled `NAO CONFIRMADO`, rather than represented as a proved combat defeat. The existing conservative accounting is preserved: before the first action, refund the entry; after combat starts, consume the entry and award no prize.

## Compatibility

The frozen reliability-only snapshot uses local protocol v5, 48-byte Packet, with one 32-bit terminal digest and a Result message. The dependent, still-unreleased appearance layer expands v5 to 52 bytes. Both boards must run the same final version. v4's 44-byte Hello may be read only to show `/ atualizar`; invite/admission/moves/results from old peers are rejected. No claim of mixed-version play is made. The 48-byte reliability-only intermediate is not a release target and must not be mixed with the final 52-byte candidate.

## Limits requiring an explicit decision before release

No two-party exchange can infer delivery across a complete persistent partition. If both ACK and receipt are lost, a locally predicted winner remains unconfirmed and receives no reward; the entry policy above remains conservative. Reconnect, durable result reconciliation, paid-entry verification, ranking authority and anti-cheat identity are not implemented. A durable reconciliation policy would require a separately authorized persistence design; this patch does not silently add one.

The simultaneous-surrender ordering and the `NAO CONFIRMADO` explanation need review. Automatic idle forfeits, refunds after combat on uncertainty, and rewards on a mere local prediction are intentionally absent.

## Focused verification

Run `bash tools/test_pvp_reliability.sh /tmp/rpg-pvp-reliability`.

The frozen v2 reliability snapshot passed seven optimized targets and six ASan/UBSan targets. The final reliability runner has eleven optimized and ten sanitizer targets, adding closure/deadline/wrap, natural terminal ordering, legacy terminal persistence and production Save30 journal regressions. These targets cover existing class/level combat, readiness, retries, lost commands/ACKs, implicit ACK, uint32 wrap, draw, 48 simulated admission-compensation power-cut boundaries plus ten terminal-settlement cuts in the legacy FatigueJournal/Save28 harness, SaveError queue freeze, recovery and single settlement; plus the independent audit regressions with corrected expected behavior. Those legacy persistence tests are not end-to-end qualification of the currently shipped PersistentJournal/Save30 backend; tests/pvp_reliability_save30.cpp separately runs the real ClubRadio with production PersistentJournal/PersistentSlotBackend and an injectable host blob store. It passed optimized and ASan/UBSan: failed write, false write acknowledgement with proved readback, failed readback, retry/reboot, no campaign resource mutation, Save30/epoch1 byte checks, and ten terminal-settlement cuts (seven conservative losses, three persisted wins), without repeated payment. This is not a physical NVS/ESP flash test.

New tests include 300-second live idle with an available surrender, seven-command/seven-ACK burst loss, final MoveAck exhaustion recovered by receipt, complete terminal-response partition without fabricated reward, simultaneous surrender in both delivery/turn orders, surrender during an opponent action or own pending command, receipt identity/sequence/winner/digest poison, 100 duplicate commands/receipts, confirmed ACK/receipt before Cancel, Cancel before proof in the same or a later batch, original-deadline expiry without extension, late-proof rejection after settlement, result exit/reopen and old-version rejection. A further 2,000 seeded two-Session races exercise temporary packet loss, reordering and duplication; both endpoints must reach an identical confirmed digest.

LeakSanitizer is disabled because the host's ptrace environment does not support it. No leak pass, actual RF reliability, board FPS, ESP32 RAM/flash margin, radio-send/peer-table errors, physical touch, physical power cuts or two-board qualification is claimed. The fake radio always reports successful enqueue/send. Full aggregate and firmware qualification remain pending by design.

The receive batch is deliberately processed before timer expiry, as in the existing adapter: valid queued proof can win over an elapsed timer until failure/settlement is actually processed. With no proof, the tick expires the original ten-second command deadline; Cancel never restarts it. Tests cover the exact boundary and uint32 wrap.

The closing-session proof window is half-open: [close time, close time + 10 seconds). A proof at its exact upper boundary is rejected even if the Duel timer has not run again. This independent closing-window bound does not reset the original pending deadline.
