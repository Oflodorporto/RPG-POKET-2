#!/usr/bin/env python3
"""Fail-closed package12 source inventory and preserved SHOP/Save/OTA budgets."""
from pathlib import Path
import hashlib,json,re,struct,subprocess,zlib
ROOT=Path(__file__).resolve().parents[1]
BASELINE='136c748fd35aebd2df7c9bdaddd01d224646fddd'
BASELINE_TREE='32354343098cd57b75d9569cdcd81532b1fe7d1d'
COOP_COMMIT='08e4cf22059a90d8902a2c10b9abad059852f506'
COOP_TREE='0409abf815dfe9acd474f910e26676cf7c0ffddd'
INVENTORY_SHA='3a7c0b7eb88933f7f8519dd2ea894423c9dd9be67c11092f62fe757f3df91106'
RULES_BASE_SHA='1e935a35f4ac3ff25e4a4182d22f39db36276e06b14c3090f2c5ce1a648603f7'
RULES_COMPAT_SHA='6d7d57fc0cf9f1156ba293dc0350b0230841c5150f9a3bdd0018c15c40f2a51c'
PACKAGING={
 '.github/workflows/release.yml','.github/workflows/package12-check.yml',
 'tools/check_package12_scope.py','tools/check_package12_release_gate.py',
 'tools/test_package12.sh','tools/test_package12_local.py','tools/test_package12_release.py',
 'tools/verify_package12_release.py','tools/release_package12_manifest.py','tools/record_package12_identity.py',
 'tools/test_package12_coop.py','tools/capture_package12_coop.py',
 'tests/package12_release.cpp','tests/package12_release_gate.py','tests/package12_release_verifier.py',
 'tests/package12_build_evidence.py','tests/package12_qualification_config.py','tests/package12_coop_config.py','tests/package12_coop_controller.cpp',
 'tests/fixtures/package12-approved-sources.json','tests/fixtures/package12-published-baselines.json',
 'docs/PACKAGE12_RELEASE_QUALIFICATION.md','docs/RELEASE_PACKAGE12_NOTAS.txt','PACKAGE12_STAGING_ONLY.txt',
}
GENERATED={'release/artes.pak','release/firmware.bin','release/SHA256SUMS','release/manifest.json'}
CONTRACT={'version':'2026.10.14-coop77','build':2026101406,'package':'7.7 - Treino cooperativo experimental','generation':'7.7','save_format':30,'save_bytes':160,'reads_save_formats':[29,30],'save_reset_epoch':1,'ration_limit':10,'old_reader_ration_limit':9,'art_assets':666,'art_payload_bytes':7470912,'art_total_bytes':7470928,'art_crc32':'9b4bcfc0','art_sha256':'14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56','known_psram_bytes':7638272,'ota_partition_bytes':3145728,'new_atlas_flash_bytes':0,'new_psram_allocations':0,'coop_production_default':1,'coop_host_default':0,'pvp_protocol_version':5,'pvp_packet_bytes':52}
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT)
def assert_rules_compatibility(before,after):
 # GCC UBSan lowers the pointer-selection expression with an uninitialized
 # generated index on its day branch. Selecting indexed values preserves both tables
 # and the selection exactly, without disabling diagnostics or sanitizers.
 old=b'return (night?dark:day)[n%4];'
 new=b'return night?dark[n%4]:day[n%4];'
 assert hashlib.sha256(before).hexdigest()==RULES_BASE_SHA,'Unexpected historical Rules source'
 assert before.count(old)==2 and after==before.replace(old,new),'Rules changed beyond the two equivalent indexed returns'
 assert hashlib.sha256(after).hexdigest()==RULES_COMPAT_SHA,'Unreviewed Rules compatibility change'
def assert_generated_inventory(root,tracked):
 assert not set(tracked)&GENERATED,'Generated releases must not be tracked'
 for name in GENERATED:
  p=root/name;assert not p.parent.is_symlink(),name
  if p.exists() or p.is_symlink():assert p.is_file() and not p.is_symlink(),name
def production_inventory(root,expected):
 found=set()
 for p in (root/'firmware').rglob('*'):
  assert not p.is_symlink(),str(p)
  if p.is_file():found.add(str(p.relative_to(root)))
 assert found<=expected,'Unexpected production input: '+repr(sorted(found-expected))
 return found

