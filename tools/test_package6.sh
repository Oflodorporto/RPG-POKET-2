#!/usr/bin/env bash
set -euo pipefail
# Qualification always runs the complete inherited aggregate exactly once.
# --focused is strictly a local iteration aid, never invoked in CI.
case "${1:-}" in ''|--focused) ;; *) echo 'usage: tools/test_package6.sh [--focused]' >&2; exit 2;; esac
export RPG_DETECT_LEAKS="${RPG_DETECT_LEAKS:-1}"
if [[ "${1:-}" != --focused ]]; then bash tools/test_native.sh; else python3 tools/unpack_art.py; fi
python3 tools/check_package6_scope.py
python3 tests/package6_release_gate.py
python3 tools/check_controller_snapshot.py
python3 tools/prepare_d20_mesh.py --check
python3 tools/prepare_tent_marker.py --check
python3 assets/reusable/effects01/check_library.py
python3 assets/reusable/equipment01/check_library.py
bash tools/test_procedural6.sh
bash tools/test_combat_trace_ui.sh
python3 tools/test_combat_trace.py --baseline tests/fixtures/package6-procedural-only --save-format 30 --seeds 64 --report build/package6/differential.json
python3 tools/test_combat_trace.py --baseline tests/fixtures/package6-procedural-only --save-format 30 --seeds 16 --sanitizers --report build/package6/differential-sanitized.json
python3 tools/test_package6_save_review.py
python3 tools/test_package6_save_review.py --sanitizers
mkdir -p build/tests build/d20-camp77 build/ruins-bag77 build/screens/camp-identity77 build/screens/package6-integration
for name in package6_integration_controller package6_integration_render d20_camp77_render d20_camp77_controller camp_identity77_render ruins_bag77_controller ruins_bag77_render; do
 for pass in normal sanitizer; do
  flags=(-O2)
  if [[ "$pass" == sanitizer ]]; then flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); fi
  extras=()
  if [[ "$name" == d20_camp77_render || "$name" == camp_identity77_render ]]; then extras=(-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc); fi
  echo "RUN package6 qualification $pass $name"
  g++ -std=c++17 -UNDEBUG "${flags[@]}" "tests/$name.cpp" "${extras[@]}" -o "build/tests/${name}_package6_$pass"
  args=()
  if [[ "$name" == ruins_bag77_render ]]; then args=(build/ruins-bag77); fi
  if [[ "$name" == package6_integration_render ]]; then args=(build/screens/package6-integration); fi
  ASAN_OPTIONS=detect_leaks="$RPG_DETECT_LEAKS" UBSAN_OPTIONS=halt_on_error=1 "build/tests/${name}_package6_$pass" "${args[@]}"
 done
done
python3 tests/package6_release_verifier.py --synthetic
python3 tools/test_package6_release.py --synthetic-fixture
python3 tools/test_package6_release.py --synthetic-fixture --sanitizers
printf '%s\n' 'PASS host-only package6 qualification. ESP32 compilation, real-byte OTA checks and physical validation are separate gates.'
