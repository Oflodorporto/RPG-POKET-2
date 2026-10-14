#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
out="${1:-/tmp/coop-ui-targeting}"
mkdir -p "$out"
flags=(-std=c++17 -Wall -Wextra -Wpedantic -Werror -Wno-misleading-indentation -Wno-sign-compare)
g++ "${flags[@]}" -O2 tests.cpp -o "$out/ui-tests-opt"
"$out/ui-tests-opt" "$out/ui-trace.jsonl" | tee "$out/ui-optimized.log"
g++ "${flags[@]}" -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined tests.cpp -o "$out/ui-tests-sanitized"
ASAN_OPTIONS=detect_leaks=0:strict_string_checks=1 UBSAN_OPTIONS=halt_on_error=1 "$out/ui-tests-sanitized" | tee "$out/ui-sanitized.log"
g++ "${flags[@]}" -O2 review_tests.cpp -o "$out/review-tests-opt"
"$out/review-tests-opt" | tee "$out/review-optimized.log"
g++ "${flags[@]}" -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined review_tests.cpp -o "$out/review-tests-san"
ASAN_OPTIONS=detect_leaks=0:strict_string_checks=1 UBSAN_OPTIONS=halt_on_error=1 "$out/review-tests-san" | tee "$out/review-sanitized.log"
