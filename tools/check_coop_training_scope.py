#!/usr/bin/env python3
"""Read-only local candidate gate; never schedules CI or publishes."""
from pathlib import Path
import hashlib,json,subprocess
root=Path(__file__).resolve().parents[1]
def git(*args):return subprocess.check_output(['git','-C',str(root),*args],text=True)
assert git('branch','--show-current').strip()=='local/coop-training-firmware'
BASE='6a23dda54114db611b818b51e4b1b942019098fa'
allowed={'firmware/RPG_POKET_2/ClubRadio.h','firmware/RPG_POKET_2/RPG_POKET_2.ino','firmware/RPG_POKET_2/View.h'}
for line in git('diff',BASE,'--name-status').splitlines():
 status,path=line.split('\t',1)
 if status=='M':assert path in allowed,path
 else:assert status=='A' and path.startswith(('firmware/RPG_POKET_2/Coop','prototypes/','tests/coop_','tools/','docs/COOP_')),line
for prefix in ['assets','cartao','.github','firmware/RPG_POKET_2/Save.h','firmware/RPG_POKET_2/Persistence.h','firmware/RPG_POKET_2/Rules.h','firmware/RPG_POKET_2/ClubProtocol.h','firmware/RPG_POKET_2/ClubProtocol.cpp','firmware/RPG_POKET_2/ClubDuel.cpp']:
 assert not git('diff',BASE,'--',prefix),prefix
frozen=json.loads((root/'prototypes/cooperative_platform/FROZEN_INPUTS.json').read_text())
for entry in frozen['files']:
 assert hashlib.sha256((root/entry['path']).read_bytes()).hexdigest()==entry['sha256'],entry['path']
config=(root/'firmware/RPG_POKET_2/CoopConfig.h').read_text();assert '#define RPG_ENABLE_COOP_TRAINING 0' in config
sketch=(root/'firmware/RPG_POKET_2/RPG_POKET_2.ino').read_text();loop=sketch.split('void loop(){',1)[1]
assert loop.index('if(coopIsOpen())')<loop.index('if(tickUpdater())')
assert 'delay(1);return;' in loop[:loop.index('if(tickUpdater())')]
assert loop.index('if(coopIsOpen()){delay(1);return;}')>loop.index('if(pendingTouch)')
assert loop.index('if(coopIsOpen()){delay(1);return;}')<loop.index('rpg::offerEvent(')
view=(root/'firmware/RPG_POKET_2/View.h').read_text();render=view.split('template<class Canvas> void render(',1)[1]
assert render.index('Page::CoopTraining')<render.index('WifiOverlay<Canvas>')
assert 'button(124,218,102,"Coop treino")' in view and 'hit(x,y,124,218,102)' in sketch
radio=(root/'firmware/RPG_POKET_2/Coop/CoopRadioDevice.h').read_text();callback=radio.split('inline void receiveCallback(',1)[1].split('enum class OpenError',1)[0]
for forbidden in ['session->','training()','game','malloc','new ','esp_now_send']:assert forbidden not in callback,forbidden
for path in list((root/'firmware/RPG_POKET_2/Coop').glob('*.h'))+[root/'firmware/RPG_POKET_2/CoopDevice.h']:
 code=path.read_text()
 for forbidden in ['journal.save(', 'savedTransition(', 'rpg::finish(', 'rpg::reserveFight(', 'rpg::settleFight(', 'settings.put', 'heap_caps_malloc(', 'new uint', 'setRotation(']:
  # Inspect lines that are not whole-line comments; inline commentary is harmless.
  live='\n'.join(line.split('//',1)[0] for line in code.splitlines())
  assert forbidden not in live,(path,forbidden)
print('PASS: candidate-only branch/default-off, only 3 existing integration files changed, campaign/PvP protocol/assets/workflows frozen, 19 prototype hashes, exclusive-loop entry/exit guards, copy-only callback, no campaign/write/allocation APIs')
