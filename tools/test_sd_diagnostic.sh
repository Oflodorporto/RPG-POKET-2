#!/usr/bin/env bash
set -euo pipefail
mkdir -p build/sd_diagnostic
python3 tools/sd_diagnostic/generate_fixture.py
python3 tools/prepare_enemy_art.py >/dev/null
c++ -std=c++17 -O2 -Wall -Wextra -Werror tests/sd_diagnostic.cpp -lcrypto -o build/sd_diagnostic/test
build/sd_diagnostic/test cartao/RPGPOKET/artes.pak tools/sd_diagnostic/fixtures/transfer_fixture.bin
c++ -std=c++17 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined tests/sd_diagnostic.cpp -lcrypto -o build/sd_diagnostic/test_san
ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-0} build/sd_diagnostic/test_san cartao/RPGPOKET/artes.pak tools/sd_diagnostic/fixtures/transfer_fixture.bin
python3 tests/sd_diagnostic_package.py
