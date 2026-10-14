#!/usr/bin/env bash
set -euo pipefail
python3 tools/check_controller_snapshot.py
mkdir -p build/tests build/screens/procedural6
for pass in normal sanitizer; do
 if [[ "$pass" == normal ]]; then flags=(-O2); seeds=100000; else flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); seeds=10000; fi
 for name in procedural6 procedural6_persistence procedural6_controller procedural6_render save29 persistence29_adapter dungeon_save island_save event_expedition fatigue76; do
  echo "RUN $pass $name"
  g++ -std=c++17 -UNDEBUG "${flags[@]}" "tests/$name.cpp" -o "build/tests/${name}_${pass}"
  args=();if [[ "$name" == procedural6 ]]; then args+=("$seeds");elif [[ "$name" == procedural6_render ]]; then args+=(build/screens/procedural6);fi
  ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/tests/${name}_${pass}" "${args[@]}"
 done
done
