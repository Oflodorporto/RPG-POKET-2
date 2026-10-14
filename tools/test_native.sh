#!/usr/bin/env bash
set -eu
python3 tools/check_controller_snapshot.py
python3 tools/prepare_clock_art.py --check
python3 tools/prepare_scene_art.py --check > build-scene-report.tmp
rm build-scene-report.tmp
python3 tests/scene_asset_catalog.py
python3 tools/unpack_art.py
python3 tools/prepare_enemy_art.py --check --extract-reference build/enemy_art_source
python3 tests/enemy_asset_catalog.py
python3 tools/test_enemy_transition.py
python3 tools/unpack_art.py --baseline-only --output build/enemy_transition/old.pak
mkdir -p build/tests build/screens build/screens/fatigue76 build/screens/visual77 build/screens/hero77 build/screens/enemy77
for name in scale77_render scale77_depth save29 reset29 persistence29_adapter daily_travel29 daily_travel29_combat escort_recovery29 scenes77_controller scenes29_controller escort29_controller scenes77_render outcomes77_render daily77_render clock77_render carvalho77_controller carvalho77_render touch77_controller touch77_render dungeon77_postvictory dungeon77_result_render dungeon77_layout_controller ota_art_readback boot_art_diagnostics shared_spi_startup ota_verify_render ota_sd_diagnostics ota_sd_diagnostics_render hero77_controller hero77_render enemy_codec enemy_controller enemy_render enemy_ota visual77_controller visual77_render fatigue76 fatigue76_persistence fatigue76_controller fatigue76_club_radio fatigue76_render commerce75 commerce75_render balance74 long_session render_session first_experience persistence_adapter update_adapter event_catalog event_expedition event_objectives regional_world island_save club_protocol club_duel guild_save guild_events camp_loot dungeon_save dungeon_controller title_controller travel_economy rules_save gear_save forge_save quests_save card_read menu_state settings_adapter sd_adapter class_progression class_powers martial_powers launch_readiness campaign_port origin_story exploration_save locks_save progression_journey paladin_smite champion_progression arcane_progression barbarian_progression enemy_info bestiary_save campaign_aurora campaign_anwen campaign_finale dialogue_story render; do
  extras=()
  if [[ "$name" == clock77_render ]]; then extras+=(-DRPG_TEST_CLOCK_ALLOCATIONS -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc); fi
  if [[ "$name" == ota_art_readback ]]; then extras+=(tests/update_fake/cJSON.c firmware/RPG_POKET_2/UpdateInstall.cpp -Itests/update_fake -Ifirmware/RPG_POKET_2 -DRPG_TEST_REAL_SHA256= -DRPG_TEST_VERIFY_DETAIL -Wno-deprecated-declarations -lcrypto); fi
  if [[ "$name" == boot_art_diagnostics ]]; then extras+=(-Itests/settings_fake); fi
  if [[ "$name" == shared_spi_startup ]]; then extras+=(-Itests/shared_spi_fake); fi
  if [[ "$name" == enemy_render || "$name" == scale77_render ]]; then extras+=(-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc); fi
  if [[ "$name" == fatigue76_club_radio ]]; then extras+=(firmware/RPG_POKET_2/ClubProtocol.cpp firmware/RPG_POKET_2/ClubDuel.cpp -Itests/club_fake); fi
  if [[ "$name" == ota_sd_diagnostics || "$name" == enemy_ota || "$name" == update_adapter ]]; then extras+=(tests/update_fake/cJSON.c firmware/RPG_POKET_2/UpdateInstall.cpp -Itests/update_fake); fi
  if [[ "$name" == club_protocol || "$name" == club_duel ]]; then extras+=(firmware/RPG_POKET_2/ClubProtocol.cpp firmware/RPG_POKET_2/ClubDuel.cpp -Ifirmware/RPG_POKET_2); fi
  if [[ "$name" == settings_adapter ]]; then extras+=(-Itests/settings_fake); fi
  if [[ "$name" == sd_adapter ]]; then extras+=(-Itests/sd_fake); fi
  g++ -std=c++17 -O2 -UNDEBUG -Wall -Wextra "tests/$name.cpp" "${extras[@]}" -o "build/tests/$name"
  if [[ "$name" == scenes77_render || "$name" == outcomes77_render || "$name" == daily77_render || "$name" == clock77_render || "$name" == carvalho77_render || "$name" == touch77_render || "$name" == dungeon77_result_render || "$name" == ota_verify_render || "$name" == ota_sd_diagnostics_render ]]; then "build/tests/$name" build/screens; elif [[ "$name" == hero77_controller ]]; then "build/tests/$name" build/hero77-coverage.csv; elif [[ "$name" == hero77_render ]]; then "build/tests/$name" build/screens/hero77; elif [[ "$name" == visual77_render ]]; then "build/tests/$name" build/screens/visual77; elif [[ "$name" == fatigue76_render ]]; then "build/tests/$name" build/screens/fatigue76; elif [[ "$name" == commerce75 ]]; then "build/tests/$name" build/commerce75-economy.csv; elif [[ "$name" == commerce75_render ]]; then "build/tests/$name" build/screens; elif [[ "$name" == balance74 ]]; then "build/tests/$name" build/balance74-rewards.csv; elif [[ "$name" == progression_journey ]]; then "build/tests/$name" build/progressao.csv; elif [[ "$name" == render ]]; then "build/tests/$name" build/screens; else "build/tests/$name"; fi
