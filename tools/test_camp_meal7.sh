#!/usr/bin/env bash
set -euo pipefail
python3 tools/check_controller_snapshot.py
if [[ "${1:-}" == --isolated ]]; then python3 tools/check_camp_meal7_scope.py; fi
mkdir -p build/tests build/screens/camp-identity77
for name in camp_identity77_render camp_meal7_errors camp_meal7_controller d20_camp77_controller camp_loot daily_travel29 daily_travel29_combat daily77_render; do
  extras=()
  if [[ "$name" == camp_identity77_render ]]; then extras+=(-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc); fi
  for mode in normal sanitizer; do
    flags=(-O2)
    if [[ "$mode" == sanitizer ]]; then flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); fi
    g++ -std=c++17 -UNDEBUG -Wall -Wextra "${flags[@]}" "tests/$name.cpp" "${extras[@]}" -o "build/tests/${name}_meal7_$mode"
    args=(); if [[ "$name" == daily77_render ]]; then args+=(build/screens); fi
    ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/tests/${name}_meal7_$mode" "${args[@]}"
    echo "PASS TARGET: $name / $mode"
  done
done
python3 tools/capture_camp_meal7.py
