#!/usr/bin/env bash
set -euo pipefail
python3 tools/prepare_touch_skin.py --check
# Published dungeon rendering, rules, savings, and all storage paths remain exact.
git diff --exit-code 59258cec67458e2d05d4db6a6c7f7656eee1959c -- firmware/RPG_POKET_2/DungeonView.h firmware/RPG_POKET_2/CombatPresentation.h firmware/RPG_POKET_2/TouchGate.h tools/test_dungeon77.sh
python3 - <<'PY'
from pathlib import Path
import subprocess
p='firmware/RPG_POKET_2/RPG_POKET_2.ino'
a=subprocess.check_output(['git','show','59258cec67458e2d05d4db6a6c7f7656eee1959c:'+p],text=True)
b=Path(p).read_text()
for s in (a,b):assert 'void acknowledgeDungeonResult(){' in s
assert a.split('void acknowledgeDungeonResult(){')[1].split('void tapped(')[0]==b.split('void acknowledgeDungeonResult(){')[1].split('void tapped(')[0]
print('PASS: published dungeon acknowledgement remains byte-identical')
PY
bash tools/test_dungeon77.sh
mkdir -p build/tests build/screens/touch77
for pass in normal sanitizer; do
 if [[ "$pass" == normal ]]; then names='touch77_controller touch77_render gear_save'; flags=(-O2); else names='touch77_controller touch77_render'; flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); fi
 for name in $names; do
  echo "RUN $pass $name"
  g++ -std=c++17 -UNDEBUG "${flags[@]}" "tests/$name.cpp" -o "build/tests/${name}_$pass"
  args=(); if [[ "$name" == touch77_render ]]; then args+=(build/screens/touch77); fi
  ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/tests/${name}_$pass" "${args[@]}"
 done
done
