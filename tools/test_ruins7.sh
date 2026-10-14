#!/usr/bin/env bash
set -euo pipefail
python3 tools/check_controller_snapshot.py
python3 tools/unpack_art.py
mkdir -p build/ruins7
for pass in normal sanitizer; do
 if [[ $pass == normal ]]; then flags=(-O2); names='ruins7_noalloc ruins7_render ruins7_controller ruins_bag77_controller package6_integration_controller title_controller dungeon_controller touch77_controller carvalho77_controller dungeon77_postvictory dungeon77_layout_controller save29 reset29'; else flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); names='ruins7_noalloc ruins7_render ruins7_controller ruins_bag77_controller package6_integration_controller'; fi
 for name in $names; do
  echo "RUN ruins7 $pass $name"
  extras=(); [[ $name != ruins7_noalloc ]] || extras=(-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc)
  g++ -std=c++17 -UNDEBUG "${flags[@]}" "tests/$name.cpp" "${extras[@]}" -o "build/ruins7/${name}_$pass"
  args=(); [[ $name != *_render ]] || args=(build/ruins7)
  ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-0} UBSAN_OPTIONS=halt_on_error=1 "build/ruins7/${name}_$pass" "${args[@]}"
 done
done
python3 tools/capture_ruins7.py
