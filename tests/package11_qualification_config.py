#!/usr/bin/env python3
"""Negative checks for qualification wiring, inherited coverage and source guards."""
from pathlib import Path
import copy
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
import threading
import time
from unittest.mock import patch
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from record_package11_identity import assert_generated_only
from check_package11_scope import assert_generated_inventory, production_inventory, GENERATED
from test_package11_local import tasks
from test_package10_local import tasks as inherited_tasks

REPOSITORY = 'Oflodorporto/RPG-POKET-2'
BASELINE = '6a23dda54114db611b818b51e4b1b942019098fa'
FQBN = 'esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,EraseFlash=none'
qualification = (ROOT/'.github/workflows/package11-check.yml').read_text()
release = (ROOT/'.github/workflows/release.yml').read_text()
probes = 0


def rejected(check, value, label):
    global probes
    try: check(value)
    except (AssertionError, ValueError, KeyError): probes += 1
    else: raise AssertionError('Accepted '+label)


def section(text, key):
    found = re.findall(r'^'+re.escape(key)+r':[^\n]*\n((?:[ \t]+[^\n]*\n|\n|#[^\n]*\n)*)', text, re.M)
    assert len(found) == 1, 'Missing/duplicate '+key
    return found[0]


def assert_isolated_compile(text):
    assert '--output-dir' not in text and '--export-binaries=true' not in text
    assert text.count('arduino-cli compile') == 1
    assert text.count('--export-binaries') == 1
    assert text.count('--build-path') == 1
    assert '--build-path build/esp32-package11 --export-binaries=false firmware/RPG_POKET_2' in text
    assert "--fqbn '"+FQBN+"'" in text
    assert 'core install esp32:esp32@3.3.12' in text
    assert 'arduino-cli version | tee build/package11-esp32-build.log' in text
    assert '| tee -a build/package11-esp32-build.log' in text
    assert 'test "$(git rev-parse HEAD)" = "$GITHUB_SHA"' in text
    compiled = text.index('arduino-cli compile')
    copied = text.index('cp build/esp32-package11/RPG_POKET_2.ino.bin build/RPG_POKET_2.ino.bin')
    scope = text.index('python3 tools/check_package11_scope.py', copied)
    manifest = text.index('python3 tools/release_package11_manifest.py')
    verifier = text.index('python3 tools/verify_package11_release.py', manifest)
    negative = text.index('python3 tests/package11_release_verifier.py', verifier)
    ota = text.index('python3 tools/test_package11_release.py release\n', negative)
    sanitizer = text.index('python3 tools/test_package11_release.py release --sanitizers', ota)
    assert compiled < copied < scope < manifest < verifier < negative < ota < sanitizer
    assert not re.search(r'\b(?:cd|cp|mv|rsync)\b[^\n]*firmware/RPG_POKET_2', text)


