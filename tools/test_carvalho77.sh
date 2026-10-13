#!/usr/bin/env bash
set -euo pipefail
python3 tools/check_controller_snapshot.py
# No rules/save/OTA/art/other city/travel renderer changes in this candidate.
git diff --exit-code 91c64a364a7c1c697b9cd91b7e72027fa03f2dee -- firmware/RPG_POKET_2/Rules.h firmware/RPG_POKET_2/Save.h firmware/RPG_POKET_2/FatigueJournal.h firmware/RPG_POKET_2/UpdateDevice.h firmware/RPG_POKET_2/DeviceSettings.h firmware/RPG_POKET_2/AssetStore.h firmware/RPG_POKET_2/AssetCatalog.h firmware/RPG_POKET_2/DungeonView.h firmware/RPG_POKET_2/LaunchView.h firmware/RPG_POKET_2/World.h firmware/RPG_POKET_2/BagView.h firmware/RPG_POKET_2/BagSkinData.h firmware/RPG_POKET_2/ReleaseVersion.h .github/workflows
python3 - <<'CHECK'
from pathlib import Path
import subprocess
p='firmware/RPG_POKET_2/RPG_POKET_2.ino'
a=subprocess.check_output(['git','show','91c64a364a7c1c697b9cd91b7e72027fa03f2dee:'+p],text=True)
b=Path(p).read_text()
old=next(l for l in a.splitlines() if 'if(menu.journey.tick(now))' in l)
new=next(l for l in b.splitlines() if 'if(menu.journey.tick(now))' in l)
assert new.replace('view.cityPage=0;','')==old
print('PASS: arrival loop differs only by transient page reset (source guard, not loop execution)')
CHECK
python3 tools/unpack_art.py
mkdir -p build/tests build/screens/carvalho77
for pass in normal sanitizer; do
 if [[ "$pass" == normal ]]; then names='carvalho77_controller carvalho77_render touch77_controller touch77_render title_controller dungeon_controller travel_economy'; flags=(-O2); else names='carvalho77_controller carvalho77_render'; flags=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie); fi
 for name in $names; do
  echo "RUN $pass $name"
  g++ -std=c++17 -UNDEBUG "${flags[@]}" "tests/$name.cpp" -o "build/tests/${name}_$pass"
  args=(); if [[ "$name" == *_render ]]; then args+=(build/screens/carvalho77); fi
  ASAN_OPTIONS=detect_leaks=${RPG_DETECT_LEAKS:-1} UBSAN_OPTIONS=halt_on_error=1 "build/tests/${name}_$pass" "${args[@]}"
 done
done
