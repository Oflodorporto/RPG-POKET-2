#!/usr/bin/env bash
# Bounded native candidate checks. No Arduino installation, CI or publication.
set -euo pipefail
cd "$(dirname "$0")/.."
out="${1:-/tmp/coop-training-final}"
mkdir -p "$out"
python3 tools/check_coop_training_scope.py | tee "$out/scope.log"
flags=(-std=c++17 -UNDEBUG -Wall -Wextra -Werror -Wno-misleading-indentation -Wno-sign-compare -Wno-unused-function -Wno-switch -Wno-unused-parameter)
# Warning exemptions are inherited production view/test-harness style.
for name in session radio input device pvp_exclusion default_off; do
 extras=()
 if [[ "$name" == radio || "$name" == device || "$name" == input ]]; then extras+=(-Itests/coop_fake); fi
 if [[ "$name" == device || "$name" == input ]]; then extras+=(-DRPG_ENABLE_COOP_TRAINING=1 firmware/RPG_POKET_2/CoopRuntime.cpp); fi
 if [[ "$name" == default_off ]]; then extras+=(firmware/RPG_POKET_2/CoopRuntime.cpp); fi
 if [[ "$name" == pvp_exclusion ]]; then extras+=(-Itests/club_fake -Ifirmware/RPG_POKET_2 firmware/RPG_POKET_2/ClubProtocol.cpp firmware/RPG_POKET_2/ClubDuel.cpp); fi
 g++ "${flags[@]}" -O2 "tests/coop_training_$name.cpp" "${extras[@]}" -o "$out/$name"
 "$out/$name" | tee "$out/$name-optimized.log"
 if [[ "$name" != default_off ]]; then
  g++ "${flags[@]}" -O1 -g -fno-omit-frame-pointer -no-pie -fsanitize=address,undefined "tests/coop_training_$name.cpp" "${extras[@]}" -o "$out/$name-sanitized"
  ASAN_OPTIONS=detect_leaks=0:strict_string_checks=1 UBSAN_OPTIONS=halt_on_error=1 "$out/$name-sanitized" | tee "$out/$name-sanitizers.log"
 fi
done
if nm -C "$out/default_off" | grep -E 'coopSession|coopInput|coopDevice|coopruntime::|coopradio::|coopplatform::Session'; then echo 'ERROR: cooperative runtime symbol in default build';exit 1;fi
g++ "${flags[@]}" -O2 -c firmware/RPG_POKET_2/CoopRuntime.cpp -o "$out/runtime-off.o"
test -z "$(nm --defined-only "$out/runtime-off.o")" || { echo 'ERROR: disabled runtime translation unit has definitions';exit 1; }
echo 'PASS: flag0 CoopRuntime.cpp has no defined symbols or static initialization' | tee "$out/runtime-off.log"
g++ "${flags[@]}" -DRPG_ENABLE_COOP_TRAINING=1 -Itests/coop_fake -MM firmware/RPG_POKET_2/CoopRuntime.cpp > "$out/runtime-dependencies.log"
if grep -E '/(Persistence|Dungeon|AssetCatalog|PersonalArt|EnemyArt|CoopView|CoopPortraitBridge|CoopEligibilityBridge|View|MenuView|UpdateDevice)\.h|/font/' "$out/runtime-dependencies.log"; then echo 'ERROR: legacy art/save headers leaked into runtime TU';exit 1;fi
g++ "${flags[@]}" -O2 -DRPG_ENABLE_COOP_TRAINING=1 -Itests/coop_fake -c firmware/RPG_POKET_2/CoopRuntime.cpp -o "$out/runtime-on.o"
nm -S -C --defined-only "$out/runtime-on.o" > "$out/runtime-symbols.log"
if grep -E ' assetMeta$| artFallback$| font$| artMemory$|enemyArt::workspace|ruins7Art::' "$out/runtime-symbols.log"; then echo 'ERROR: art storage emitted by runtime TU';exit 1;fi
size "$out/runtime-on.o" "$out/runtime-off.o" | tee "$out/runtime-object-size.log"
for name in club_protocol club_duel pvp_reliability_save30; do
 g++ -std=c++17 -O2 -UNDEBUG -Ifirmware/RPG_POKET_2 -Itests/club_fake "tests/$name.cpp" firmware/RPG_POKET_2/ClubProtocol.cpp firmware/RPG_POKET_2/ClubDuel.cpp -o "$out/$name"
 "$out/$name" | tee "$out/$name-optimized.log"
done
echo 'PASS: 6 candidate optimized, 5 candidate ASan/UBSan, 3 existing PvP optimized, default-off symbol check. ESP32/RF/physical LCD not tested.' | tee "$out/summary.log"
