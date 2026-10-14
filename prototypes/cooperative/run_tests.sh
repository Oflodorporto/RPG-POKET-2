#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
out="${1:-../../build/coop-prototype}"
mkdir -p "$out"
flags=(-std=c++17 -Wall -Wextra -Wpedantic -Werror -Wno-misleading-indentation -Wno-sign-compare)
# The two warning exemptions originate in the read-only production Rules.h style.
g++ "${flags[@]}" -O2 tests.cpp -o "$out/coop-tests-opt"
"$out/coop-tests-opt" "$out/turn-trace.jsonl" | tee "$out/optimized.log"
g++ "${flags[@]}" -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined tests.cpp -o "$out/coop-tests-asan-ubsan"
# LeakSanitizer cannot run under this executor ptrace; C++ allocation counter remains enabled.
ASAN_OPTIONS=detect_leaks=0:strict_string_checks=1 UBSAN_OPTIONS=halt_on_error=1 "$out/coop-tests-asan-ubsan" | tee "$out/sanitizers.log"
g++ "${flags[@]}" -O2 review_tests.cpp -o "$out/coop-review-opt"
"$out/coop-review-opt" | tee "$out/review-optimized.log"
g++ "${flags[@]}" -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined review_tests.cpp -o "$out/coop-review-asan-ubsan"
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$out/coop-review-asan-ubsan" | tee "$out/review-sanitizers.log"