def assert_workflow(text, publishing):
    branch = 'main' if publishing else 'feat/package11-release'
    event = [line.strip() for line in section(text, 'on').splitlines() if line.strip() and not line.lstrip().startswith('#')]
    assert len(event) == 3 and event[:2] == ['push:', 'branches: ['+branch+']']
    assert event[2].startswith('paths: [') and event[2].endswith(']')
    permissions = [line.strip() for line in section(text, 'permissions').splitlines() if line.strip()]
    assert permissions == (['contents: write', 'actions: read'] if publishing else ['contents: read'])
    expected_if = "github.repository == '"+REPOSITORY+"' && github.event_name == 'push' && github.ref == 'refs/heads/"+branch+"'"
    guards = re.findall(r'^\s+if: (.+)$', text, re.M)
    assert guards == [expected_if]*(2 if publishing else 1)
    assert len(re.findall(r'^  [a-z]+:\s*$', section(text, 'jobs'), re.M)) == 1
    assert 'shell: bash --noprofile --norc -euo pipefail {0}' in text
    assert 'RPG_DETECT_LEAKS: "1"' in text and 'PYTHONDONTWRITEBYTECODE: "1"' in text
    assert 'fetch-depth: 0' in text
    for forbidden in ('workflow_dispatch', 'pull_request', 'workflow_run', 'schedule:', 'vars.', 'continue-on-error:', 'actions/download-artifact', 'gh run download', 'git push'):
        assert forbidden not in text, forbidden
    assert text.count('bash tools/test_package11.sh') == 1
    assert 'bash tools/test_package10.sh' not in text
    assert 'python3 tools/check_package10_scope.py' not in text
    assert 'python3 tests/package10_qualification_config.py' not in text
    assert_isolated_compile(text)
    if publishing:
        query = 'actions/workflows/package11-check.yml/runs?branch=feat%2Fpackage11-release&head_sha=$GITHUB_SHA&status=success&per_page=100'
        assert query in text
        assert text.count('python3 tools/check_package11_release_gate.py "$RUNNER_TEMP/qualification.json"') == 2
        assert text.index('python3 tools/check_package11_release_gate.py') < text.index('bash tools/test_package11.sh') < text.index('arduino-cli compile') < text.index('gh release create')
        assert text.rindex('python3 tools/check_package11_release_gate.py') < text.index('gh release create')
        assert text.count('gh release create') == 1
        assert 'gh release create v2026.10.14-shop77 release/firmware.bin release/artes.pak release/manifest.json --target "$GITHUB_SHA"' in text
        assert "--title 'RPG POKET - 7.7 - Comercio em lote' --notes-file docs/RELEASE_PACKAGE11_NOTAS.txt --latest" in text
        assert 'assert_generated_only(subprocess.check_output' in text
        assert 'python3 tools/verify_package11_release.py build/published-package11' in text
        assert 'python3 tools/test_package11_release.py build/published-package11' in text
        assert 'cmp "release/$name" "build/published-package11/$name"' in text
        assert 'cmp release/manifest.json build/published-package11/latest.json' in text
    else:
        for forbidden in ('gh release', 'contents: write', 'actions: write', 'GH_TOKEN:', 'git tag', 'git commit', 'curl '):
            assert forbidden not in text, forbidden
        assert text.index('python3 tools/test_package11_release.py release --sanitizers') < text.index('python3 tools/record_package11_identity.py') < text.index('actions/upload-artifact@v4')
        for path in ('build/package11-validation/**', 'build/package11-source-identity.json', 'build/package11-scope-budget.json', 'build/package11-esp32-build.log', 'build/bulk-shop/screens/*.png', 'build/bulk-shop/sanitized/*.ppm', 'build/ration-limit10-oldreader/**', 'build/pvp-review/screens-sanitizer/*', 'build/narrative8-render/png/*', 'build/clock-cycle7/visibility-preview/*', 'build/package7-differential*.json'):
            assert path in text, path


for text, publishing in ((qualification, False), (release, True)):
    assert_workflow(text, publishing)
    for bad in (
        text.replace('--build-path build/esp32-package11', '--output-dir build'),
        text.replace('--build-path build/esp32-package11', '--build-path firmware/RPG_POKET_2/build'),
        text.replace('--export-binaries=false', '--export-binaries=true'),
        text.replace('--export-binaries=false', ''),
        text.replace('core install esp32:esp32@3.3.12', 'core install esp32:esp32'),
        text.replace('PSRAM=opi', 'PSRAM=disabled'),
        text.replace('EraseFlash=none', 'EraseFlash=all'),
        text.replace('          python3 tools/check_package11_scope.py\n', ''),
        text.replace('cp build/esp32-package11/RPG_POKET_2.ino.bin build/RPG_POKET_2.ino.bin', 'cp build/esp32-package11/* build/'),
        text.replace('          python3 tools/release_package11_manifest.py\n', ''),
        text.replace('          python3 tools/verify_package11_release.py\n', ''),
        text.replace('          python3 tools/test_package11_release.py release --sanitizers\n', ''),
    ):
        rejected(assert_isolated_compile, bad, 'unsafe compiler/manifest/source guard wiring')
    for bad in (
        text.replace('on:\n  push:', 'on:\n  workflow_dispatch:\n  push:'),
        text.replace('on:\n  push:', 'on:\n  pull_request:'),
        text.replace('branches: [main]' if publishing else 'branches: [feat/package11-release]', 'branches: ["**"]'),
        text.replace("github.repository == '"+REPOSITORY+"' && ", ''),
        text.replace('RPG_DETECT_LEAKS: "1"', 'RPG_DETECT_LEAKS: "0"'),
        text.replace('fetch-depth: 0', 'fetch-depth: 1'),
        text.replace('bash tools/test_package11.sh', 'bash tools/test_package10.sh'),
        text.replace('shell: bash --noprofile --norc -euo pipefail {0}', 'shell: bash'),
        text.replace('permissions:', 'permissions:\n  actions: write'),
    ):
        rejected(lambda value: assert_workflow(value, publishing), bad, 'unsafe branch/permission/full-retest wiring')
