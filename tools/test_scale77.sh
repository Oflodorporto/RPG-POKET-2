#!/usr/bin/env bash
set -euo pipefail
# Full scenes77/Save29 aggregate plus presentation-only scale/rim regressions.
# Native/ASan/UBSan assertions do not replace the exact-commit ESP32 build or
# the owner's physical test. This script never publishes or updates a device.
bash tools/test_scenes77.sh
python3 tools/check_scale77_scope.py
