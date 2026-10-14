#!/usr/bin/env bash
# Reproduce only the local PvP reliability, profile and real-renderer checks.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"; cd "$root"
art="${1:?Pass the tracked artes.pak path}"
out="${2:-/tmp/rpg-pvp-candidate}"
mkdir -p "$out/appearance"
bash tools/test_pvp_reliability.sh "$out/reliability"
source=(tests/pvp_appearance_protocol.cpp firmware/RPG_POKET_2/ClubProtocol.cpp firmware/RPG_POKET_2/ClubDuel.cpp)
g++ -std=c++17 -O2 -UNDEBUG -Ifirmware/RPG_POKET_2 "${source[@]}" -o "$out/appearance/profiles"
"$out/appearance/profiles" | tee "$out/appearance/run.log"
g++ -std=c++17 -O1 -g -UNDEBUG -Ifirmware/RPG_POKET_2 -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie "${source[@]}" -o "$out/appearance/profiles_san"
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$out/appearance/profiles_san" | tee "$out/appearance/sanitizer-run.log"
bash tools/test_pvp_presentation.sh "$art" "$out/presentation"
echo 'PASS: 13 optimized targets and 12 ASan/UBSan targets; local focused checks only.'