done

# Targeted memory/undefined-behaviour audit of real persistence/OTA adapters and renderer.
for name in scale77_render scale77_depth save29 reset29 persistence29_adapter daily_travel29 daily_travel29_combat escort_recovery29 scenes77_controller scenes29_controller escort29_controller scenes77_render outcomes77_render daily77_render clock77_render carvalho77_controller carvalho77_render touch77_controller touch77_render dungeon77_postvictory dungeon77_result_render dungeon77_layout_controller ota_art_readback boot_art_diagnostics shared_spi_startup ota_verify_render ota_sd_diagnostics ota_sd_diagnostics_render hero77_controller hero77_render enemy_codec enemy_controller enemy_render enemy_ota visual77_controller visual77_render fatigue76 fatigue76_persistence fatigue76_controller fatigue76_club_radio fatigue76_render commerce75 commerce75_render balance74 long_session render_session first_experience persistence_adapter update_adapter render; do
  extras=()
  if [[ "$name" == clock77_render ]]; then extras+=(-DRPG_TEST_CLOCK_ALLOCATIONS -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc); fi
  if [[ "$name" == ota_art_readback ]]; then extras+=(tests/update_fake/cJSON.c firmware/RPG_POKET_2/UpdateInstall.cpp -Itests/update_fake -Ifirmware/RPG_POKET_2 -DRPG_TEST_REAL_SHA256= -DRPG_TEST_VERIFY_DETAIL -Wno-deprecated-declarations -lcrypto); fi
  if [[ "$name" == boot_art_diagnostics ]]; then extras+=(-Itests/settings_fake); fi
  if [[ "$name" == shared_spi_startup ]]; then extras+=(-Itests/shared_spi_fake); fi
  if [[ "$name" == enemy_render || "$name" == scale77_render ]]; then extras+=(-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc); fi
  if [[ "$name" == fatigue76_club_radio ]]; then extras+=(firmware/RPG_POKET_2/ClubProtocol.cpp firmware/RPG_POKET_2/ClubDuel.cpp -Itests/club_fake); fi
  if [[ "$name" == ota_sd_diagnostics || "$name" == enemy_ota || "$name" == update_adapter ]]; then extras+=(tests/update_fake/cJSON.c firmware/RPG_POKET_2/UpdateInstall.cpp -Itests/update_fake); fi
  g++ -std=c++17 -O1 -g -UNDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie "tests/$name.cpp" "${extras[@]}" -o "build/tests/${name}_san"
  if [[ "$name" == scenes77_render || "$name" == outcomes77_render || "$name" == daily77_render || "$name" == clock77_render || "$name" == carvalho77_render || "$name" == touch77_render || "$name" == dungeon77_result_render || "$name" == ota_verify_render || "$name" == ota_sd_diagnostics_render || "$name" == hero77_render || "$name" == visual77_render || "$name" == render || "$name" == commerce75_render || "$name" == fatigue76_render ]]; then ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/tests/${name}_san" build/screens; else ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/tests/${name}_san"; fi
done

python3 tools/test_enemy_transition.py --sanitizers
