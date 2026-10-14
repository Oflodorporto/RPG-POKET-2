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
from record_package7_identity import assert_generated_only
from check_package7_scope import assert_generated_inventory, production_inventory, GENERATED
from test_package7_release import published_lineage
from test_package7_local import tasks

qualification=(ROOT/'.github/workflows/package7-check.yml').read_text()
release=(ROOT/'.github/workflows/release.yml').read_text()
def assert_isolated_compile(text):
    assert '--output-dir' not in text and '--export-binaries=true' not in text
    assert '--build-path build/esp32-package7 --export-binaries=false firmware/RPG_POKET_2' in text
    assert 'arduino-cli version | tee build/package7-esp32-build.log' in text
    assert '| tee -a build/package7-esp32-build.log' in text
    copied = text.index('cp build/esp32-package7/RPG_POKET_2.ino.bin build/RPG_POKET_2.ino.bin')
    assert text.index('arduino-cli compile') < copied < text.index('python3 tools/check_package7_scope.py') < text.index('python3 tools/release_package7_manifest.py')

for text in (qualification,release):
    assert_isolated_compile(text)
    for invalid in (
        text.replace('--build-path build/esp32-package7', '--output-dir build'),
        text.replace('--export-binaries=false', '--export-binaries=true'),
        text.replace('--export-binaries=false', ''),
        text.replace('          python3 tools/check_package7_scope.py\n', ''),
        text.replace('cp build/esp32-package7/RPG_POKET_2.ino.bin build/RPG_POKET_2.ino.bin', 'cp build/esp32-package7/* build/'),
    ):
        try: assert_isolated_compile(invalid)
        except (AssertionError, ValueError): pass
        else: raise AssertionError('Accepted unsafe compiler export/source guard wiring')
    assert 'shell: bash --noprofile --norc -euo pipefail {0}' in text
    assert 'RPG_DETECT_LEAKS: "1"' in text and 'PYTHONDONTWRITEBYTECODE: "1"' in text
    assert 'workflow_dispatch' not in text and 'vars.' not in text
    assert 'bash tools/test_package7.sh' in text
    assert 'core install esp32:esp32@3.3.12' in text
    assert 'PSRAM=opi,EraseFlash=none' in text and 'PartitionScheme=app3M_fat9M_16MB' in text
    assert 'python3 tools/test_package7_release.py release\n' in text
    assert 'python3 tools/test_package7_release.py release --sanitizers' in text
    assert 'python3 tests/package7_release_verifier.py' in text
assert 'branches: [feat/package7-release]' in qualification
assert 'contents: read' in qualification and 'contents: write' not in qualification
assert 'gh release' not in qualification and 'git push' not in qualification
assert 'branches: [main]' in release and "github.event_name == 'push' && github.ref == 'refs/heads/main'" in release
assert 'actions/workflows/package7-check.yml/runs?branch=feat%2Fpackage7-release&head_sha=$GITHUB_SHA&status=success' in release
assert release.index('python3 tools/check_package7_release_gate.py') < release.index('gh release create')
assert release.count('gh release create') == 1 and '--target "$GITHUB_SHA"' in release
assert 'tools/test_package6.sh' not in release and 'tools/test_package5.sh' not in release
wrapper=(ROOT/'tools/test_package7.sh').read_text()
assert wrapper.count('python3 -u tools/test_package7_local.py') == 1 and '--focused' not in wrapper
assert 'set -euo pipefail' in wrapper and 'export RPG_DETECT_LEAKS="${RPG_DETECT_LEAKS:-1}"' in wrapper
plan=tasks(False); byname=dict(plan)
assert len(plan)+1 == 127 and len(byname)==len(plan)
for name in ('package7_integration_controller','ruins7_controller','ruins7_noalloc','ruins7_render','camp_meal7_controller','camp_meal7_errors','camp_identity77_render','clock_cycle7_logic','clock_cycle7_render','fatigue7','fatigue7_crossings','fatigue7_controller','fatigue7_render'):
    assert name in byname and any('-fsanitize=address,undefined' in command for command in byname[name]),name
