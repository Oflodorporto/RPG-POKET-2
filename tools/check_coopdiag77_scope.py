#!/usr/bin/env python3
"""Exact docs-main plus reviewed radio diagnostics and recovery, fail closed.

Historical package12/SHOP/PvP gates and inventories remain immutable. This
new scope permits only the listed reviewed source delta and new release files.
The reviewed radio implementation is pinned independently of the full inventory.
"""
from pathlib import Path
import hashlib,json,os,re,struct,subprocess,sys,zlib
sys.dont_write_bytecode=True
from check_package12_scope import (assert_rules_compatibility, assert_generated_inventory,
    production_inventory, CONTRACT as HISTORICAL_CONTRACT, BASELINE as SHOP_BASELINE)
ROOT=Path(__file__).resolve().parents[1]
BASELINE='c8738eb67bde3817feaab9f3a53c30607396a5f5'
BASELINE_TREE='ba63078c4c1273844cd5fdbf6b6651c0270a6d9a'
COOP_COMMIT='b688df55d8137ccdd9d70bd491579c309c67c5ad'
COOP_TREE='8aff875e2d54967744784fd390ddae0e45c60047'
INVENTORY_SHA='6ce6152214643504d2ea2832b7bf6f841caef094efce4ab7466d9489124823f6'
PACKAGING={'tests/coopdiag77_release_gate.py', 'docs/COOPDIAG77_RELEASE_QUALIFICATION.md', 'docs/RELEASE_COOPDIAG77_NOTAS.txt', '.github/workflows/coopdiag77-check.yml', 'tools/verify_coopdiag77_release.py', 'tests/fixtures/coopdiag77-published-baselines.json', 'tools/test_coopdiag77_release.py', 'tests/fixtures/coopdiag77-approved-sources.json', 'tests/coopdiag77_scope.py', 'tools/release_coopdiag77_manifest.py', 'tests/coopdiag77_build_evidence.py', '.github/workflows/release.yml', 'tools/test_coopdiag77_local.py', 'tests/coopdiag77_qualification_config.py', 'tools/check_coopdiag77_release_gate.py', 'tests/coopdiag77_release_verifier.py', 'tests/coopdiag77_release.cpp', 'tools/test_coopdiag77.sh', 'tools/check_coopdiag77_scope.py', 'COOPDIAG77_STAGING_ONLY.txt', 'tools/record_coopdiag77_identity.py'}
REVIEWED_PRODUCTION={'firmware/RPG_POKET_2/Coop/CoopDiagnostics.h': 'a3db67429d0e693451d0bbf001c0580acd55df57cb2e5255127feba6de17dd61', 'firmware/RPG_POKET_2/Coop/CoopEndpoint.h': '5ab9489a3e71109e3fca22d97235993e80bb5827067d0e09b9141b9285fd4e7e', 'firmware/RPG_POKET_2/Coop/CoopInput.h': '15eb9ce9dd0ed1c6fc88130d000386938dfbfaca6b7b4c0d30b40a8888715181', 'firmware/RPG_POKET_2/Coop/CoopRadioDevice.h': '6b99630d294be9165255956168873d70e74e73f4a0c4ca4b816a3037ab0a8699', 'firmware/RPG_POKET_2/Coop/CoopSession.h': '02a65caf47dd2f39d0b2201767428b748898d94b0ba883f0b4c33528a39d8cb1', 'firmware/RPG_POKET_2/Coop/CoopTraining.h': '1bef07d7911f6afe8340b13ecc2a391d5493d2c755658a300e9eeb76537abeb0', 'firmware/RPG_POKET_2/Coop/CoopView.h': '6d4e476c05480b4d8b6c33cd9ab016e5cdb137758ae7c5810fc950aa5d902044', 'firmware/RPG_POKET_2/Coop/CoopViewModel.h': '31fa45e2c353366eb7223e473c355eac8ff2344340dbd840e33d127ff67165fc', 'firmware/RPG_POKET_2/CoopRuntime.cpp': '3f4f79f58cfaa628945bae07e4181a024f6c21ce50640c7c93d3aa4791c2d4fe', 'firmware/RPG_POKET_2/CoopUi.h': '090b97dfa83d0702bc9bec7911e0ad45f87a70326ea8bf0a3d84a26b24d0ef59', 'firmware/RPG_POKET_2/View.h': '3b0ac7b95da34a78dc6237eead83833d51ffb5d1acb461421c68b2962f9d7509'}
REVIEWED_CHANGES={'tests/coop_radio_diagnostics_device.cpp', 'tools/test_coop_radio_diagnostics_device.py', 'tests/coop_fake/esp_wifi.h', 'tests/coop_radio_recovery_regression.cpp', 'firmware/RPG_POKET_2/Coop/CoopTraining.h', 'tools/capture_coop_radio_diagnostics.py', 'tests/coop_radio_diagnostics_render.cpp', 'firmware/RPG_POKET_2/Coop/CoopRadioDevice.h', 'firmware/RPG_POKET_2/CoopUi.h', 'tests/coop_touch_queue_regression.cpp', 'tests/coop_radio_runtime_ui.cpp', 'firmware/RPG_POKET_2/Coop/CoopEndpoint.h', 'tests/coop_fake/esp_now.h', 'tools/test_coop_radio_runtime_ui.py', 'firmware/RPG_POKET_2/ReleaseVersion.h', 'firmware/RPG_POKET_2/Coop/CoopView.h', 'firmware/RPG_POKET_2/Coop/CoopSession.h', 'firmware/RPG_POKET_2/View.h', 'tests/coop_fake/esp_timer.h', 'firmware/RPG_POKET_2/Coop/CoopDiagnostics.h', 'firmware/RPG_POKET_2/Coop/CoopInput.h', 'tools/test_coop_radio_recovery_regression.py', 'tests/coop_fake/FakeCore.h', 'firmware/RPG_POKET_2/CoopRuntime.cpp', 'firmware/RPG_POKET_2/Coop/CoopViewModel.h'}
GENERATED={'release/artes.pak','release/firmware.bin','release/SHA256SUMS','release/manifest.json'}
CONTRACT={**HISTORICAL_CONTRACT,'version':'2026.10.15-coopdiag77','build':2026101502,
          'package':'7.7 - Treino cooperativo: sinal e diagnostico','coop_admission_version':2,
          'coop_accept_timeout_ms':30000,'coop_technical_timeout_ms':12000,'qualification_groups':157}
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT)
def validate_files(root,entries):
 assert isinstance(entries,list), 'Frozen inventory must be a list'
 names=[]
 for e in entries:
  assert isinstance(e,dict) and set(e)=={'path','bytes','sha256','executable'}, 'Invalid frozen source record'
  name=e['path']
  assert isinstance(name,str) and name and not Path(name).is_absolute() and '..' not in Path(name).parts
  assert str(Path(name))==name and '\\' not in name and '\x00' not in name, 'Noncanonical source path'
  assert type(e['bytes']) is int and e['bytes']>=0, 'Invalid frozen source size'
  assert isinstance(e['sha256'],str) and re.fullmatch('[0-9a-f]{64}',e['sha256']), 'Invalid frozen source hash'
  assert type(e['executable']) is bool, 'Invalid frozen source mode'
  assert name not in PACKAGING, 'Packaging may not hide in frozen inventory'
  names.append(name)
 assert len(names)==len(set(names)), 'Duplicate frozen path'
 for e in entries:
  name=e['path'];p=root/name;assert p.is_file() and not p.is_symlink(),name
  assert not any(parent.is_symlink() for parent in p.parents if parent!=root and root in parent.parents),name
  data=p.read_bytes();assert len(data)==e['bytes'] and hashlib.sha256(data).hexdigest()==e['sha256'],'Frozen source changed: '+name
  assert bool(p.stat().st_mode&0o111)==e['executable'],'Source mode changed: '+name

