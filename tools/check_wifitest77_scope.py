#!/usr/bin/env python3
"""Exact docs-main plus independently reviewed isolated Wi-Fi radio test, fail closed.

Historical package12/SHOP/PvP gates and inventories remain immutable. This
new scope permits only the listed reviewed source delta and new release files.
The reviewed radio implementation is pinned independently of the full inventory.
"""
if not __debug__:
    raise RuntimeError("Qualification requires assertions; optimized Python is forbidden")
from pathlib import Path
import hashlib,json,os,re,struct,subprocess,sys,zlib
sys.dont_write_bytecode=True
from check_package12_scope import (assert_rules_compatibility, assert_generated_inventory,
    production_inventory, CONTRACT as HISTORICAL_CONTRACT, BASELINE as SHOP_BASELINE)
ROOT=Path(__file__).resolve().parents[1]
BASELINE='4fc221f07b62d6bd26fcf03fa1bb12c8fd577111'
BASELINE_TREE='89c4252b249e144ced3382fd85e3476e122d3433'
COOP_COMMIT='6026fb49db4fe874b9c3f0571cbe4fdb9e66adb3'
COOP_TREE='5ad62cddfe052bea09bf99c29989731722088981'
# Populated only after independent review of the complete final source delta.
REVIEW_APPROVED=True
INVENTORY_SHA='e9313cd3f0fc7796ca478cb8e78c3f684bd966fbbacf581dce7ae15a8fa0eb35'
PACKAGING={'tests/wifitest77_optimization.py','tests/wifitest77_release_gate.py', 'docs/WIFITEST77_RELEASE_QUALIFICATION.md', 'docs/RELEASE_WIFITEST77_NOTAS.txt', '.github/workflows/wifitest77-check.yml', 'tools/verify_wifitest77_release.py', 'tests/fixtures/wifitest77-published-baselines.json', 'tools/test_wifitest77_release.py', 'tests/fixtures/wifitest77-approved-sources.json', 'tests/wifitest77_scope.py', 'tools/release_wifitest77_manifest.py', 'tests/wifitest77_build_evidence.py', '.github/workflows/release.yml', 'tools/test_wifitest77_local.py', 'tests/wifitest77_qualification_config.py', 'tools/check_wifitest77_release_gate.py', 'tests/wifitest77_release_verifier.py', 'tests/wifitest77_release.cpp', 'tools/test_wifitest77.sh', 'tools/check_wifitest77_scope.py', 'WIFITEST77_STAGING_ONLY.txt', 'tools/record_wifitest77_identity.py'}
REVIEWED_PRODUCTION={'firmware/RPG_POKET_2/Coop/RadioOwner.h': '3fdedd6e1c2ca1aa38df9ec6629d37a58a7da934b9876beea91a0f3917e3af2d',
 'firmware/RPG_POKET_2/MenuView.h': 'd32036cedb5d0d3f57f8c53587dae15a4600d2145bb013dd4d0d2b7853b76be5',
 'firmware/RPG_POKET_2/RPG_POKET_2.ino': 'ffbd8b2d0acd7d718906af3d72e9f8238b0ed1866afb5d325f3780869e708321',
 'firmware/RPG_POKET_2/View.h': '1666d3b07010d62c5987d06828bf2d9f547940a8cebdc68d7bc8dd82ccca6718',
 'firmware/RPG_POKET_2/WifiRadioTestConfig.h': '0162b009e2f90566e5155f66bc9721d6207a5040ead48c175f1a59328a3f503a',
 'firmware/RPG_POKET_2/WifiRadioTestCsv.h': '9693ed458a05f626232df8d8d19cd6bc1873005f0a617716fb8ef995d0264c9b',
 'firmware/RPG_POKET_2/WifiRadioTestDevice.h': '793b7c78f769a12ff6ae64dbd95093fe498b04ba904e310e3264194a9ecb1eeb',
 'firmware/RPG_POKET_2/WifiRadioTestExport.h': '0b6e73a0234bc1ff9ca4428b17afb0b53509105598bd67cf092a8f713717d270',
 'firmware/RPG_POKET_2/WifiRadioTestFlow.h': '3ed9a19e307d9584e7ccdc7853af5b5fc591da062e2db8b10b161f2cea88963c',
 'firmware/RPG_POKET_2/WifiRadioTestModel.h': '1c6e7aa4d73b7988c2948f7f0f5ad50d683b2ae5d3912c99c9a884fab5e221d8',
 'firmware/RPG_POKET_2/WifiRadioTestRuntime.h': '7eb57c7798102594b011e311604db985319af3e9f7856d72b88fd42257d143a3',
 'firmware/RPG_POKET_2/WifiRadioTestView.h': 'e21f89dddd205a8ed7d11d6f4aa8f34d5d3cd62afe275abc6c7e5cba9c7b9769'}
