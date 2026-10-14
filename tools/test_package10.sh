#!/usr/bin/env bash
set -euo pipefail
# The CI entrypoint always runs the complete checkpointed inherited+package10 plan.
# Local focused iteration calls individual packaging/verifier/OTA tools instead.
test "$#" = 0
export RPG_DETECT_LEAKS="${RPG_DETECT_LEAKS:-1}"
test "$RPG_DETECT_LEAKS" = 1
export PYTHONDONTWRITEBYTECODE=1
mkdir -p build
python3 tools/unpack_art.py
python3 tools/check_package10_scope.py
python3 tests/package10_qualification_config.py
python3 -c 'import sys; sys.path.insert(0,"tools"); from test_package10_release import published_lineage; published_lineage()'
python3 tests/package10_release_gate.py
python3 -u tools/test_package10_local.py --run-id "ci-${GITHUB_SHA:-local-$(date +%s)}" --jobs 3
python3 tests/package10_release_verifier.py --synthetic
python3 tools/test_package10_release.py --synthetic-fixture
python3 tools/test_package10_release.py --synthetic-fixture --sanitizers
python3 tools/capture_d20_camp77.py
python3 tools/capture_camp_identity77.py
python3 tools/capture_ruins_bag77.py
python3 tools/capture_ruins7.py
python3 tools/capture_camp_meal7.py
python3 tools/capture_clock_cycle7.py
python3 tools/capture_clock_visibility7.py
python3 tools/capture_narrative8.py
python3 tools/capture_pvp_presentation.py build/pvp-presentation
test -s build/narrative8-render/index.html
python3 -c 'import json; from pathlib import Path; d=json.loads(Path("build/narrative8-render/summary.json").read_text()); assert d["rendered_frames"] == 3481 and d["unique_previews"] == 671'
for image in \
  build/ruins7/ruins7-two-pages-medieval-render.png \
  build/ruins7/ruins7-two-pages-night-render.png \
  build/screens/camp-identity77/setup-all-identities.png \
  build/screens/camp-identity77/rest-all-identities.png \
  build/screens/camp-identity77/setup-meal-four-identities.gif \
  build/screens/camp-identity77/rest-meal-four-identities.gif \
  build/clock-cycle7/previews/clock-local-phases.png \
  build/clock-cycle7/previews/clock-day-realtime.gif \
  build/clock-cycle7/previews/clock-night-realtime.gif \
  build/d20-camp77/previews/d20-rolling.gif; do test -s "$image"; done
for folder in build/fatigue7/screens build/screens/procedural6 build/screens/package6-integration; do
  test -n "$(find "$folder" -type f -name '*.ppm' -print -quit)"
done
python3 tools/check_package10_scope.py
printf '%s\n' 'PASS host qualification and actual-render evidence. ESP32 compile and real-byte OTA gates remain mandatory; no device validation or publication implied.'
