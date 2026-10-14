#!/usr/bin/env python3
"""Static CI wiring, inventory and source-cleanliness negative checks."""
from pathlib import Path
import importlib.util
import copy
import json
import os
import subprocess
from unittest.mock import patch
import re
import sys
import tempfile
sys.dont_write_bytecode = True
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from record_package10_identity import assert_generated_only
from check_package10_scope import assert_generated_inventory, production_inventory, GENERATED
from test_package10_release import published_lineage
from test_package10_local import tasks

qualification=(ROOT/'.github/workflows/package10-check.yml').read_text()
release=(ROOT/'.github/workflows/release.yml').read_text()
def assert_isolated_compile(text):
    assert '--output-dir' not in text and '--export-binaries=true' not in text
    assert '--build-path build/esp32-package10 --export-binaries=false firmware/RPG_POKET_2' in text
    assert 'arduino-cli version | tee build/package10-esp32-build.log' in text
    assert '| tee -a build/package10-esp32-build.log' in text
    copied = text.index('cp build/esp32-package10/RPG_POKET_2.ino.bin build/RPG_POKET_2.ino.bin')
    assert text.index('arduino-cli compile') < copied < text.index('python3 tools/check_package10_scope.py') < text.index('python3 tools/release_package10_manifest.py')

for text in (qualification,release):
    assert_isolated_compile(text)
    for invalid in (
        text.replace('--build-path build/esp32-package10', '--output-dir build'),
        text.replace('--export-binaries=false', '--export-binaries=true'),
        text.replace('--export-binaries=false', ''),
        text.replace('          python3 tools/check_package10_scope.py\n', ''),
        text.replace('cp build/esp32-package10/RPG_POKET_2.ino.bin build/RPG_POKET_2.ino.bin', 'cp build/esp32-package10/* build/'),
    ):
        try: assert_isolated_compile(invalid)
        except (AssertionError, ValueError): pass
        else: raise AssertionError('Accepted unsafe compiler export/source guard wiring')
    assert 'shell: bash --noprofile --norc -euo pipefail {0}' in text
    assert 'RPG_DETECT_LEAKS: "1"' in text and 'PYTHONDONTWRITEBYTECODE: "1"' in text
    assert 'workflow_dispatch' not in text and 'vars.' not in text
    assert 'bash tools/test_package10.sh' in text
    assert 'core install esp32:esp32@3.3.12' in text
    assert 'PSRAM=opi,EraseFlash=none' in text and 'PartitionScheme=app3M_fat9M_16MB' in text
    assert 'python3 tools/test_package10_release.py release\n' in text
    assert 'python3 tools/test_package10_release.py release --sanitizers' in text
    assert 'python3 tests/package10_release_verifier.py' in text
assert 'branches: [feat/package10-release]' in qualification
assert 'contents: read' in qualification and 'contents: write' not in qualification
assert 'gh release' not in qualification and 'git push' not in qualification
assert 'branches: [main]' in release and "github.event_name == 'push' && github.ref == 'refs/heads/main'" in release
assert 'actions/workflows/package10-check.yml/runs?branch=feat%2Fpackage10-release&head_sha=$GITHUB_SHA&status=success' in release
assert release.index('python3 tools/check_package10_release_gate.py') < release.index('gh release create')
assert release.count('gh release create') == 1 and '--target "$GITHUB_SHA"' in release
assert 'tools/test_package6.sh' not in release and 'tools/test_package5.sh' not in release
wrapper=(ROOT/'tools/test_package10.sh').read_text()
assert wrapper.count('python3 -u tools/test_package10_local.py') == 1 and '--focused' not in wrapper
assert 'set -euo pipefail' in wrapper and 'export RPG_DETECT_LEAKS="${RPG_DETECT_LEAKS:-1}"' in wrapper
plan=tasks(False); byname=dict(plan)
assert len(plan)+1 == 143 and len(byname)==len(plan)
for name in ('narrative8_controller','narrative8_events','narrative8_render','package7_integration_controller','ruins7_controller','ruins7_noalloc','ruins7_render','camp_meal7_controller','camp_meal7_errors','camp_identity77_render','clock_cycle7_logic','clock_cycle7_render','fatigue7','fatigue7_crossings','fatigue7_controller','fatigue7_render'):
    assert name in byname and any('-fsanitize=address,undefined' in command for command in byname[name]),name