REVIEWED_CHANGES={'docs/WIFI_RADIO_TEST.md',
 'firmware/RPG_POKET_2/Coop/RadioOwner.h',
 'firmware/RPG_POKET_2/MenuView.h',
 'firmware/RPG_POKET_2/RPG_POKET_2.ino',
 'firmware/RPG_POKET_2/ReleaseVersion.h',
 'firmware/RPG_POKET_2/View.h',
 'firmware/RPG_POKET_2/WifiRadioTestConfig.h',
 'firmware/RPG_POKET_2/WifiRadioTestCsv.h',
 'firmware/RPG_POKET_2/WifiRadioTestDevice.h',
 'firmware/RPG_POKET_2/WifiRadioTestExport.h',
 'firmware/RPG_POKET_2/WifiRadioTestFlow.h',
 'firmware/RPG_POKET_2/WifiRadioTestModel.h',
 'firmware/RPG_POKET_2/WifiRadioTestRuntime.h',
 'firmware/RPG_POKET_2/WifiRadioTestView.h',
 'tests/WifiRadioRuntimeHarness.h',
 'tests/WifiRadioRuntimeNetwork.h',
 'tests/WifiRadioSimulation.h',
 'tests/dungeon_controller.cpp',
 'tests/title_controller.cpp',
 'tests/wifi_radio_device.cpp',
 'tests/wifi_radio_device_races.cpp',
 'tests/wifi_radio_fake/Arduino.h',
 'tests/wifi_radio_fake/FakeCore.h',
 'tests/wifi_radio_fake/WiFi.h',
 'tests/wifi_radio_fake/esp_now.h',
 'tests/wifi_radio_fake/esp_system.h',
 'tests/wifi_radio_fake/esp_timer.h',
 'tests/wifi_radio_fake/esp_wifi.h',
 'tests/wifi_radio_fake/freertos/FreeRTOS.h',
 'tests/wifi_radio_test_export.cpp',
 'tests/wifi_radio_test_flow.cpp',
 'tests/wifi_radio_test_model.cpp',
 'tests/wifi_radio_test_render.cpp',
 'tests/wifi_radio_test_runtime.cpp',
 'tools/test_wifi_radio_test.py'}