native=(ROOT/'tools/test_native.sh').read_text()
loops=re.findall(r'for name in ([^;]+); do',native)
for name in loops[0].split(): assert name in byname,name
for name in loops[1].split(): assert any('-fsanitize=address,undefined' in command for command in byname[name]),name
for name in ('merged-cadence','clock-real-adapters','enemy-transition-normal','enemy-transition-sanitizer','save29-30-review-normal','save29-30-review-sanitizer','combat-differential-normal','combat-differential-sanitizer'):assert name in byname,name
assert_generated_only('')
allowed=['release/firmware.bin','release/artes.pak','release/manifest.json','release/SHA256SUMS']
assert_generated_only('\n'.join('?? '+p for p in allowed))
for status in (' M firmware/RPG_POKET_2/Rules.h','M  tools/test_package7.sh','?? release/extra.txt','?? firmware/RPG_POKET_2/Injected.h','?? tests/unreviewed.py','?? tools/other.py','?? release/firmware.bin\n?? release/x','A  release/firmware.bin'):
    try: assert_generated_only(status)
    except AssertionError: pass
    else: raise AssertionError('Accepted unreviewed output/source: '+status)
with tempfile.TemporaryDirectory(prefix='package7-generated-inventory-') as temporary:
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
with tempfile.TemporaryDirectory(prefix='package7-production-inventory-') as temporary:
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

# Exercise real pinned Git objects plus fail-closed metadata/ancestry/source probes.
lineage=json.loads((ROOT/'tests/fixtures/package7-published-baselines.json').read_text())
with patch.dict(os.environ, {'GITHUB_ACTIONS':'false'}):
    published_lineage(lineage)
    for field in ('historical_readme_base','historical_readme_base_tree','historical_readme_sha256','qualification_parent','qualification_parent_tree'):
        invalid=copy.deepcopy(lineage);invalid[field]='0'*len(invalid[field])
        try: published_lineage(invalid)
        except AssertionError: pass
        else: raise AssertionError('Accepted changed pinned lineage: '+field)
    original_check_output=subprocess.check_output
    def probe_git(change_args, output, ci=False):
        def changed(command, **kwargs):
            if command[1:] == change_args: return output
            return original_check_output(command,**kwargs)
        with patch.dict(os.environ,{'GITHUB_ACTIONS':'true' if ci else 'false'}),patch('subprocess.check_output',side_effect=changed):
            try: published_lineage(lineage)
            except AssertionError: return
        raise AssertionError('Accepted altered ancestry/source: '+repr(change_args))
    parent=lineage['qualification_parent'];base=lineage['historical_readme_base']
    probe_git(['rev-list','--parents','-n','1',parent],(parent+' '+base+' '+base+'\n').encode())
    probe_git(['diff','--name-only',parent],b'firmware/RPG_POKET_2/Rules.h\n')
    probe_git(['diff','--name-only',parent],b'tools/test_package6.sh\n')
    probe_git(['show',base+':README.md'],b'changed README')
    probe_git(['rev-list','--parents','-n','1','HEAD'],(parent+' '+base+'\n').encode(),ci=True)
    # A virtual retry HEAD with exactly the recorded parent exercises the CI success path.
    def valid_ci(command,**kwargs):
        if command[1:] == ['rev-list','--parents','-n','1','HEAD']:
            head=original_check_output(['git','rev-parse','HEAD'],cwd=ROOT).decode().strip()
            return (head+' '+parent+'\n').encode()
        return original_check_output(command,**kwargs)
    with patch.dict(os.environ,{'GITHUB_ACTIONS':'true'}),patch('subprocess.check_output',side_effect=valid_ci):
        published_lineage(lineage)

print('PASS: main-push-only publication, branch-only read permission, shell/pipefail, mandatory LSan, exact-SHA source gate, complete inherited+4-module plan (127 tasks), exact four generated-output paths; isolated no-export compile, ignored-file/symlink rejection and exact retry lineage negatives.')
