# Independent controller review

Date: 2026-10-14. Scope: local-only `CoopTouchUi.h`, its interface to the frozen cooperative engine, and `tests.cpp`. No firmware, radio, save format, production code, integration, or publication changes were made by this reviewer. Renderer review is separate.

## Result

Two controller defects were reproduced, reported to the implementer, fixed in the new UI header, and independently regression-tested. No unresolved controller defect was found in this scope.

1. **Drag-out-and-return activated the original target.** The original held-contact path ignored all subsequent pressed coordinates. A down on an enemy, movement to another enemy or outside the screen, and release back on the original enemy submitted an attack. The corrected `onTouch` at `CoopTouchUi.h:69-74` permanently disarms that contact after any sampled hit-zone change. Motion remaining within one hit zone is permitted; a fresh release/down cycle is required after leaving it. Tests: `review_tests.cpp:23-47` and `:123-142`.
2. **Unrelated peer rejection overwrote local success presentation.** During the host's already-accepted action awaiting ACK, a different peer command could set `Endpoint.lastError=Busy`. After the ACK, the original controller displayed Rejected even though its action had been accepted and applied exactly once. The corrected completion path at `CoopTouchUi.h:29-32`, with submission identity captured at `:88`, checks the authoritative accepted sequence for that specific submitting peer and clears unrelated errors on success. Tests: `review_tests.cpp:48-56`, `:110-121`.

The initially failing run is recorded in `output/coop-ui-targeting/review-initial.log`: 13 cases, 4,921 checks, four failed assertions representing these two defects.

## Final verification

- Independent suite: **16 cases, 10,929 checks, zero failures**, both strict C++17 optimized and ASan+UBSan builds.
- Includes 2,000 combinations of start, held-movement, and release positions in Action and Target modes; invalid cursor/local peer input; half-open hit boundaries; held touch across pause/state changes; same-revision state-hash changes; pending touch/ACK races; same-revision recovery retaining the original intent; sequence/resource/turn gates; rejection versus acceptance and nonce change.
- Main UI suite independently rerun after the fixes: optimized **460,982 checks**, sanitized **460,965 checks**; 7 scenarios, 230,400 exhaustive geometry points; zero instrumented C++ heap allocations.
- Native `sizeof(TouchUi)` remains **48 bytes**. This is not an ESP32 RAM, flash, or physical touch measurement.
- LeakSanitizer was disabled (`detect_leaks=0`) for the executor's ptrace limitation. AddressSanitizer and UndefinedBehaviorSanitizer were enabled, with UBSan set to halt on error.
- All seven files in `prototypes/cooperative/` were compared byte-for-byte against the frozen `output/coop-prototype/cooperative-local-prototype.zip` and match. Frozen input archive, patch, and trace checksums also pass.

Logs: `output/coop-ui-targeting/review-optimized.log`, `review-sanitized.log`, and `review-primary/ui-optimized.log` / `ui-sanitized.log`.

Reviewed controller SHA-256: `93426be93b2b399f53d299dc381625734648eb794686b01799975090c6674263`.

Reviewed independent tests SHA-256: `0dcca2cc8d0c41f754659c45aac30ba13996409344431989e2ea86de46ae3977`.

## Reproduce independent suite

From the extracted checkout root:

```sh
mkdir -p /tmp/coop-ui-review
flags='-std=c++17 -Wall -Wextra -Wpedantic -Werror -Wno-misleading-indentation -Wno-sign-compare'
g++ $flags -O2 prototypes/cooperative_targeting/review_tests.cpp -o /tmp/coop-ui-review/review-opt
/tmp/coop-ui-review/review-opt
g++ $flags -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined prototypes/cooperative_targeting/review_tests.cpp -o /tmp/coop-ui-review/review-san
ASAN_OPTIONS=detect_leaks=0:strict_string_checks=1 UBSAN_OPTIONS=halt_on_error=1 /tmp/coop-ui-review/review-san
```

The touch API assumes one logical contact, serialized calls, logical landscape coordinates, and meaningful release coordinates. It rejects observed zone crossings; it cannot observe movement between supplied samples. Hardware calibration, multi-touch, driver release-coordinate behavior, and the complete class-ability system remain outside this proof.