GENERATED={'release/artes.pak','release/firmware.bin','release/SHA256SUMS','release/manifest.json'}
CONTRACT={**HISTORICAL_CONTRACT,'version':'2026.10.17-wifitest77','build':2026101701,
          'package':'7.7 - Teste Wi-Fi','coop_admission_version':2,
          'coop_accept_timeout_ms':30000,'coop_technical_timeout_ms':12000,'qualification_groups':165}
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
 assert REVIEW_APPROVED is True, 'Final wifitest77 source inventory has not been independently approved'
 assert isinstance(INVENTORY_SHA,str) and re.fullmatch('[0-9a-f]{64}',INVENTORY_SHA), 'Missing approved inventory digest'
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
 assert REVIEW_APPROVED is True and REVIEWED_PRODUCTION, 'No reviewed final production hashes'
 assert set(REVIEWED_PRODUCTION)<REVIEWED_CHANGES
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
 frozen=json.loads((ROOT/'tests/fixtures/wifitest77-approved-sources.json').read_text())
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
 assert header=='#pragma once\nconstexpr const char* FW_VERSION="2026.10.17-wifitest77";\nconstexpr uint32_t FW_BUILD=2026101701u;\nconstexpr const char* FW_PACKAGE="7.7 - Teste Wi-Fi";\n'
 for name in ('README.md','docs/COOP_TRAINING_CONTROLS.md','docs/COOP_TRAINING_HARDWARE_CHECKLIST.md','docs/ESTADO_ATUAL.md','docs/GUIA_RAPIDO.md','docs/INDICE.md'):
  assert (ROOT/name).read_bytes()==git('show',BASELINE+':'+name),'Preserve published coopfix77 documentation until verified publication'
 # Every production file present in SHOP remains byte-identical except the
 # reviewed integration hooks, isolated updater body, release identity and
 # exactly two equivalent Rules returns pinned independently above.
 rules_path='firmware/RPG_POKET_2/Rules.h'
 assert_rules_compatibility(git('show',SHOP_BASELINE+':'+rules_path),(ROOT/rules_path).read_bytes())
 changed={'ClubRadio.h','RPG_POKET_2.ino','View.h','MenuView.h','UpdateDevice.h','ReleaseVersion.h','Rules.h'}
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
 assert re.search(r'LevelUp,\s*PurchaseReceipt,\s*CoopTraining,\s*WifiRadioTest\s*};',view), 'Only append WifiRadioTest after historical pages'
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
 for path in list((ROOT/'firmware/RPG_POKET_2/Coop').glob('*.h'))+[ROOT/'firmware/RPG_POKET_2/CoopDevice.h',ROOT/'firmware/RPG_POKET_2/CoopRuntime.cpp']+list((ROOT/'firmware/RPG_POKET_2').glob('WifiRadioTest*.h')):
  live='\n'.join(line.split('//',1)[0] for line in path.read_text().splitlines())
  for forbidden in ['journal.save(','savedTransition(','rpg::finish(','rpg::reserveFight(','rpg::settleFight(','settings.put','heap_caps_malloc(','new uint','setRotation(']:assert forbidden not in live,(path,forbidden)
 report=dict(schema=1,source_commit=head,source_tree=git('rev-parse','HEAD^{tree}').decode().strip(),baseline_commit=BASELINE,baseline_tree=BASELINE_TREE,coop_commit=COOP_COMMIT,frozen_inventory_sha256=INVENTORY_SHA,frozen_files=len(entries),packaging_exceptions=sorted(PACKAGING),reviewed_changes=sorted(REVIEWED_CHANGES),reviewed_production=REVIEWED_PRODUCTION,contract=CONTRACT,new_art_payload_bytes=0,new_atlas_flash_bytes=0,new_psram_allocations=0,framebuffer_bytes=153600,decode_workspace_bytes=13760,known_psram_bytes=7638272,candidate_firmware_bytes=None,target_compile_verified=False,physical_validation=False,published=False,new_reset=False,experimental_training=True,note='Native qualification does not establish RF cause or physical-board behavior. Save30/160, readers29+30, epoch1, rations10 and all frozen SHOP/PvP behavior preserved; default COOP1/RADIO1, explicit COOP1/RADIO0 and COOP0/RADIO1 required.')
 (ROOT/'build').mkdir(exist_ok=True);(ROOT/'build/wifitest77-scope-budget.json').write_text(json.dumps(report,indent=2)+'\n')
 print('PASS: exact docs-main plus reviewed isolated Wi-Fi radio test inventory; old gates/docs frozen; Save30/160/read29+30/epoch1/3slots/rations10, art666, no extra allocator calls; known PSRAM7638272; no physical qualification implied.')
if __name__=='__main__':main()
