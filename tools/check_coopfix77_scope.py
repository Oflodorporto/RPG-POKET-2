#!/usr/bin/env python3
"""Exact docs-main plus reviewed touch/admission/terminal hotfix, fail closed.

Historical package12/SHOP/PvP gates and inventories remain immutable. This
new scope permits only the listed reviewed source delta and new release files.
"""
from pathlib import Path
import hashlib,json,os,re,struct,subprocess,sys,zlib
sys.dont_write_bytecode=True
from check_package12_scope import (assert_rules_compatibility, assert_generated_inventory,
    production_inventory, CONTRACT as HISTORICAL_CONTRACT, BASELINE as SHOP_BASELINE)
ROOT=Path(__file__).resolve().parents[1]
BASELINE='d767538669137ca08a2dd53f3805e8ba1be006b8'
BASELINE_TREE='e2963cbd9236ea3c2990884a8e4f088544beb613'
COOP_COMMIT='da68148210b3155b72e41deb08a46ab854d09a0e'
COOP_TREE='6c003c6df520658297b832f6abc6e61645defd8c'
INVENTORY_SHA='9cb0634c6f3ad031732c38bb9db84c17fd972fa869553857b4d686af1f9b3a10'
PACKAGING={'tests/coopfix77_release_gate.py', 'docs/COOPFIX77_RELEASE_QUALIFICATION.md', 'docs/RELEASE_COOPFIX77_NOTAS.txt', '.github/workflows/coopfix77-check.yml', 'tools/verify_coopfix77_release.py', 'tests/fixtures/coopfix77-published-baselines.json', 'tools/test_coopfix77_release.py', 'tests/fixtures/coopfix77-approved-sources.json', 'tests/coopfix77_scope.py', 'tests/coopfix77_invite_render.py', 'tools/release_coopfix77_manifest.py', 'tests/coopfix77_build_evidence.py', '.github/workflows/release.yml', 'tools/test_coopfix77_local.py', 'tests/coopfix77_qualification_config.py', 'tools/check_coopfix77_release_gate.py', 'tests/coopfix77_release_verifier.py', 'tests/coopfix77_release.cpp', 'tools/test_coopfix77.sh', 'tools/check_coopfix77_scope.py', 'tools/check_coopfix77_invite_render.py', 'COOPFIX77_STAGING_ONLY.txt', 'tools/record_coopfix77_identity.py'}
REVIEWED_CHANGES={'tools/test_coop_invite_countdown_regression.py', 'firmware/RPG_POKET_2/Coop/CoopInput.h', 'tests/coop_invite_countdown_regression.cpp', 'firmware/RPG_POKET_2/Coop/CoopSession.h', 'tools/test_coop_touch_queue_regression.py', 'firmware/RPG_POKET_2/ReleaseVersion.h', 'tests/coop_training_render.cpp', 'firmware/RPG_POKET_2/Coop/CoopTraining.h', 'tests/coop_training_session.cpp', 'tests/coop_training_radio.cpp', 'firmware/RPG_POKET_2/Coop/CoopView.h', 'firmware/RPG_POKET_2/Coop/CoopViewModel.h', 'firmware/RPG_POKET_2/Coop/CoopAdmission.h', 'tests/coop_touch_queue_regression.cpp'}
GENERATED={'release/artes.pak','release/firmware.bin','release/SHA256SUMS','release/manifest.json'}
CONTRACT={**HISTORICAL_CONTRACT,'version':'2026.10.15-coopfix77','build':2026101501,
          'package':'7.7 - Treino cooperativo: toque, saida e convite','coop_admission_version':2,
          'coop_accept_timeout_ms':30000,'coop_technical_timeout_ms':12000,'qualification_groups':153}
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT)
def validate_files(root,entries):
 names=[e['path'] for e in entries]
 assert len(names)==len(set(names)), 'Duplicate frozen path'
 for e in entries:
  name=e['path'];assert isinstance(name,str) and name and not Path(name).is_absolute() and '..' not in Path(name).parts
  assert name not in PACKAGING, 'Packaging may not hide in frozen inventory'
  p=root/name;assert p.is_file() and not p.is_symlink(),name
  assert not any(parent.is_symlink() for parent in p.parents if parent!=root and root in parent.parents),name
  data=p.read_bytes();assert len(data)==e['bytes'] and hashlib.sha256(data).hexdigest()==e['sha256'],'Frozen source changed: '+name
  assert bool(p.stat().st_mode&0o111)==e['executable'],'Source mode changed: '+name

