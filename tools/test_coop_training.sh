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
 if [[ "$name" == radio || "$name" == device ]]; then extras+=(-Itests/coop_fake); fi
 if [[ "$name" == pvp_exclusion ]]; then extras+=(-Itests/club_fake -Ifirmware/RPG_POKET_2 firmware/RPG_POKET_2/ClubProtocol.cpp firmware/RPG_POKET_2/ClubDuel.cpp); fi
 g++ "${flags[@]}" -O2 "tests/coop_training_$name.cpp" "${extras[@]}" -o "$out/$name"
 "$out/$name" | tee "$out/$name-optimized.log"
 if [[ "$name" != default_off ]]; then
  g++ "${flags[@]}" -O1 -g -fno-omit-frame-pointer -no-pie -fsanitize=address,undefined "tests/coop_training_$name.cpp" "${extras[@]}" -o "$out/$name-sanitized"
  ASAN_OPTIONS=detect_leaks=0:strict_string_checks=1 UBSAN_OPTIONS=halt_on_error=1 "$out/$name-sanitized" | tee "$out/$name-sanitizers.log"
 fi
done
if nm -C "$out/default_off" | grep -E 'coopSession|coopradio::|coopplatform::Session'; then echo 'ERROR: cooperative runtime symbol in default build';exit 1;fi
for name in club_protocol club_duel pvp_reliability_save30; do
 g++ -std=c++17 -O2 -UNDEBUG -Ifirmware/RPG_POKET_2 -Itests/club_fake "tests/$name.cpp" firmware/RPG_POKET_2/ClubProtocol.cpp firmware/RPG_POKET_2/ClubDuel.cpp -o "$out/$name"
 "$out/$name" | tee "$out/$name-optimized.log"
done
echo 'PASS: 6 candidate optimized, 5 candidate ASan/UBSan, 3 existing PvP optimized, default-off symbol check. ESP32/RF/physical LCD not tested.' | tee "$out/summary.log"
