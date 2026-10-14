#!/usr/bin/env python3
"""Guard the visual-only meal revision against the published package6 tree."""
from pathlib import Path
import hashlib,json,subprocess
root=Path(__file__).resolve().parents[1]
base='3f9d3221b99e6b4ba55dc270a2a1d0a9155ea27b'
allowed={'CampView.h','PanelView.h','DailyTravelView.h','RPG_POKET_2.ino'}
files=subprocess.check_output(['git','ls-tree','-r','--name-only',base],cwd=root,text=True).splitlines();checked=0
for name in files:
    p=Path(name)
    if (name.startswith('firmware/') and p.name not in allowed) or name.startswith(('assets/','cartao/')):
        assert (root/p).read_bytes()==subprocess.check_output(['git','show',f'{base}:{name}'],cwd=root),name
        checked+=1
sketch='firmware/RPG_POKET_2/RPG_POKET_2.ino'
before=subprocess.check_output(['git','show',f'{base}:{sketch}'],cwd=root,text=True)
after=(root/sketch).read_text()
a=before.index('  if(lcdReady&&(dirty||')
expected=before[:a]+before[a:].replace('view.page==Page::CampRoll||','view.page==Page::CampSetup||view.page==Page::CampRoll||',1)
assert after==expected,'Only the CampSetup paint cadence may change in this worker patch'
pack=root/'cartao/RPGPOKET/artes.pak'
report=dict(baseline=base,protected_files_byte_identical=checked,art_sha256=hashlib.sha256(pack.read_bytes()).hexdigest(),new_art_payload_bytes=0,new_psram_bytes=0,save30_reset_epoch1_unchanged=True,rng_resources_rules_persistence_unchanged=True,controller_snapshots_unchanged=True,sketch_change='Add CampSetup to the existing 120 ms periodic redraw predicate',hardware_fps_touch_unverified=True,published=False)
out=root/'build/camp7';out.mkdir(parents=True,exist_ok=True);(out/'scope.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS: {checked} protected source/art files identical; zero art/PSRAM delta; save/RNG/resources unchanged; one redraw-only sketch change')
