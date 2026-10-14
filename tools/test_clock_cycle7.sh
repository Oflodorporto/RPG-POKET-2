#!/usr/bin/env bash
set -euo pipefail
mkdir -p build/tests build/clock-cycle7
python3 tools/check_controller_snapshot.py
python3 tools/prepare_clock_art.py --check
for target in clock_cycle7_logic clock_cycle7_render clock77_render; do
  flags=(); args=()
  if [[ "$target" != clock_cycle7_logic ]]; then flags+=(-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc); args+=(build/clock-cycle7); fi
  if [[ "$target" == clock77_render ]]; then flags+=(-DRPG_TEST_CLOCK_ALLOCATIONS); fi
  g++ -std=c++17 -O2 -UNDEBUG "tests/$target.cpp" "${flags[@]}" -o "build/tests/$target"
  "build/tests/$target" "${args[@]}"
  g++ -std=c++17 -O1 -g -UNDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie "tests/$target.cpp" "${flags[@]}" -o "build/tests/${target}_san"
  ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/tests/${target}_san" "${args[@]}"
done
python3 tools/test_clock_cycle7.py
python3 tools/capture_clock_cycle7.py
python3 tools/capture_clock_visibility7.py