for old, new in (
    ('head_sha=$GITHUB_SHA', 'head_sha=main'),
    ('branch=feat%2Fpackage11-release', 'branch=main'),
    ('--target "$GITHUB_SHA"', '--target main'),
    ('python3 tools/check_package11_release_gate.py "$RUNNER_TEMP/qualification.json"', 'true'),
    ('cmp "release/$name" "build/published-package11/$name"', 'true'),
):
    rejected(lambda value: assert_workflow(value, True), release.replace(old, new), 'unsafe exact-SHA/public-byte wiring')

wrapper = (ROOT/'tools/test_package11.sh').read_text()
assert wrapper.count('python3 -u tools/test_package11_local.py') == 1 and '--focused' not in wrapper
assert 'set -euo pipefail' in wrapper and 'test "$#" = 0' in wrapper
assert 'export RPG_DETECT_LEAKS="${RPG_DETECT_LEAKS:-1}"' in wrapper
assert 'test "$RPG_DETECT_LEAKS" = 1' in wrapper
assert 'python3 tools/capture_bulk_commerce.py' in wrapper
assert 'test -s build/bulk-shop/screens/compras-vendas-em-lote.png' in wrapper
for stale in ('tools/test_package10.sh', 'tools/check_package10_scope.py', 'tests/package10_qualification_config.py', 'tools/check_bulk_commerce_scope.py', 'tools/test_bulk_commerce.sh'):
    assert stale not in wrapper

plan = tasks(False); byname = dict(plan); old = dict(inherited_tasks(False))
assert len(plan)+1 == 149 and len(byname) == len(plan)
assert len(old)+1 == 143 and set(old) <= set(byname)
for name, commands in old.items():
    assert [[part.replace('package11-binaries','package10-binaries') for part in command] for command in byname[name]] == commands, name
new_names = {'bulk_commerce', 'bulk_commerce_controller', 'bulk_commerce_render', 'ration_limit10', 'ration-limit10-oldreader-normal', 'ration-limit10-oldreader-sanitizer'}
assert set(byname) == set(old) | new_names
for name in ('bulk_commerce', 'bulk_commerce_controller', 'bulk_commerce_render', 'ration_limit10'):
    commands = byname[name]
    assert len(commands) == 4
    assert '-O2' in commands[0] and '-UNDEBUG' in commands[0]
    assert '-fsanitize=address,undefined' in commands[2] and '-UNDEBUG' in commands[2]
    assert '-fno-omit-frame-pointer' in commands[2] and '-no-pie' in commands[2]
    assert commands[1][0].endswith(name+'_normal') and commands[3][0].endswith(name+'_sanitizer')
for command in byname['bulk_commerce_render'][::2]:
    assert '-DRPG_TEST_COMMERCE_ALLOCATIONS' in command and '-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc' in command
