#!/usr/bin/env bash
set -euo pipefail
# The default CI gate always includes all inherited native/sanitizer tests.
# --focused is a local iteration aid, never accepted by qualification workflows.
case "${1:-}" in ''|--focused) ;; *) echo 'usage: tools/test_package5.sh [--focused]' >&2; exit 2;; esac
if [[ "${1:-}" != --focused ]]; then bash tools/test_native.sh; else python3 tools/unpack_art.py; fi
python3 tools/check_package5_scope.py
python3 tools/check_controller_snapshot.py
python3 tools/prepare_d20_mesh.py --check
python3 tools/prepare_tent_marker.py --check
python3 assets/reusable/effects01/check_library.py
python3 assets/reusable/equipment01/check_library.py
mkdir -p build/tests build/d20-camp77 build/ruins-bag77 build/screens/camp-identity77
for name in d20_camp77_render d20_camp77_controller camp_identity77_render ruins_bag77_controller ruins_bag77_render; do
  extras=()
  if [[ "$name" == d20_camp77_render || "$name" == camp_identity77_render ]]; then extras+=(-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc); fi
  for pass in normal sanitizer; do
    if [[ "$pass" == normal ]]; then flags=(-O2 -Wall -Wextra); else flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); fi
    echo "RUN package5 $pass $name"
    g++ -std=c++17 -UNDEBUG "${flags[@]}" "tests/$name.cpp" "${extras[@]}" -o "build/tests/${name}_${pass}"
    args=()
    if [[ "$name" == ruins_bag77_render ]]; then args+=(build/ruins-bag77); fi
    ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/tests/${name}_${pass}" "${args[@]}"
  done
done
