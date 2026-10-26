#!/usr/bin/env bash
# Focused bag decoration checks only. No upload, release, hardware or radio IO.
set -euo pipefail
python3 tools/check_controller_snapshot.py
python3 tools/prepare_touch_skin.py --check
mkdir -p build/bag-leather/screens build/bag-leather/tests
for mode in normal sanitizer; do
  flags=(-O2)
  if [[ "$mode" == sanitizer ]]; then flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); fi
  for test in bag_leather_render touch77_controller ruins_bag77_controller dungeon77_postvictory dungeon77_layout_controller save29 reset29 gear_save; do
    if [[ "$mode" == sanitizer && "$test" == gear_save ]]; then continue; fi
    extra=(); args=()
    if [[ "$test" == bag_leather_render ]]; then
      extra=(-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc)
      args=(build/bag-leather/screens)
    fi
    echo "RUN $mode $test"
    g++ -std=c++17 -UNDEBUG "${flags[@]}" "tests/$test.cpp" "${extra[@]}" -o "build/bag-leather/tests/${test}_${mode}"
    ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/bag-leather/tests/${test}_${mode}" "${args[@]}"
  done
done