assert byname['bulk_commerce_render'][1][-1] == 'build/bulk-shop/screens'
assert byname['bulk_commerce_render'][3][-1] == 'build/bulk-shop/sanitized'
assert byname['ration-limit10-oldreader-normal'] == [['python3','tools/test_ration_limit10_oldreader.py']]
assert byname['ration-limit10-oldreader-sanitizer'] == [['python3','tools/test_ration_limit10_oldreader.py','--sanitizers']]
runner = (ROOT/'tools/test_package11_local.py').read_text()
assert "NARRATIVE8_SKIP_IMAGES='1'" in runner
assert "assert not a.focused" in runner and "os.environ.get('RPG_DETECT_LEAKS') == '1'" in runner
assert "with oldreader_lock if name.startswith('ration-limit10-oldreader-')" in runner
assert "LSAN_OPTIONS='exitcode=23'" in runner
assert 'Checkpoint log missing or changed' in runner
assert 'Checkpoint commands changed' in runner
assert 'Frozen input changed; create a new run-id' in runner
assert 'Source changed during validation' in runner
assert "summary['tasks'] == 149" in (ROOT/'tools/record_package11_identity.py').read_text()

# Exercise the runner itself: CI restrictions, frozen plan/source, log evidence,
# and the shared old-reader staging lock, without compiling synthetic commands.
import test_package11_local as local_runner
for focused, leaks in ((True, '1'), (False, '0')):
    args = ['runner', '--run-id', 'configuration-probe'] + (['--focused'] if focused else [])
    with patch.dict(os.environ, {'GITHUB_ACTIONS':'true', 'RPG_DETECT_LEAKS':leaks}), patch.object(sys, 'argv', args):
        rejected(lambda _: local_runner.main(), None, 'CI focused/disabled-LSan runner')
with tempfile.TemporaryDirectory(prefix='package11-checkpoint-') as temporary:
    folder = Path(temporary)
    fingerprint = {'sha256':'a'*64, 'files':{'reviewed.cpp':'b'*64}, 'detect_leaks':'0'}
    selected = [('ration-limit10-oldreader-normal', [['probe','normal']]), ('ration-limit10-oldreader-sanitizer', [['probe','sanitizer']])]
    seen = []; active = [0]; mutex = threading.Lock()
    def pretend_run(command, **kwargs):
        if command[0] == 'probe':
            with mutex:
                active[0] += 1
                assert active[0] == 1, 'Old-reader modes raced while staging headers'
            time.sleep(0.02)
            with mutex:
                seen.append(command[1]); active[0] -= 1
        return subprocess.CompletedProcess(command, 0)
    cwd = Path.cwd()
    try:
        with patch.dict(os.environ, {'GITHUB_ACTIONS':'false','RPG_DETECT_LEAKS':'0'}), patch.object(sys, 'argv', ['runner','--run-id','fixture','--jobs','2']), patch.object(local_runner, 'ROOT', folder), patch.object(local_runner, 'tasks', return_value=selected), patch.object(local_runner, 'inventory', return_value=fingerprint), patch.object(local_runner.subprocess, 'run', side_effect=pretend_run):
            local_runner.main()
            assert sorted(seen) == ['normal','sanitizer']
            out = folder/'build/package11-validation/fixture'
            summary = json.loads((out/'summary.json').read_text())
            assert summary['all_passed'] and summary['tasks'] == 3
            local_runner.main()
            assert len(seen) == 2, 'An intact completed checkpoint was needlessly rerun'
            with patch.object(local_runner, 'inventory', return_value={'sha256':'c'*64}):
                rejected(lambda _: local_runner.main(), None, 'changed frozen source manifest')
            with patch.object(local_runner, 'tasks', return_value=selected+ [('extra', [['probe','extra']])]):
                rejected(lambda _: local_runner.main(), None, 'changed frozen task plan')
            log = out/'ration-limit10-oldreader-normal.log'
            original = log.read_bytes(); log.write_bytes(b'forged successful evidence')
            rejected(lambda _: local_runner.main(), None, 'modified resumed checkpoint log')
            log.write_bytes(original)
            checkpoints = out/'checkpoints.json'; state = json.loads(checkpoints.read_text())
            state['ration-limit10-oldreader-normal']['commands'] = [['probe','different']]
            checkpoints.write_text(json.dumps(state))
            rejected(lambda _: local_runner.main(), None, 'modified resumed checkpoint command')
    finally:
        os.chdir(cwd)