def main():
 assert git('rev-parse',BASELINE+'^{tree}').decode().strip()==BASELINE_TREE
 frozen=json.loads((ROOT/'tests/fixtures/package12-approved-sources.json').read_text())
 assert frozen['schema']==1 and frozen['baseline_commit']==BASELINE and frozen['baseline_tree']==BASELINE_TREE
 assert frozen['coop_commit']==COOP_COMMIT and frozen['coop_tree']==COOP_TREE
 assert set(frozen['packaging_exceptions'])==PACKAGING and frozen['contract']==CONTRACT
 entries=frozen['frozen_files'];assert len({e['path'] for e in entries})==len(entries)
 assert hashlib.sha256(json.dumps(entries,sort_keys=True,separators=(',',':')).encode()).hexdigest()==INVENTORY_SHA,'Reviewed source inventory changed'
 for e in entries:
  name=e['path'];assert not Path(name).is_absolute() and '..' not in Path(name).parts
  if name in PACKAGING:continue
  p=ROOT/name;assert p.is_file() and not p.is_symlink(),name
  data=p.read_bytes();assert len(data)==e['bytes'] and hashlib.sha256(data).hexdigest()==e['sha256'],'Frozen source changed: '+name
  assert bool(p.stat().st_mode&0o111)==e['executable'],'Source mode changed: '+name
 expected={e['path'] for e in entries}|PACKAGING
 tracked=set(git('ls-files','--cached','-z').decode().split('\0'))-{''};assert_generated_inventory(ROOT,tracked)
 actual=set(git('ls-files','--cached','--others','--exclude-standard','-z').decode().split('\0'))-{''}
 assert not actual-expected-GENERATED,'Unexpected source paths: '+repr(sorted(actual-expected-GENERATED))
 assert expected<=actual,'Missing source paths: '+repr(sorted(expected-actual))
 for name in PACKAGING:assert (ROOT/name).is_file() and not (ROOT/name).is_symlink(),name
 production=production_inventory(ROOT,expected)
 header=(ROOT/'firmware/RPG_POKET_2/ReleaseVersion.h').read_text()
 assert header=='#pragma once\nconstexpr const char* FW_VERSION="2026.10.14-coop77";\nconstexpr uint32_t FW_BUILD=2026101406u;\nconstexpr const char* FW_PACKAGE="7.7 - Treino cooperativo experimental";\n'
 for name in ('README.md','docs/ESTADO_ATUAL.md'):
  assert (ROOT/name).read_bytes()==git('show',BASELINE+':'+name),'Preserve current published SHOP/lore/showcase docs'
 # Every production file present in SHOP remains byte-identical except the
 # reviewed integration hooks, isolated updater body, release identity and
 # exactly two equivalent Rules returns pinned independently above.
 rules_path='firmware/RPG_POKET_2/Rules.h'
 assert_rules_compatibility(git('show',BASELINE+':'+rules_path),(ROOT/rules_path).read_bytes())
 changed={'ClubRadio.h','RPG_POKET_2.ino','View.h','UpdateDevice.h','ReleaseVersion.h','Rules.h'}
 for path in git('ls-tree','-r','--name-only',BASELINE,'firmware').decode().splitlines():
  if path.removeprefix('firmware/RPG_POKET_2/') not in changed:
   assert (ROOT/path).read_bytes()==git('show',BASELINE+':'+path),'SHOP production regression: '+path
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
 before=sum(len(alloc.findall(git('show',BASELINE+':'+n).decode())) for n in git('ls-tree','-r','--name-only',BASELINE,'firmware').decode().splitlines())
 after=sum(len(alloc.findall((ROOT/n).read_text())) for n in production);assert after==before,'Unexpected production allocator call'
 for path in list((ROOT/'firmware/RPG_POKET_2/Coop').glob('*.h'))+[ROOT/'firmware/RPG_POKET_2/CoopDevice.h',ROOT/'firmware/RPG_POKET_2/CoopRuntime.cpp']:
  live='\n'.join(line.split('//',1)[0] for line in path.read_text().splitlines())
  for forbidden in ['journal.save(','savedTransition(','rpg::finish(','rpg::reserveFight(','rpg::settleFight(','settings.put','heap_caps_malloc(','new uint','setRotation(']:assert forbidden not in live,(path,forbidden)
 report=dict(schema=1,source_commit=git('rev-parse','HEAD').decode().strip(),source_tree=git('rev-parse','HEAD^{tree}').decode().strip(),baseline_commit=BASELINE,coop_commit=COOP_COMMIT,frozen_inventory_sha256=INVENTORY_SHA,frozen_files=len(entries),packaging_exceptions=sorted(PACKAGING),contract=CONTRACT,new_art_payload_bytes=0,new_atlas_flash_bytes=0,new_psram_allocations=0,framebuffer_bytes=153600,decode_workspace_bytes=13760,known_psram_bytes=7638272,candidate_firmware_bytes=None,target_compile_verified=False,physical_validation=False,published=False,new_reset=False,experimental_training=True,note='Known allocation bound, not free heap/FPS/RF/touch measurements. Save30 and all SHOP semantics preserved. Production Arduino default ON; standalone inherited host tests OFF, explicit integrated ON qualification is mandatory.')
 (ROOT/'build').mkdir(exist_ok=True);(ROOT/'build/package12-scope-budget.json').write_text(json.dumps(report,indent=2)+'\n')
 print('PASS: exact main SHOP plus reviewed coop inventory; Save30/160/read29+30/epoch1/rations10, art666, unchanged core rules and old gates; no extra allocator calls; known PSRAM7638272; experimental production default ON.')
if __name__=='__main__':main()
