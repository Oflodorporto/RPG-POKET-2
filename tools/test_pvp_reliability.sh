#!/usr/bin/env bash
# Focused local candidate checks. Does not publish, build ESP32 or run aggregates.
set -euo pipefail
cd "$(dirname "$0")/.."
out="${1:-/tmp/rpg-pvp-reliability}"
mkdir -p "$out"
for name in club_duel club_protocol fatigue76_club_radio pvp_reliability_protocol pvp_reliability_radio pvp_reliability_races pvp_reliability_closing pvp_reliability_persistence pvp_reliability_ordering pvp_reliability_save30; do
  extras=(-Ifirmware/RPG_POKET_2 -Itests/club_fake firmware/RPG_POKET_2/ClubProtocol.cpp firmware/RPG_POKET_2/ClubDuel.cpp)
  g++ -std=c++17 -O2 -UNDEBUG -Wall -Wextra "tests/$name.cpp" "${extras[@]}" -o "$out/$name" 2>"$out/$name-build.log"
  "$out/$name" | tee "$out/$name-run.log"
  g++ -std=c++17 -O1 -g -UNDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie "tests/$name.cpp" "${extras[@]}" -o "$out/${name}_san" 2>"$out/$name-san-build.log"
  ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$out/${name}_san" | tee "$out/$name-san-run.log"
done
g++ -std=c++17 -O2 -UNDEBUG -Wall -Wextra tests/guild_save.cpp -o "$out/guild_save" 2>"$out/guild_save-build.log"
"$out/guild_save" | tee "$out/guild_save-run.log"
echo 'PASS: 11 optimized targets and 10 ASan/UBSan targets; LeakSanitizer disabled in this ptrace environment.'