# ESP32 uses unsigned long for uint32_t even though unsigned int is also 32-bit.
# Compile the actual production function in an ILP32 type-only harness. No
# multilib headers/libraries are needed: std::min has its standard two-argument
# declaration and dependencies are declarations only. This is not an ESP32 build.
def assert_sale_maximum_integer_types(trading):
    global probes
    functions = re.findall(r'^inline unsigned saleMaximum\([^\n]+\}$', trading, re.M)
    assert len(functions) == 1, 'Missing/changed production saleMaximum extraction'
    production = functions[0]
    deduced = production.replace('std::min<uint32_t>(', 'std::min(')
    preamble = r'''using uint32_t = unsigned long;
static_assert(sizeof(unsigned int)==4 && sizeof(unsigned long)==4, "ILP32 required");
static_assert(__is_same(uint32_t,unsigned long), "Target uint32_t alias required");
namespace std { template<class T> constexpr const T& min(const T&,const T&); }
struct Game { struct { uint32_t gold; } p; unsigned equipped[4]; };
bool tradeSafe(const Game&); bool tradable(unsigned); bool tradeGear(unsigned);
unsigned gearSlot(unsigned); unsigned tradeUnit(const Game&,unsigned);
unsigned tradeStock(const Game&,unsigned);
static_assert(__is_same(decltype(tradeStock(Game{},0)),unsigned int), "Stock type");
static_assert(__is_same(decltype((999999u-Game{}.p.gold)/1u),unsigned long), "Room type");
'''
    with tempfile.TemporaryDirectory(prefix='package11-sale-integer-types-') as temporary:
        source = Path(temporary)/'saleMaximum.cpp'
        command = ['g++','-std=c++17','-m32','-fsyntax-only',str(source)]
        source.write_text(preamble+production+'\n')
        fixed = subprocess.run(command,capture_output=True,text=True,env=dict(os.environ,LC_ALL='C'))
        assert fixed.returncode == 0, fixed.stderr
        assert deduced != production, 'Missing explicit saleMaximum comparison type'
        source.write_text(preamble+deduced+'\n')
        broken = subprocess.run(command,capture_output=True,text=True,env=dict(os.environ,LC_ALL='C'))
        assert broken.returncode != 0 and 'deduced conflicting types' in broken.stderr, broken.stderr
        probes += 1
    print('PASS: real saleMaximum text compiles with ILP32 unsigned-long uint32_t; deduced-min regression rejected (type-only, not ESP32 compilation).')


