#!/usr/bin/env python3
"""Read-only local candidate gate; never schedules CI or publishes."""
from pathlib import Path
import hashlib,json,subprocess,sys,os
root=Path(__file__).resolve().parents[1]
def git(*args):return subprocess.check_output(['git','-C',str(root),*args],text=True)
branch=git('branch','--show-current').strip()
if '--ci' in sys.argv or not branch:
 assert os.environ.get('GITHUB_EVENT_NAME')=='push'
 assert os.environ.get('GITHUB_REF')=='refs/heads/feat/coop-training-check'
 assert os.environ.get('GITHUB_SHA')==git('rev-parse','HEAD').strip()
 assert not git('status','--porcelain','--untracked-files=all').strip(),'CI requires exact clean source'
else:assert branch in ('local/coop-training-firmware','feat/coop-training-check')
BASE='6a23dda54114db611b818b51e4b1b942019098fa'
CANDIDATE='b117da8bd46eb1cb3fa4e051654dc9e32c6f126e'
CANDIDATE_TREE='56a7bba85c83ba3584589502a81f3c69cde5dc01'
assert git('rev-parse',CANDIDATE+'^{tree}').strip()==CANDIDATE_TREE,'Reviewed source tree mismatch'
WORKFLOW='.github/workflows/coop-training-check.yml'
ci_only={WORKFLOW,'tools/check_coop_training_scope.py','tools/record_coop_training_build.py','tests/coop_training_qualification_config.py','tests/coop_training_build_evidence.py','docs/COOP_TRAINING_CI_QUALIFICATION.md'}
# Exact target-compatibility exception after CI38619347753 rejected the
# Xtensa std::atomic lock-free assumption. No other reviewed source may change.
TARGET_COMPATIBILITY_PATCH={'firmware/RPG_POKET_2/Coop/CoopRadioDevice.h': '2b3ee80d66ed1e0ab24990fc342a69c3bde2f5ca4a445e05e3909c35589925b7', 'tests/coop_fake/freertos/FreeRTOS.h': '662b4dd45c2113bdd4b93b3a68648dbdf49fb3b366e94135dc6e608d51fbcb15', 'tests/coop_training_radio.cpp': '57f3a513afd58ffce480c1d56bceb39d0fc214650772c5c424eadb5a6baa9466'}
for name,digest in TARGET_COMPATIBILITY_PATCH.items():
 assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,('Target compatibility patch mismatch',name)
for line in git('diff',CANDIDATE,'--name-status').splitlines():
 status,path=line.split('\t',1)
 assert status in ('A','M') and path in ci_only|set(TARGET_COMPATIBILITY_PATCH),line
allowed={'firmware/RPG_POKET_2/ClubRadio.h','firmware/RPG_POKET_2/RPG_POKET_2.ino','firmware/RPG_POKET_2/View.h'}
for line in git('diff',BASE,'--name-status').splitlines():
 status,path=line.split('\t',1)
 if status=='M':assert path in allowed,path
 else:assert status=='A' and (path==WORKFLOW or path.startswith(('firmware/RPG_POKET_2/Coop','prototypes/','tests/coop_','tools/','docs/COOP_'))),line
assert set(git('diff',BASE,'--name-only','--','.github').splitlines())<={WORKFLOW}
for prefix in ['assets','cartao','firmware/RPG_POKET_2/Save.h','firmware/RPG_POKET_2/Persistence.h','firmware/RPG_POKET_2/Rules.h','firmware/RPG_POKET_2/ReleaseVersion.h','firmware/RPG_POKET_2/ClubProtocol.h','firmware/RPG_POKET_2/ClubProtocol.cpp','firmware/RPG_POKET_2/ClubDuel.cpp']:
 assert not git('diff',BASE,'--',prefix),prefix
production=set(git('ls-files','--','firmware').splitlines())
for p in (root/'firmware').rglob('*'):
 assert not p.is_symlink(),p
 if p.is_file():assert str(p.relative_to(root)) in production,('Untracked/ignored production input',p)
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
print('PASS: candidate-only branch/default-off, reviewed production frozen except exact target synchronization fix, only 3 existing integration files changed, campaign/PvP protocol/assets/old workflows frozen, isolated read-only qualification only, 19 prototype hashes, exclusive-loop entry/exit guards, copy-only callback, no campaign/write/allocation APIs')
