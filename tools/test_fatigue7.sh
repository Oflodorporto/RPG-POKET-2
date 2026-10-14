#!/usr/bin/env bash
set -euo pipefail
mkdir -p build/fatigue7/screens
python3 tools/check_controller_snapshot.py
python3 tools/unpack_art.py >/dev/null
for pass in ${RPG_FATIGUE7_PASSES:-normal sanitizer}; do
 flags=(-O2)
 if [[ "$pass" == sanitizer ]]; then flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); fi
 for name in fatigue7 fatigue7_crossings fatigue7_controller fatigue7_render fatigue76 fatigue76_controller fatigue76_persistence fatigue76_club_radio fatigue76_render combat_trace_semantics combat_trace_controller combat_trace_render club_duel club_protocol class_powers martial_powers; do
  extras=();args=()
  if [[ "$name" == fatigue7 ]]; then extras+=(firmware/RPG_POKET_2/ClubDuel.cpp); fi
  if [[ "$name" == fatigue76_club_radio ]]; then extras+=(firmware/RPG_POKET_2/ClubProtocol.cpp firmware/RPG_POKET_2/ClubDuel.cpp -Itests/club_fake); fi
  if [[ "$name" == club_duel || "$name" == club_protocol ]]; then extras+=(firmware/RPG_POKET_2/ClubProtocol.cpp firmware/RPG_POKET_2/ClubDuel.cpp -Ifirmware/RPG_POKET_2); fi
  if [[ "$name" == *_render ]]; then args+=(build/fatigue7/screens); fi
  echo "RUN $pass $name"
  g++ -std=c++17 -UNDEBUG "${flags[@]}" "tests/$name.cpp" "${extras[@]}" -o "build/fatigue7/${name}_${pass}"
  ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-0} UBSAN_OPTIONS=halt_on_error=1 "build/fatigue7/${name}_${pass}" "${args[@]}"
 done
done
