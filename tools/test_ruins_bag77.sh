#!/usr/bin/env bash
set -euo pipefail
python3 tools/check_controller_snapshot.py
python3 tools/check_ruins_bag77_scope.py
mkdir -p build/ruins-bag77
for pass in normal sanitizer; do
 if [[ "$pass" == normal ]]; then flags=(-O2); else flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); fi
 for name in ruins_bag77_controller ruins_bag77_render touch77_controller dungeon77_postvictory dungeon77_layout_controller save29 reset29; do
  echo "RUN $pass $name"
  g++ -std=c++17 -UNDEBUG "${flags[@]}" "tests/$name.cpp" -o "build/ruins-bag77/${name}_${pass}"
  ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/ruins-bag77/${name}_${pass}" build/ruins-bag77
 done
done
python3 tools/capture_ruins_bag77.py
