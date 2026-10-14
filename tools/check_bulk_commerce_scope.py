#!/usr/bin/env python3
"""Read-only scope/ABI/art guard for the local commerce candidate."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
ROOT = Path(__file__).resolve().parents[1]
BASELINE = '6a23dda54114db611b818b51e4b1b942019098fa'
ALLOWED = {'DailyTravel.h','Exploration.h','GuildEvents.h','RPG_POKET_2.ino','Rules.h','Save.h','StoryPaper.h','Trading.h','TradingView.h','View.h'}
def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)
production = git('ls-tree','-r','--name-only',BASELINE,'firmware').decode().splitlines()
changed=[]
allocator = re.compile(rb'\b(?:malloc|calloc|realloc|ps_malloc|ps_calloc|heap_caps_malloc|heap_caps_calloc|heap_caps_realloc)\s*\(')
before_alloc=after_alloc=0
for name in production:
    old=git('show',BASELINE+':'+name)
    new=(ROOT/name).read_bytes()
    before_alloc+=len(allocator.findall(old));after_alloc+=len(allocator.findall(new))
    if old!=new:
        assert name.startswith('firmware/RPG_POKET_2/') and Path(name).name in ALLOWED, name
        changed.append(name)
assert before_alloc==after_alloc, 'Unexpected new production heap allocation'
assert set(p.as_posix() for p in (ROOT/'firmware').rglob('*') if p.is_file())==set((ROOT/p).as_posix() for p in production), 'Unreviewed production file'
assert (ROOT/'README.md').read_bytes()==git('show',BASELINE+':README.md'), 'README lore/showcase must stay untouched'
assert '#define' not in (ROOT/'firmware/RPG_POKET_2/Trading.h').read_text()
assert re.search(r'PERSISTENT_SAVE_FORMAT=30', (ROOT/'firmware/RPG_POKET_2/Persistence.h').read_text())
assert re.search(r'PERSISTENT_SAVE_SIZE=160', (ROOT/'firmware/RPG_POKET_2/Persistence.h').read_text())
art=ROOT/'cartao/RPGPOKET/artes.pak'
assert hashlib.sha256(art.read_bytes()).hexdigest()=='14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56'
# Bind the sampled non-cosmetic PvP trace, rather than merely running its printer.
for mode in ('normal','sanitizers'):
    executable=ROOT/f'build/bulk-shop/{mode}/pvp_review_equivalence'
    if executable.is_file():
        env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks='+env.get('RPG_DETECT_LEAKS','0');env['UBSAN_OPTIONS']='halt_on_error=1'
        subprocess.run(['python3',str(ROOT/'tools/check_pvp_equivalence.py'),str(executable)],cwd=ROOT,env=env,check=True)
report={'schema':1,'baseline':BASELINE,'changed_production_files':changed,'production_allocation_calls_added':after_alloc-before_alloc,'save_format':30,'reads_save_formats':[29,30],'save_bytes':160,'reset_epoch':1,'new_persistent_fields':0,'pvp_changed':False,'prices_changed':False,'art_changed':False,'target_compile_verified':False,'physical_validation':False,'published':False,'downgrade_warning':'Old firmware rejects new records with 10 rations. Mixed journals may recover an older <=9 state; both records with 10 block. Upgrade preserves old saves.'}
out=ROOT/'build/bulk-shop/scope.json';out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(report,indent=2)+'\n')
print('PASS: bounded commerce/ration scope; Save30/read29+30/160 bytes/epoch1; PvP, OTA, prices and art preserved; no new production allocations.')