native=(ROOT/'tools/test_native.sh').read_text()
loops=re.findall(r'for name in ([^;]+); do',native)
for name in loops[0].split(): assert name in byname,name
for name in loops[1].split(): assert any('-fsanitize=address,undefined' in command for command in byname[name]),name
for name in ('merged-cadence','clock-real-adapters','enemy-transition-normal','enemy-transition-sanitizer','save29-30-review-normal','save29-30-review-sanitizer','combat-differential-normal','combat-differential-sanitizer'):assert name in byname,name
assert_generated_only('')
allowed=['release/firmware.bin','release/artes.pak','release/manifest.json','release/SHA256SUMS']
assert_generated_only('\n'.join('?? '+p for p in allowed))
for status in (' M firmware/RPG_POKET_2/Rules.h','M  tools/test_package10.sh','?? release/extra.txt','?? firmware/RPG_POKET_2/Injected.h','?? tests/unreviewed.py','?? tools/other.py','?? release/firmware.bin\n?? release/x','A  release/firmware.bin'):
    try: assert_generated_only(status)
    except AssertionError: pass
    else: raise AssertionError('Accepted unreviewed output/source: '+status)
with tempfile.TemporaryDirectory(prefix='package10-generated-inventory-') as temporary:
    folder=Path(temporary);(folder/'release').mkdir()
    assert_generated_inventory(folder, [])
    for name in GENERATED:
        try: assert_generated_inventory(folder, [name])
        except AssertionError: pass
        else: raise AssertionError('Accepted tracked generated output: '+name)
        path=folder/name;path.write_bytes(b'fixture');assert_generated_inventory(folder,[]);path.unlink()
        path.symlink_to(folder/'missing')
        try: assert_generated_inventory(folder, [])
        except AssertionError: pass
        else: raise AssertionError('Accepted generated symlink: '+name)
        path.unlink();path.mkdir()
        try: assert_generated_inventory(folder, [])
        except AssertionError: pass
        else: raise AssertionError('Accepted generated directory: '+name)
        path.rmdir()
    (folder/'release').rmdir();(folder/'elsewhere').mkdir();(folder/'release').symlink_to(folder/'elsewhere',target_is_directory=True)
    try: assert_generated_inventory(folder, [])
    except AssertionError: pass
    else: raise AssertionError('Accepted symlinked release output directory')
# Ignored sketch-local export files stay forbidden; no subtree exemption.
with tempfile.TemporaryDirectory(prefix='package10-production-inventory-') as temporary:
    folder=Path(temporary); sketch=folder/'firmware/RPG_POKET_2'; sketch.mkdir(parents=True)
    known=sketch/'Known.h'; known.write_text('// frozen fixture')
    expected={'firmware/RPG_POKET_2/Known.h'}
    assert production_inventory(folder,expected) == expected
    export=sketch/'build/esp32.esp32.esp32s3'; export.mkdir(parents=True)
    for name in ('RPG_POKET_2.ino.bin','build.options.json','Injected.h'):
        extra=export/name;extra.write_bytes(b'not approved')
        try: production_inventory(folder,expected)
        except AssertionError: pass
        else: raise AssertionError('Accepted ignored compiler/source file: '+name)
        extra.unlink()
    for target in (known,folder/'missing',folder):
        extra=export/'link';extra.symlink_to(target)
        try: production_inventory(folder,expected)
        except AssertionError: pass
        else: raise AssertionError('Accepted ignored production symlink: '+str(target))
        extra.unlink()

