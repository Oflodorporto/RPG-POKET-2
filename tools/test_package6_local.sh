#!/usr/bin/env bash
set -euo pipefail
# Native-only staging check. This neither qualifies nor builds a firmware release.
# The baseline is the frozen procedural-only checkout (before combat tracing).
if [[ $# != 1 || ! -f "$1/firmware/RPG_POKET_2/DungeonGeneration.h" ]]; then
 echo 'usage: tools/test_package6_local.sh /path/to/frozen-procedural-only-checkout' >&2
 exit 2
fi
baseline="$1"
echo 'LOCAL PACKAGE6 STAGING ONLY: inherited camp77 identity is not qualified.'
export RPG_DETECT_LEAKS="${RPG_DETECT_LEAKS:-0}"
python3 tools/check_controller_snapshot.py
bash tools/test_native.sh
bash tools/test_procedural6.sh
bash tools/test_combat_trace_ui.sh
python3 tools/test_combat_trace.py --baseline "$baseline" --save-format 30 --seeds 64 --report build/package6-integration/differential.json
python3 tools/test_combat_trace.py --baseline "$baseline" --save-format 30 --seeds 16 --sanitizers --report build/package6-integration/differential-sanitized.json
python3 tools/prepare_d20_mesh.py --check
python3 tools/prepare_tent_marker.py --check
python3 assets/reusable/effects01/check_library.py
python3 assets/reusable/equipment01/check_library.py
mkdir -p build/tests build/d20-camp77 build/ruins-bag77 build/screens/camp-identity77 build/screens/package6-integration
for name in package6_integration_controller package6_integration_render d20_camp77_render d20_camp77_controller camp_identity77_render ruins_bag77_controller ruins_bag77_render; do
 for pass in normal sanitizer; do
  flags=(-O2)
  if [[ "$pass" == sanitizer ]]; then flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); fi
  extras=()
  if [[ "$name" == d20_camp77_render || "$name" == camp_identity77_render ]]; then extras=(-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc); fi
  echo "RUN package6 integration $pass $name"
  g++ -std=c++17 -UNDEBUG "${flags[@]}" "tests/$name.cpp" "${extras[@]}" -o "build/tests/${name}_integration_$pass"
  args=()
  if [[ "$name" == ruins_bag77_render ]]; then args=(build/ruins-bag77); fi
  if [[ "$name" == package6_integration_render ]]; then args=(build/screens/package6-integration); fi
  ASAN_OPTIONS=detect_leaks="$RPG_DETECT_LEAKS" UBSAN_OPTIONS=halt_on_error=1 "build/tests/${name}_integration_$pass" "${args[@]}"
 done
done
echo 'PASS native integration only. Product variety, release gates, ESP32 build and device validation remain separate.'