assert_sale_maximum_integer_types((ROOT/'firmware/RPG_POKET_2/Trading.h').read_text())
# Separately execute the complete production header with the real host library,
# tying the declaration-only type probe above to the real operand declarations.
with tempfile.TemporaryDirectory(prefix='package11-sale-host-') as temporary:
    folder=Path(temporary);source=folder/'sale.cpp';binary=folder/'sale'
    source.write_text(r'''#include "Trading.h"
#include <cassert>
#include <type_traits>
using namespace rpg;
static_assert(std::is_same<decltype(Game{}.p.gold),uint32_t>::value, "Gold type changed");
static_assert(std::is_same<decltype(tradeStock(Game{},0)),unsigned>::value, "Stock type changed");
static_assert(std::is_same<decltype(tradeUnit(Game{},0)),unsigned>::value, "Unit type changed");
int main(){
 auto g=create(0,47);g.p.life=3;const unsigned unit=tradeUnit(g,0);assert(unit);
 const unsigned room[]={0,unit-1,unit,unit*2-1,unit*3,999999};
 const unsigned expected[]={0,0,1,1,3,3};
 for(unsigned i=0;i<6;++i){g.p.gold=999999-room[i];assert(saleMaximum(g,0)==expected[i]);}
 g.p.gold=1000000;assert(saleMaximum(g,0)==0);
}
''')
    subprocess.run(['g++','-std=c++17','-O2','-UNDEBUG','-I'+str(ROOT/'firmware/RPG_POKET_2'),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    print('PASS: real Trading.h host execution, six sale-cap boundaries plus invalid-gold guard; production operand types verified.')

# Historical package10 implementation, fixtures and test tools stay byte-identical.
for name in subprocess.check_output(['git','ls-tree','-r','--name-only',BASELINE],cwd=ROOT,text=True).splitlines():
    if 'package10' in Path(name).name or name == '.github/workflows/package10-check.yml':
        assert (ROOT/name).read_bytes() == subprocess.check_output(['git','show',BASELINE+':'+name],cwd=ROOT), 'Historical package10 changed: '+name

# Reject tracked generated files, ignored compiler exports and production symlinks.
assert_generated_only('')
assert_generated_only('\n'.join('?? '+name for name in sorted(GENERATED)))
for status in (' M firmware/RPG_POKET_2/Rules.h','M  tools/test_package11.sh','?? release/extra.txt','?? firmware/RPG_POKET_2/Injected.h','?? tests/unreviewed.py','?? tools/other.py','?? release/firmware.bin\n?? release/x','A  release/firmware.bin','?? release/../firmware.bin'):
    rejected(assert_generated_only, status, 'unreviewed source/output '+status)
with tempfile.TemporaryDirectory(prefix='package11-generated-inventory-') as temporary:
    folder=Path(temporary);(folder/'release').mkdir()
    assert_generated_inventory(folder, [])
    for name in GENERATED:
        rejected(lambda names: assert_generated_inventory(folder, names), [name], 'tracked generated output '+name)
        path=folder/name;path.write_bytes(b'fixture');assert_generated_inventory(folder,[]);path.unlink()
        path.symlink_to(folder/'missing')
        rejected(lambda _: assert_generated_inventory(folder, []), None, 'generated symlink '+name)
        path.unlink();path.mkdir()
        rejected(lambda _: assert_generated_inventory(folder, []), None, 'generated directory '+name)
        path.rmdir()
    (folder/'release').rmdir();(folder/'elsewhere').mkdir();(folder/'release').symlink_to(folder/'elsewhere',target_is_directory=True)
    rejected(lambda _: assert_generated_inventory(folder, []), None, 'symlinked release output directory')
with tempfile.TemporaryDirectory(prefix='package11-production-inventory-') as temporary:
    folder=Path(temporary);sketch=folder/'firmware/RPG_POKET_2';sketch.mkdir(parents=True)
    known=sketch/'Known.h';known.write_text('// frozen fixture')
    expected={'firmware/RPG_POKET_2/Known.h'}
    assert production_inventory(folder,expected) == expected
    export=sketch/'build/esp32.esp32.esp32s3';export.mkdir(parents=True)
    for name in ('RPG_POKET_2.ino.bin','build.options.json','Injected.h'):
        extra=export/name;extra.write_bytes(b'not approved')
        rejected(lambda _: production_inventory(folder, expected), None, 'ignored compiler/source file '+name)
        extra.unlink()
    for target in (known,folder/'missing',folder):
        extra=export/'link';extra.symlink_to(target)
        rejected(lambda _: production_inventory(folder, expected), None, 'ignored production symlink')
        extra.unlink()
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

# Fail closed on every public metadata field and reviewed documentation ancestry.
import test_package11_release as ota
original_check_output = subprocess.check_output
with patch.dict(os.environ, {'GITHUB_ACTIONS': 'false'}):
    lineage = ota.published_lineage()
    for key, value in lineage.items():
        if isinstance(value, dict):
            for field, item in value.items():
                invalid = copy.deepcopy(lineage)
                invalid[key][field] = False if isinstance(item, bool) else item+1 if isinstance(item, int) else '0'*len(item)
                rejected(ota.published_lineage, invalid, 'changed lineage '+key+'/'+field)
        else:
            invalid = copy.deepcopy(lineage)
            invalid[key] = value+1 if isinstance(value, int) else '0'*len(value)
            rejected(ota.published_lineage, invalid, 'changed lineage '+key)
    invalid = copy.deepcopy(lineage); invalid['schema'] = True
    rejected(ota.published_lineage, invalid, 'boolean masquerading as lineage schema')
    invalid = copy.deepcopy(lineage); invalid['unreviewed'] = 'extra'
    rejected(ota.published_lineage, invalid, 'unreviewed lineage field')
    parent = lineage['qualification_parent']; showcase = lineage['showcase_commit']
    mutations = [
        (['rev-list','--parents','-n','1',parent], (parent+' '+showcase+' '+parent+'\n').encode()),
        (['diff','--name-only',showcase,parent], b'README.md\nfirmware/RPG_POKET_2/Rules.h\n'),
        (['show',parent+':README.md'], b'changed README'),
        (['rev-parse',parent+'^{tree}'], ('0'*40+'\n').encode()),
        (['rev-list','--parents','-n','1',ota.PVP_COMMIT], (ota.PVP_COMMIT+' '+showcase+'\n').encode()),
        (['diff','--name-only',ota.CLOCK_COMMIT,lineage['pvp_qualification_parent']], b'README.md\nfirmware/RPG_POKET_2/Rules.h\n'),
        (['diff','--name-only',ota.PVP_COMMIT,lineage['pvp_readme_commit']], b'README.md\nfirmware/RPG_POKET_2/Rules.h\n'),
        (['diff','--name-only',lineage['pvp_readme_commit'],showcase], b'README.md\n'),
        (['diff','--name-only',lineage['pvp_readme_commit'],showcase], ('\n'.join((*ota.SHOWCASE_PATHS,'firmware/RPG_POKET_2/Rules.h'))+'\n').encode()),
        (['rev-list','--parents','-n','1',ota.STORY_COMMIT], (ota.STORY_COMMIT+' '+parent+'\n').encode()),
        (['rev-list','--parents','-n','1',ota.CLOCK_COMMIT], (ota.CLOCK_COMMIT+' '+parent+'\n').encode()),
        (['diff','--name-only',ota.STORY_COMMIT,lineage['clock_qualification_parent']], b'README.md\nfirmware/RPG_POKET_2/Rules.h\n'),
        (['diff','--name-only',ota.IMMERSION_COMMIT,lineage['story_qualification_parent']], b'README.md\nfirmware/RPG_POKET_2/Rules.h\n'),
    ]
    for arguments, output in mutations:
        def changed(command, **kwargs):
            if command[1:] == arguments: return output
            return original_check_output(command, **kwargs)
        with patch('subprocess.check_output', side_effect=changed):
            rejected(ota.published_lineage, lineage, 'Git ancestry/source mutation '+repr(arguments))
    head = original_check_output(['git','rev-parse','HEAD'],cwd=ROOT).decode().strip()
    def virtual_ci(valid):
        def changed(command, **kwargs):
            if command[1:] == ['rev-list','--parents','-n','1','HEAD']:
                return (head+' '+parent+('' if valid else ' '+parent)+'\n').encode()
            return original_check_output(command, **kwargs)
        return changed
    with patch.dict(os.environ, {'GITHUB_ACTIONS':'true'}):
        with patch('subprocess.check_output', side_effect=virtual_ci(False)):
            rejected(ota.published_lineage, lineage, 'invalid candidate CI ancestry')
        with patch('subprocess.check_output', side_effect=virtual_ci(True)):
            ota.published_lineage(lineage)
print(f'PASS: {probes} negative configuration/source/lineage probes; branch-only read permission; future main exact-SHA gate; isolated ESP32 core3.3.12 no-export build; 143 intact inherited plus6 bulk tasks (149); mandatory CI LSan; actual render evidence; frozen historical tools.')
