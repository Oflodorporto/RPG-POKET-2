#!/usr/bin/env python3
"""Static CI wiring, inventory and source-cleanliness negative checks."""
from pathlib import Path
import importlib.util
import re
import sys
import tempfile
sys.dont_write_bytecode = True
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from record_package7_identity import assert_generated_only
from check_package7_scope import assert_generated_inventory, GENERATED
from test_package7_local import tasks

qualification=(ROOT/'.github/workflows/package7-check.yml').read_text()
release=(ROOT/'.github/workflows/release.yml').read_text()
for text in (qualification,release):
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
print('PASS: main-push-only publication, branch-only read permission, shell/pipefail, mandatory LSan, exact-SHA source gate, complete inherited+4-module plan (127 tasks), exact four generated-output paths and negative probes.')