def validate_ancestry(head,parents,ci=False,ci_sha=None):
 assert re.fullmatch('[0-9a-f]{40}',head), 'Invalid source commit'
 if head!=BASELINE:
  assert parents==[head,BASELINE], 'Candidate must be sole child of exact docs-main lease'
 if ci:
  assert head!=BASELINE and head==ci_sha, 'CI must qualify the exact candidate commit'

def validate_frozen(frozen):
 assert set(frozen)=={'schema','baseline_commit','baseline_tree','coop_commit','coop_tree','packaging_exceptions','reviewed_changes','reviewed_production','contract','frozen_files'}
 assert type(frozen['schema']) is int and frozen['schema']==1
 assert frozen['baseline_commit']==BASELINE and frozen['baseline_tree']==BASELINE_TREE
 assert frozen['coop_commit']==COOP_COMMIT and frozen['coop_tree']==COOP_TREE
 assert frozen['packaging_exceptions']==sorted(PACKAGING) and frozen['reviewed_changes']==sorted(REVIEWED_CHANGES)
 assert frozen['reviewed_production']==REVIEWED_PRODUCTION, 'Reviewed production manifest changed'
 assert frozen['contract']==CONTRACT
 entries=frozen['frozen_files']
 assert hashlib.sha256(json.dumps(entries,sort_keys=True,separators=(',',':')).encode()).hexdigest()==INVENTORY_SHA,'Reviewed source inventory changed'
 return entries

