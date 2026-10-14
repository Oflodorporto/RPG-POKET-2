#!/usr/bin/env bash
# Focused narrative candidate checks. This does not replace package/release qualification.
set -euo pipefail
mkdir -p build/tests build/narrative8 build/narrative8-render build/narrative8-events build/narrative8-dungeon
python3 tools/check_controller_snapshot.py | tee build/narrative8/controller-snapshot.log
python3 tools/check_narrative8_scope.py | tee build/narrative8/scope.log
python3 tools/unpack_art.py > build/narrative8/art-prepare.log
for name in dialogue_story campaign_anwen origin_story campaign_port campaign_aurora campaign_finale event_catalog event_objectives guild_events event_expedition quests_save title_controller dungeon_controller dungeon77_postvictory dungeon_save daily_travel29 daily_travel29_combat travel_economy save29 narrative8_controller narrative8_events dungeon77_result_render; do
  g++ -std=c++17 -O2 -UNDEBUG -Wall -Wextra "tests/$name.cpp" -o "build/tests/$name" > "build/narrative8/$name-compile.log" 2>&1
  args=()
  case "$name" in
    narrative8_events) args=(build/narrative8-events);;
    dungeon77_result_render) args=(build/narrative8-dungeon);;
  esac
  "build/tests/$name" "${args[@]}" | tee "build/narrative8/$name.log"
done
python3 tools/capture_narrative8.py
if [[ "${1:-}" == "--sanitizers" ]]; then
  for name in narrative8_controller narrative8_events narrative8_render dialogue_story campaign_anwen; do
    g++ -std=c++17 -O1 -g -UNDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie "tests/$name.cpp" -o "build/tests/${name}_san" > "build/narrative8/$name-san-compile.log" 2>&1
    args=();if [[ "$name" == narrative8_render ]]; then mkdir -p build/narrative8-render/sanitizer;args=(build/narrative8-render/sanitizer cartao/RPGPOKET/artes.pak);fi
    NARRATIVE8_SKIP_IMAGES=1 ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/tests/${name}_san" "${args[@]}" | tee "build/narrative8/$name-san.log"
  done
fi