def main():
 assert git('rev-parse',BASELINE+'^{tree}').decode().strip()==BASELINE_TREE
 assert git('rev-parse',COOP_COMMIT+'^{tree}').decode().strip()==COOP_TREE
 head=git('rev-parse','HEAD').decode().strip()
 if head!=BASELINE:
  assert git('rev-list','--parents','-n','1','HEAD').decode().split()==[head,BASELINE], 'Candidate must be sole child of exact docs-main lease'
 if os.environ.get('GITHUB_ACTIONS')=='true':
  assert head!=BASELINE and head==os.environ['GITHUB_SHA']
 frozen=json.loads((ROOT/'tests/fixtures/coopfix77-approved-sources.json').read_text())
 assert frozen['schema']==1 and frozen['baseline_commit']==BASELINE and frozen['baseline_tree']==BASELINE_TREE
 assert frozen['coop_commit']==COOP_COMMIT and frozen['coop_tree']==COOP_TREE
 assert set(frozen['packaging_exceptions'])==PACKAGING and set(frozen['reviewed_changes'])==REVIEWED_CHANGES
 assert frozen['contract']==CONTRACT
 entries=frozen['frozen_files']
 assert hashlib.sha256(json.dumps(entries,sort_keys=True,separators=(',',':')).encode()).hexdigest()==INVENTORY_SHA,'Reviewed source inventory changed'
 validate_files(ROOT,entries)
 expected={e['path'] for e in entries}|PACKAGING
 tracked=set(git('ls-files','--cached','-z').decode().split('\0'))-{''};assert_generated_inventory(ROOT,tracked)
 actual=set(git('ls-files','--cached','--others','--exclude-standard','-z').decode().split('\0'))-{''}
 assert not actual-expected-GENERATED,'Unexpected source paths: '+repr(sorted(actual-expected-GENERATED))
 assert expected<=actual,'Missing source paths: '+repr(sorted(expected-actual))
 for name in PACKAGING:assert (ROOT/name).is_file() and not (ROOT/name).is_symlink(),name
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
 assert header=='#pragma once\nconstexpr const char* FW_VERSION="2026.10.15-coopfix77";\nconstexpr uint32_t FW_BUILD=2026101501u;\nconstexpr const char* FW_PACKAGE="7.7 - Treino cooperativo: toque, saida e convite";\n'
 for name in ('README.md','docs/ESTADO_ATUAL.md'):
  assert (ROOT/name).read_bytes()==git('show',BASELINE+':'+name),'Preserve published coop77 documentation until verified publication'
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
 report=dict(schema=1,source_commit=head,source_tree=git('rev-parse','HEAD^{tree}').decode().strip(),baseline_commit=BASELINE,baseline_tree=BASELINE_TREE,coop_commit=COOP_COMMIT,frozen_inventory_sha256=INVENTORY_SHA,frozen_files=len(entries),packaging_exceptions=sorted(PACKAGING),reviewed_changes=sorted(REVIEWED_CHANGES),contract=CONTRACT,new_art_payload_bytes=0,new_atlas_flash_bytes=0,new_psram_allocations=0,framebuffer_bytes=153600,decode_workspace_bytes=13760,known_psram_bytes=7638272,candidate_firmware_bytes=None,target_compile_verified=False,physical_validation=False,published=False,new_reset=False,experimental_training=True,note='Native qualification does not establish RF cause or physical-board behavior. Save30/160, readers29+30, epoch1, rations10 and all frozen SHOP/PvP behavior preserved; default Arduino ON and explicit OFF required.')
 (ROOT/'build').mkdir(exist_ok=True);(ROOT/'build/coopfix77-scope-budget.json').write_text(json.dumps(report,indent=2)+'\n')
 print('PASS: exact docs-main plus reviewed hotfix inventory; old gates/docs frozen; Save30/160/read29+30/epoch1/rations10, art666, no extra allocator calls; known PSRAM7638272; no physical qualification implied.')
if __name__=='__main__':main()