def validate_paths(root,entries,tracked,actual):
 expected={e['path'] for e in entries}|PACKAGING
 assert_generated_inventory(root,tracked)
 actual=set(actual)
 assert not actual-expected-GENERATED,'Unexpected source paths: '+repr(sorted(actual-expected-GENERATED))
 assert expected<=actual,'Missing source paths: '+repr(sorted(expected-actual))
 for name in PACKAGING:
  p=root/name;assert p.is_file() and not p.is_symlink(),name
  assert not any(parent.is_symlink() for parent in p.parents if parent!=root and root in parent.parents),name
 return expected

def validate_reviewed_production(root,entries):
 indexed={e['path']:e for e in entries}
 assert len(REVIEWED_PRODUCTION)==11 and set(REVIEWED_PRODUCTION)<REVIEWED_CHANGES
 assert {p for p in REVIEWED_CHANGES if p.startswith('firmware/')}==set(REVIEWED_PRODUCTION)|{'firmware/RPG_POKET_2/ReleaseVersion.h'}
 for name,digest in REVIEWED_PRODUCTION.items():
  assert indexed[name]['sha256']==digest, 'Frozen inventory differs from reviewed radio implementation: '+name
  assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest, 'Reviewed radio implementation changed: '+name

def main():
 assert git('rev-parse',BASELINE+'^{tree}').decode().strip()==BASELINE_TREE
 assert git('rev-parse',COOP_COMMIT+'^{tree}').decode().strip()==COOP_TREE
 head=git('rev-parse','HEAD').decode().strip()
 validate_ancestry(head,git('rev-list','--parents','-n','1','HEAD').decode().split(),
                   os.environ.get('GITHUB_ACTIONS')=='true',os.environ.get('GITHUB_SHA'))
 frozen=json.loads((ROOT/'tests/fixtures/coopdiag77-approved-sources.json').read_text())
 entries=validate_frozen(frozen)
 validate_files(ROOT,entries)
 validate_reviewed_production(ROOT,entries)
 tracked=set(git('ls-files','--cached','-z').decode().split('\0'))-{''}
 actual=set(git('ls-files','--cached','--others','--exclude-standard','-z').decode().split('\0'))-{''}
 expected=validate_paths(ROOT,entries,tracked,actual)
 # Every pre-existing source, documentation/media file and old qualification
 # gate is byte/mode identical to the approved main except explicit deltas.
 baseline=git('ls-tree','-r',BASELINE).decode().splitlines()
 for line in baseline:
  meta,name=line.split('\t',1);mode,kind,oid=meta.split()
  assert kind=='blob' and mode in ('100644','100755'),name
  assert name in expected, 'Baseline path removed: '+name
  if name in REVIEWED_CHANGES or name in PACKAGING:continue
  p=ROOT/name;assert p.read_bytes()==git('cat-file','blob',oid),'Historical source changed: '+name
  assert bool(p.stat().st_mode&0o111)==(mode=='100755'),'Historical source mode changed: '+name
 assert expected-{line.split('\t',1)[1] for line in baseline} <= PACKAGING|REVIEWED_CHANGES
 production=production_inventory(ROOT,expected)
 header=(ROOT/'firmware/RPG_POKET_2/ReleaseVersion.h').read_text()
 assert header=='#pragma once\nconstexpr const char* FW_VERSION="2026.10.15-coopdiag77";\nconstexpr uint32_t FW_BUILD=2026101502u;\nconstexpr const char* FW_PACKAGE="7.7 - Treino cooperativo: sinal e diagnostico";\n'
 for name in ('README.md','docs/ESTADO_ATUAL.md'):
  assert (ROOT/name).read_bytes()==git('show',BASELINE+':'+name),'Preserve published coopfix77 documentation until verified publication'
 # Every production file present in SHOP remains byte-identical except the
 # reviewed integration hooks, isolated updater body, release identity and
 # exactly two equivalent Rules returns pinned independently above.
 rules_path='firmware/RPG_POKET_2/Rules.h'
 assert_rules_compatibility(git('show',SHOP_BASELINE+':'+rules_path),(ROOT/rules_path).read_bytes())
 changed={'ClubRadio.h','RPG_POKET_2.ino','View.h','UpdateDevice.h','ReleaseVersion.h','Rules.h'}
 for path in git('ls-tree','-r','--name-only',SHOP_BASELINE,'firmware').decode().splitlines():
  if path.removeprefix('firmware/RPG_POKET_2/') not in changed:
   assert (ROOT/path).read_bytes()==git('show',SHOP_BASELINE+':'+path),'SHOP production regression: '+path
 assert 'constexpr uint8_t kVersion=2,kMode=0x54;' in (ROOT/'firmware/RPG_POKET_2/Coop/CoopAdmission.h').read_text()
 session=(ROOT/'firmware/RPG_POKET_2/Coop/CoopSession.h').read_text()
 assert 'kAcceptTimeoutMs=30000' in session and 'kAdmissionTimeoutMs=12000,kLinkTimeoutMs=10000' in session
 assert 'PERSISTENT_SAVE_FORMAT=30' in (ROOT/'firmware/RPG_POKET_2/Persistence.h').read_text()
 assert 'PERSISTENT_SAVE_SIZE=160' in (ROOT/'firmware/RPG_POKET_2/Persistence.h').read_text()
 assert 'slot>2' in (ROOT/'firmware/RPG_POKET_2/Persistence.h').read_text()
 assert 'static const char* k[3][2]' in (ROOT/'firmware/RPG_POKET_2/Persistence.h').read_text()
 assert 'RATION_LIMIT=10' in (ROOT/'firmware/RPG_POKET_2/Rules.h').read_text()
 view=(ROOT/'firmware/RPG_POKET_2/View.h').read_text()
 assert 'LevelUp, PurchaseReceipt, CoopTraining };' in view,'Existing page indices must not shift'
 sketch=(ROOT/'firmware/RPG_POKET_2/RPG_POKET_2.ino').read_text();loop=sketch.split('void loop(){',1)[1]
 assert loop.index('if(coopIsOpen())')<loop.index('if(tickUpdater())')
 assert 'delay(1);return;' in loop[:loop.index('if(tickUpdater())')]
 assert loop.index('if(pendingTouch)')<loop.index('if(coopIsOpen()){delay(1);return;}')<loop.index('rpg::offerEvent(')
 assert 'button(124,218,102,"Coop treino")' in view and 'hit(x,y,124,218,102)' in sketch
 subprocess.run(['python3',str(ROOT/'tests/coop_update_install_equivalence.py')],cwd=ROOT,check=True)
 pack=(ROOT/'cartao/RPGPOKET/artes.pak').read_bytes()
 assert len(pack)==CONTRACT['art_total_bytes'] and hashlib.sha256(pack).hexdigest()==CONTRACT['art_sha256']
 assert struct.unpack('<4sIII',pack[:16])==(b'PKA1',7470912,0x9b4bcfc0,666) and zlib.crc32(pack[16:])==0x9b4bcfc0
 atlas=(ROOT/'firmware/RPG_POKET_2/Ruins7Art.h').read_text()
 atlas_bytes=sum(len(values.replace('\n','').split(','))*(2 if typ=='uint16_t' else 1) for typ,values in re.findall(r'inline constexpr (uint\d+_t) \w+\[[^\]]+\]=\{([^}]+)\};',atlas));assert atlas_bytes==18144
 protocol=(ROOT/'firmware/RPG_POKET_2/ClubProtocol.h').read_text();assert 'constexpr uint8_t VERSION = 5;' in protocol and 'static_assert(sizeof(Packet) == 52' in protocol
 alloc=re.compile(r'\b(?:malloc|calloc|realloc|ps_malloc|ps_calloc|heap_caps_malloc|heap_caps_calloc|heap_caps_realloc)\s*\(')
 before=sum(len(alloc.findall(git('show',SHOP_BASELINE+':'+n).decode())) for n in git('ls-tree','-r','--name-only',SHOP_BASELINE,'firmware').decode().splitlines())
 after=sum(len(alloc.findall((ROOT/n).read_text())) for n in production);assert after==before,'Unexpected production allocator call'
 for path in list((ROOT/'firmware/RPG_POKET_2/Coop').glob('*.h'))+[ROOT/'firmware/RPG_POKET_2/CoopDevice.h',ROOT/'firmware/RPG_POKET_2/CoopRuntime.cpp']:
  live='\n'.join(line.split('//',1)[0] for line in path.read_text().splitlines())
  for forbidden in ['journal.save(','savedTransition(','rpg::finish(','rpg::reserveFight(','rpg::settleFight(','settings.put','heap_caps_malloc(','new uint','setRotation(']:assert forbidden not in live,(path,forbidden)
 report=dict(schema=1,source_commit=head,source_tree=git('rev-parse','HEAD^{tree}').decode().strip(),baseline_commit=BASELINE,baseline_tree=BASELINE_TREE,coop_commit=COOP_COMMIT,frozen_inventory_sha256=INVENTORY_SHA,frozen_files=len(entries),packaging_exceptions=sorted(PACKAGING),reviewed_changes=sorted(REVIEWED_CHANGES),reviewed_production=REVIEWED_PRODUCTION,contract=CONTRACT,new_art_payload_bytes=0,new_atlas_flash_bytes=0,new_psram_allocations=0,framebuffer_bytes=153600,decode_workspace_bytes=13760,known_psram_bytes=7638272,candidate_firmware_bytes=None,target_compile_verified=False,physical_validation=False,published=False,new_reset=False,experimental_training=True,note='Native qualification does not establish RF cause or physical-board behavior. Save30/160, readers29+30, epoch1, rations10 and all frozen SHOP/PvP behavior preserved; default Arduino ON and explicit OFF required.')
 (ROOT/'build').mkdir(exist_ok=True);(ROOT/'build/coopdiag77-scope-budget.json').write_text(json.dumps(report,indent=2)+'\n')
 print('PASS: exact docs-main plus reviewed radio diagnostics/recovery inventory; old gates/docs frozen; Save30/160/read29+30/epoch1/3slots/rations10, art666, no extra allocator calls; known PSRAM7638272; no physical qualification implied.')
if __name__=='__main__':main()
