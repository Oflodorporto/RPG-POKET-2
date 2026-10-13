#!/usr/bin/env bash
set -euo pipefail
python3 tools/check_controller_snapshot.py
# Preserve the already published storage implementation byte for byte.
git diff --exit-code cf0639ebf7e1dc729a026d09a8ef27f1e7dbc9ae -- firmware/RPG_POKET_2/AssetStore.h firmware/RPG_POKET_2/DeviceSettings.h firmware/RPG_POKET_2/ArtPromotion.h firmware/RPG_POKET_2/StorageDiagnostics.h firmware/RPG_POKET_2/UpdateDevice.h firmware/RPG_POKET_2/UpdateModel.h firmware/RPG_POKET_2/UpdateView.h firmware/RPG_POKET_2/SharedStorage.h firmware/RPG_POKET_2/Rules.h firmware/RPG_POKET_2/Save.h firmware/RPG_POKET_2/AssetCatalog.h tools/test_ota_reliability.sh
python3 tools/unpack_art.py
python3 tools/unpack_art.py --baseline-only --output build/enemy_transition/old.pak
mkdir -p build/tests build/screens/dungeon77
for pass in normal sanitizer; do
 if [[ "$pass" == normal ]]; then names='dungeon77_postvictory dungeon77_result_render dungeon77_layout_controller dungeon_controller title_controller dungeon_save event_expedition island_save'; flags=(-O2); else names='dungeon77_postvictory dungeon77_result_render dungeon77_layout_controller'; flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); fi
 for name in $names; do
  echo "RUN $pass $name"
  g++ -std=c++17 -UNDEBUG "${flags[@]}" "tests/$name.cpp" -o "build/tests/$name"
  args=(); if [[ "$name" == dungeon77_result_render ]]; then args+=(build/screens/dungeon77); fi
  ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/tests/$name" "${args[@]}"
 done
done
