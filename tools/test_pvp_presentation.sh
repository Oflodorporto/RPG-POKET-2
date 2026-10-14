#!/usr/bin/env bash
# Focused local-only checks. Does not run a release/CI aggregate or publish.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"; cd "$root"
art="${1:?Pass the validated tracked artes.pak path}"
out="${2:-$root/build/pvp-presentation}"
mkdir -p "$out/screens" "$out/sanitized"
flags=(-std=c++17 -UNDEBUG -Itests/club_fake -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc)
sources=(tests/pvp_presentation_render.cpp firmware/RPG_POKET_2/ClubDuel.cpp firmware/RPG_POKET_2/ClubProtocol.cpp)
g++ "${flags[@]}" -O2 "${sources[@]}" -o "$out/pvp_presentation_render" 2>"$out/build.log"
"$out/pvp_presentation_render" "$art" "$out/screens" | tee "$out/run.log"
g++ "${flags[@]}" -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie "${sources[@]}" -o "$out/pvp_presentation_render_san" 2>"$out/sanitizer-build.log"
ASAN_OPTIONS=detect_leaks="${RPG_DETECT_LEAKS:-0}" UBSAN_OPTIONS=halt_on_error=1 "$out/pvp_presentation_render_san" "$art" "$out/sanitized" | tee "$out/sanitizer-run.log"
python3 tools/capture_pvp_presentation.py "$out"
