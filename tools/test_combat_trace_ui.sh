#!/usr/bin/env bash
set -euo pipefail
mkdir -p build/tests build/screens/combat6
python3 tools/check_controller_snapshot.py
python3 tools/unpack_art.py >/dev/null
for pass in normal sanitizer; do
 if [[ "$pass" == normal ]]; then flags=(-O2); else flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); fi
 for name in combat_trace_semantics combat_trace_controller combat_trace_render; do
  g++ -std=c++17 -UNDEBUG "${flags[@]}" "tests/$name.cpp" -o "build/tests/${name}_${pass}"
  args=();if [[ "$name" == combat_trace_render ]]; then args+=(build/screens/combat6); fi
  ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-0} UBSAN_OPTIONS=halt_on_error=1 "build/tests/${name}_${pass}" "${args[@]}"
 done
done
