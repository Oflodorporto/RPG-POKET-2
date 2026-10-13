#!/usr/bin/env bash
set -euo pipefail
python3 tools/check_d20_camp77_scope.py
python3 tools/prepare_d20_mesh.py --check
python3 tools/prepare_tent_marker.py --check
python3 assets/reusable/effects01/check_library.py
python3 assets/reusable/equipment01/check_library.py
mkdir -p build/tests build/d20-camp77
for name in d20_camp77_render d20_camp77_controller camp_identity77_render; do
  extras=()
  if [[ "$name" == d20_camp77_render || "$name" == camp_identity77_render ]]; then extras+=(-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc); fi
  g++ -std=c++17 -O2 -UNDEBUG -Wall -Wextra "tests/$name.cpp" "${extras[@]}" -o "build/tests/$name"
  "build/tests/$name"
  g++ -std=c++17 -O1 -g -UNDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie "tests/$name.cpp" "${extras[@]}" -o "build/tests/${name}_san"
  ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/tests/${name}_san"
done
# Pass --full only after visual acceptance and final source freeze.
if [[ "${1:-}" != --full ]]; then exit 0; fi
# Full inherited suite, including daily and ordinary camp controllers, every
# enemy, personalized heroes, touch, OTA, Save29, reset, scenes and fallbacks.
bash tools/test_native.sh
