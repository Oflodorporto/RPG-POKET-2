#!/usr/bin/env bash
# Focused candidate gates only. The unchanged gameplay/art aggregate remains separate.
set -euo pipefail
pkg-config --exists openssl || { echo 'OpenSSL development headers/library required for real SHA-256 host tests'; exit 1; }
python3 tools/check_controller_snapshot.py
python3 tools/unpack_art.py
python3 tools/unpack_art.py --baseline-only --output build/enemy_transition/old.pak
mkdir -p build/tests build/screens/ota77
for pass in normal sanitizer; do
 if [[ "$pass" == normal ]]; then names='ota_recovery ota_art_readback boot_art_diagnostics shared_spi_startup ota_verify_render update_adapter enemy_ota ota_sd_diagnostics ota_sd_diagnostics_render settings_adapter sd_adapter'; flags=(-O2); else names='ota_recovery ota_art_readback boot_art_diagnostics shared_spi_startup ota_verify_render'; flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); fi
 for name in $names; do
  extras=(); args=()
  case "$name" in
   ota_recovery|ota_art_readback) extras+=(tests/update_fake/cJSON.c firmware/RPG_POKET_2/UpdateInstall.cpp -Itests/update_fake -Ifirmware/RPG_POKET_2 -DRPG_TEST_REAL_SHA256= -DRPG_TEST_VERIFY_DETAIL -Wno-deprecated-declarations -lcrypto);;
   update_adapter|enemy_ota|ota_sd_diagnostics) extras+=(tests/update_fake/cJSON.c firmware/RPG_POKET_2/UpdateInstall.cpp -Itests/update_fake);;
   settings_adapter|boot_art_diagnostics) extras+=(-Itests/settings_fake);;
   sd_adapter) extras+=(-Itests/sd_fake);;
   shared_spi_startup) extras+=(-Itests/shared_spi_fake);;
   ota_verify_render|ota_sd_diagnostics_render) args+=(build/screens/ota77);;
  esac
  echo "RUN $pass $name"
  g++ -std=c++17 -UNDEBUG "${flags[@]}" "tests/$name.cpp" "${extras[@]}" -o "build/tests/$name"
  ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/tests/$name" "${args[@]}"
 done
done

# Loader policy changed: retain old files, still reject mismatched catalogs.
python3 tools/test_enemy_transition.py
python3 tools/test_enemy_transition.py --sanitizers