# Candidate and old historical gates are kept distinct; full inherited coverage
# is still present once each, and the narrative additions run under sanitizers.
from test_package9_local import tasks as inherited_tasks
old=dict(inherited_tasks(False))
assert set(old) <= set(byname)
pvp_names=set('pvp_reliability_protocol pvp_reliability_radio pvp_reliability_races pvp_reliability_closing pvp_reliability_persistence pvp_reliability_ordering pvp_reliability_save30 pvp_appearance_protocol pvp_presentation_render pvp_review_integration pvp_review_render pvp_review_ordering pvp_review_equivalence'.split())
assert len(pvp_names)==13 and set(byname) == set(old) | pvp_names
for name in pvp_names:
    assert any('-fsanitize=address,undefined' in command for command in byname[name]),name
    assert any('firmware/RPG_POKET_2/ClubDuel.cpp' in command and 'firmware/RPG_POKET_2/ClubProtocol.cpp' in command for command in byname[name]),name
assert 'PersistentSlotBackend<RadioStore>' in (ROOT/'tests/pvp_save30_fixture.h').read_text()
assert 'PersistentJournal<Backend>' in (ROOT/'tests/pvp_save30_fixture.h').read_text()
assert 'PERSISTENT_SAVE_SIZE' in (ROOT/'tests/pvp_reliability_save30.cpp').read_text()
assert 'test "$RPG_DETECT_LEAKS" = 1' in wrapper
assert 'test_pvp_candidate.sh' not in wrapper and 'test_pvp_reliability.sh' not in wrapper
assert 'python3 tools/capture_pvp_presentation.py build/pvp-presentation' in wrapper
assert 'build/pvp-presentation/previews/*' in qualification
assert 'build/pvp-review/screens-sanitizer/*' in qualification
for name, commands in old.items():
    assert [[part.replace('package10-binaries','package9-binaries') for part in command] for command in byname[name]] == commands, name
for name in ('narrative8_controller','narrative8_events','narrative8_render'):
    commands=byname[name]
    assert any('-fsanitize=address,undefined' in command for command in commands)
