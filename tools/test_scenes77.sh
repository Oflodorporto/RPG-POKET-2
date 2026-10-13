#!/usr/bin/env bash
set -euo pipefail
# Whole native regression plus current Save29/visual suites and sanitizers.
# Publication/ESP32 compilation remain separate exact-commit workflow gates.
bash tools/test_native.sh
python3 tools/prepare_clock_art.py --check
python3 tools/prepare_scene_art.py --check
python3 tests/scene_asset_catalog.py
