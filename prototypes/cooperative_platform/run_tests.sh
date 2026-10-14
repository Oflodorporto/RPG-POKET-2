#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
out="${1:-/tmp/coop-platform-results}"
mkdir -p "$out"
flags=(-std=c++17 -Wall -Wextra -Wpedantic -Werror -Wno-misleading-indentation -Wno-sign-compare -Ifake)
g++ "${flags[@]}" -O2 tests.cpp -o "$out/platform-opt"
"$out/platform-opt" | tee "$out/optimized.log"
g++ "${flags[@]}" -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined tests.cpp -o "$out/platform-sanitized"
ASAN_OPTIONS=detect_leaks=0:strict_string_checks=1 UBSAN_OPTIONS=halt_on_error=1 "$out/platform-sanitized" | tee "$out/sanitizers.log"