runner=(ROOT/'tools/test_package10_local.py').read_text()
assert "NARRATIVE8_SKIP_IMAGES='1'" in runner
assert 'python3 tools/capture_narrative8.py' in wrapper
assert 'build/narrative8-render/png/*' in qualification
assert 'build/narrative8-render/*.jsonl' in qualification
assert "summary['tasks'] == 143" in (ROOT/'tools/record_package10_identity.py').read_text()
assert 'python3 tools/capture_clock_visibility7.py' in wrapper
assert 'build/clock-cycle7/visibility-preview/*' in qualification
assert 'duration_ms' in (ROOT/'tools/record_package10_identity.py').read_text()
assert 'RPG_TEST_OLD_CLOCK77' in (ROOT/'tests/package10_release.cpp').read_text()
assert 'git push' not in qualification
# The production view is intentionally usable by existing header-only harnesses.
# Exercise the real pure queries at O0 and O2, without ClubDuel.cpp or any stubs.
with tempfile.TemporaryDirectory(prefix='package10-header-query-') as temporary:
    folder=Path(temporary);source=folder/'queries.cpp'
    source.write_text(r'''#include "ClubDuel.h"
#include <cassert>
#include <cstdio>
int main(){
 unsigned count=0;
 for(unsigned bits=0;bits<64;++bits)for(unsigned local=0;local<2;++local)for(unsigned turn=0;turn<2;++turn)for(unsigned cls=0;cls<4;++cls)for(unsigned mp=0;mp<9;++mp)for(unsigned action=0;action<6;++action){
  club::Duel d;d.active=bits&1;d.ready=bits&2;d.failed=bits&4;d.finished=bits&8;d.surrenderRequested=bits&16;d.waiting=bits&32;d.local=local;d.turn=turn;d.fighters[local].classId=cls;d.fighters[local].mp=mp;
  auto a=club::Action(action);const unsigned cost=action==2?(cls==2?3:4):action==1?(cls==1||cls==3?4:3):0;
  const bool playable=d.active&&d.ready&&!d.failed&&!d.finished&&!d.surrenderRequested&&(action==3||(!d.waiting&&turn==local&&action<4&&mp>=cost));
  assert(d.cost(a)==cost&&d.canPlay(a)==playable);++count;
 }
 assert(count==55296);std::printf("PASS: %u real header-only query cases, no state-machine translation unit or stub\n",count);
}
''')
    for optimization in ('-O0','-O2'):
        binary=folder/('queries'+optimization)
        subprocess.run(['g++','-std=c++17','-UNDEBUG',optimization,'-I'+str(ROOT/'firmware/RPG_POKET_2'),str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

# Fail closed against changed public metadata, Git ancestry and README bytes.
import test_package10_release as ota
probes=0
original_check_output=subprocess.check_output
with patch.dict(os.environ, {'GITHUB_ACTIONS':'false'}):
    lineage=published_lineage()
    for label in ota.SAVE30_HISTORY:
        for field,value in lineage[label].items():
            invalid=copy.deepcopy(lineage)
            invalid[label][field]=False if isinstance(value,bool) else value+1 if isinstance(value,int) else '0'*len(value)
            try: published_lineage(invalid)
            except AssertionError: probes+=1
            else: raise AssertionError('Accepted changed '+label+'/'+field)
    for field,value in lineage.items():
        if isinstance(value,dict):continue
        invalid=copy.deepcopy(lineage);invalid[field]=value+1 if isinstance(value,int) else '0'*len(value)
        try: published_lineage(invalid)
        except AssertionError:probes+=1
        else:raise AssertionError('Accepted changed lineage field: '+field)
    parent=lineage['qualification_parent']
    mutations=[
        (['rev-list','--parents','-n','1',parent],(parent+' '+ota.IMMERSION_COMMIT+' '+parent+'\n').encode()),
        (['diff','--name-only',ota.CLOCK_COMMIT,parent],b'README.md\nfirmware/RPG_POKET_2/Rules.h\n'),
        (['show',parent+':README.md'],b'changed README'),
        (['rev-list','--parents','-n','1',ota.STORY_COMMIT],(ota.STORY_COMMIT+' '+parent+'\n').encode()),
        (['rev-list','--parents','-n','1',ota.CLOCK_COMMIT],(ota.CLOCK_COMMIT+' '+parent+'\n').encode()),
        (['diff','--name-only',ota.STORY_COMMIT,lineage['clock_qualification_parent']],b'README.md\nfirmware/RPG_POKET_2/Rules.h\n'),
        (['rev-parse',parent+'^{tree}'],('0'*40+'\n').encode()),
        (['diff','--name-only',ota.IMMERSION_COMMIT,lineage['story_qualification_parent']],b'README.md\nfirmware/RPG_POKET_2/Rules.h\n'),
    ]
    for arguments,output in mutations:
        def changed(command,**kwargs):
            if command[1:]==arguments:return output
            return original_check_output(command,**kwargs)
        with patch('subprocess.check_output',side_effect=changed):
            try:published_lineage(lineage)
            except AssertionError:probes+=1
            else:raise AssertionError('Accepted Git mutation: '+repr(arguments))
    head=original_check_output(['git','rev-parse','HEAD'],cwd=ROOT).decode().strip()
    def virtual_ci(valid):
        def changed(command,**kwargs):
            if command[1:]==['rev-list','--parents','-n','1','HEAD']:
                return (head+' '+parent+('' if valid else ' '+parent)+'\n').encode()
            return original_check_output(command,**kwargs)
        return changed
    with patch.dict(os.environ,{'GITHUB_ACTIONS':'true'}):
        with patch('subprocess.check_output',side_effect=virtual_ci(False)):
            try:published_lineage(lineage)
            except AssertionError:probes+=1
            else:raise AssertionError('Accepted invalid candidate CI ancestry')
        with patch('subprocess.check_output',side_effect=virtual_ci(True)):
            published_lineage(lineage)
assert probes == sum(len(lineage[label]) for label in ota.SAVE30_HISTORY)+sum(not isinstance(value,dict) for value in lineage.values())+len(mutations)+1
print(f'PASS: branch-only read permission; future main exact-SHA gate; isolated no-export ESP32 compile and postbuild guards; ignored production/symlink rejection; full inherited narrative and expanded clock task plan (143 tasks including 13 PvP); LSan mandatory; exact source/public lineage plus {probes} fail-closed probes; capture artifacts.')
