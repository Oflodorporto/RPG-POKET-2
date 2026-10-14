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
# Exact reviewed translation-unit extraction after CI38621515603 linked flag0
# but hit Xtensa l32r reach limits with the monolithic flag1 sketch object.
RUNTIME_SPLIT_PATCH={'firmware/RPG_POKET_2/Coop/CoopEligibilityBridge.h': '63ce4d1c78dbba232ac927f497a3e4fa7bf0a5943eeaf44c34009cd75cfb7c34', 'firmware/RPG_POKET_2/Coop/CoopInput.h': '914e8cf82b16d57e20a7e9167740a07f810bb91e00e91f5397264332379e1f85', 'firmware/RPG_POKET_2/Coop/CoopPortraitBridge.h': 'f5299173a7b502726e91b7ee3ef5e12a3aea0184970c2c6d2f9fd7e65a459ec7', 'firmware/RPG_POKET_2/Coop/CoopTouchSampler.h': '97f6eb4e7234d39d50c8717e01196c66d5cee619080c2b122899594f5b84bc85', 'firmware/RPG_POKET_2/Coop/CoopTouchUi.h': 'b911b648dfa75c170ef365ce71425ac8475941ce5cad82833ff6d9970a955821', 'firmware/RPG_POKET_2/Coop/CoopTraining.h': 'f504fbdb19f65b232037e1da919f245602fe7a27840d885a66fdec497c86810e', 'firmware/RPG_POKET_2/Coop/CoopView.h': '564e8b909c3a2dfbe083a45b6f0a3882ca94eddeabf7fab3416e38a07b75b330', 'firmware/RPG_POKET_2/Coop/CoopViewModel.h': '43e942588a0fdb21e0842ce56ae76acfb76c9d4172d8a23068e2ac81113ba4e7', 'firmware/RPG_POKET_2/CoopDevice.h': 'bc2f339d068a5813e7bcfa849df979967d7047c2a914217dcbb6e13542dcc6de', 'firmware/RPG_POKET_2/CoopRuntime.cpp': 'd52bceed41cfde3ba032938af0690da9b3eb2ae23674bc7d2d10b073e4bb21db', 'firmware/RPG_POKET_2/CoopUi.h': 'd36e6d4ba86e14d7242376682f4632fa5e315242328117e31afb42c6b0631bd6', 'tests/coop_training_device.cpp': 'c7523a4f1c96779837efec926e44be6d9073ae60a7ba55fadf5cc96a6bac6a0d', 'tests/coop_training_input.cpp': '6b657212025c23d5eee07a5b54ca8d9f2436ac67402d628a1230f651b1ad5e92', 'tests/coop_training_radio.cpp': '7baef828bf83c303a0c54e86656b594277e1df8500964ae892f5f47e71646261', 'tests/coop_training_render.cpp': '0bdc807891bb4f816540f776c98af778d213fb3142202295250df16668fc9b60', 'tests/coop_training_session.cpp': 'f84ace5053ae67aa053c0c4530fec269663637341ce58f4a47b95ab83378101e', 'tools/test_coop_training.sh': '077555e77f6027a6ba9dcf2aff2fb5c1248d26ba14ea3d445fa7f3f0dd629369'}
for name,digest in {**TARGET_COMPATIBILITY_PATCH,**RUNTIME_SPLIT_PATCH}.items():
 assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,('Target compatibility patch mismatch',name)
for line in git('diff',CANDIDATE,'--name-status').splitlines():
 status,path=line.split('\t',1)
 assert status in ('A','M') and path in ci_only|set(TARGET_COMPATIBILITY_PATCH)|set(RUNTIME_SPLIT_PATCH),line
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
runtime=(root/'firmware/RPG_POKET_2/CoopRuntime.cpp').read_text()
assert runtime.startswith('#include "CoopUi.h"\n#if RPG_ENABLE_COOP_TRAINING\n')
assert runtime.rstrip().endswith('#endif')
for header in ('View.h','MenuView.h','UpdateDevice.h','FrameBuffer.h'):
 assert '#include "'+header+'"' not in runtime
facade=(root/'firmware/RPG_POKET_2/CoopUi.h').read_text()
assert 'extern coopplatform::Session coopSession;' in facade
assert '#include "Coop/CoopSession.h"' not in facade
for path in list((root/'firmware/RPG_POKET_2/Coop').glob('*.h'))+[root/'firmware/RPG_POKET_2/CoopDevice.h',root/'firmware/RPG_POKET_2/CoopRuntime.cpp']:
 code=path.read_text()
 for forbidden in ['journal.save(', 'savedTransition(', 'rpg::finish(', 'rpg::reserveFight(', 'rpg::settleFight(', 'settings.put', 'heap_caps_malloc(', 'new uint', 'setRotation(']:
  # Inspect lines that are not whole-line comments; inline commentary is harmless.
  live='\n'.join(line.split('//',1)[0] for line in code.splitlines())
  assert forbidden not in live,(path,forbidden)
print('PASS: candidate-only branch/default-off, reviewed production frozen except exact synchronization/TU extraction fixes, only 3 existing integration files changed, campaign/PvP protocol/assets/old workflows frozen, isolated read-only qualification only, 19 prototype hashes, exclusive-loop entry/exit guards, copy-only callback, no campaign/write/allocation APIs')
