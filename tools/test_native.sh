#!/usr/bin/env bash
set -eu
python3 tools/unpack_art.py
mkdir -p build/tests build/screens
for name in club_protocol club_duel guild_save guild_events camp_loot dungeon_save dungeon_controller title_controller travel_economy rules_save gear_save forge_save quests_save card_read menu_state settings_adapter sd_adapter class_progression class_powers martial_powers launch_readiness campaign_port origin_story render; do
  extras=()
  if [[ "$name" == club_protocol || "$name" == club_duel ]]; then extras+=(firmware/RPG_POKET_2/ClubProtocol.cpp firmware/RPG_POKET_2/ClubDuel.cpp -Ifirmware/RPG_POKET_2); fi
  if [[ "$name" == settings_adapter ]]; then extras+=(-Itests/settings_fake); fi
  if [[ "$name" == sd_adapter ]]; then extras+=(-Itests/sd_fake); fi
  g++ -std=c++17 -O2 -UNDEBUG -Wall -Wextra "tests/$name.cpp" "${extras[@]}" -o "build/tests/$name"
  if [[ "$name" == render ]]; then "build/tests/$name" build/screens; else "build/tests/$name"; fi
done
