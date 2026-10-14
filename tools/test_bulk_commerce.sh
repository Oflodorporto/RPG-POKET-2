#!/usr/bin/env bash
# Focused candidate gate; no remote publication, release assembly or ESP32 build.
set -euo pipefail
cd "$(dirname "$0")/.."
export PYTHONDONTWRITEBYTECODE=1
export RPG_DETECT_LEAKS="${RPG_DETECT_LEAKS:-0}"
mode="${1:-normal}"
[[ "$mode" == normal || "$mode" == sanitizers ]]
out="build/bulk-shop/$mode"
mkdir -p "$out" build/bulk-shop/screens
python3 tools/check_controller_snapshot.py
python3 tools/unpack_art.py > "$out/art.log"
flags=(-std=c++17 -UNDEBUG)
if [[ "$mode" == sanitizers ]]; then
  flags+=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie)
else flags+=(-O2); fi
for name in bulk_commerce bulk_commerce_controller bulk_commerce_render ration_limit10 commerce75 title_controller dungeon_controller ruins7_controller touch77_controller ruins_bag77_controller camp_meal7_controller escort29_controller narrative8_controller save29 event_catalog exploration_save travel_economy daily_travel29 daily_travel29_combat camp_loot procedural6_persistence long_session first_experience pvp_reliability_save30 pvp_review_integration pvp_review_equivalence; do
  extras=()
  if [[ "$name" == bulk_commerce_render ]]; then extras+=(-DRPG_TEST_COMMERCE_ALLOCATIONS -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc); fi
  if [[ "$name" == pvp_* ]]; then extras+=(-Ifirmware/RPG_POKET_2 -Itests/club_fake firmware/RPG_POKET_2/ClubProtocol.cpp firmware/RPG_POKET_2/ClubDuel.cpp); fi
  g++ "${flags[@]}" "tests/$name.cpp" "${extras[@]}" -o "$out/$name" >"$out/$name-build.log" 2>&1
  args=();if [[ "$name" == bulk_commerce_render ]]; then args+=(build/bulk-shop/screens); fi
  # LSan is unsupported under the traced executor. Keep ASan/UBSan enabled.
  ASAN_OPTIONS="detect_leaks=${RPG_DETECT_LEAKS:-0}" UBSAN_OPTIONS=halt_on_error=1 "$out/$name" "${args[@]}" >"$out/$name-run.log" 2>&1
  tail -1 "$out/$name-run.log"
done
if [[ "$mode" == sanitizers ]]; then python3 tools/test_ration_limit10_oldreader.py --sanitizers;
else python3 tools/test_ration_limit10_oldreader.py; fi
python3 tools/check_bulk_commerce_scope.py
echo "PASS: focused bulk commerce $mode gate; target compile/hardware/release gates not run."
