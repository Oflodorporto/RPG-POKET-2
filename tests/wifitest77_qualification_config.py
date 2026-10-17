#!/usr/bin/env python3
"""Fail-closed wifitest77 workflow, inherited plan and evidence wiring probes.

This configuration test never executes sanitizer runtimes, Arduino or a remote
write. The actual full runner, target builds and real-byte OTA are separate gates.
"""
if not __debug__:
    raise RuntimeError("Qualification requires assertions; optimized Python is forbidden")
from pathlib import Path
import copy
import fnmatch
import json
import os
import re
import subprocess
import sys
import tempfile
import threading
import time
from unittest.mock import patch
import yaml
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
REPOSITORY = 'Oflodorporto/RPG-POKET-2'
BASELINE = '4fc221f07b62d6bd26fcf03fa1bb12c8fd577111'
FQBN = 'esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,EraseFlash=none'
PATHS = ['firmware/**', 'assets/**', 'cartao/**', 'docs/**', 'tests/**', 'tools/**', 'README.md', 'WIFITEST77_STAGING_ONLY.txt', '.github/workflows/wifitest77-check.yml', '.github/workflows/release.yml']
ARTIFACTS = '''release/firmware.bin
release/artes.pak
release/manifest.json
release/SHA256SUMS
build/wifitest77-scope-budget.json
build/wifitest77-source-identity.json
build/wifitest77-ota/*.json
build/wifitest77-tests.log
build/wifitest77-esp32*.json
build/wifitest77-esp32*.log
build/wifitest77-esp32*.txt
build/package12-validation/**/*.json
build/package12-validation/**/*.log
build/package7-differential*.json
build/screens/procedural6/*.ppm
build/screens/package6-integration/*.ppm
build/screens/camp-identity77/*.ppm
build/screens/camp-identity77/*.png
build/screens/camp-identity77/*.gif
build/screens/camp-identity77/*.json
build/d20-camp77/previews/*.ppm
build/d20-camp77/previews/*.png
build/d20-camp77/previews/*.gif
build/d20-camp77/previews/*.json
build/ruins-bag77/ruins-bag-*.png
build/ruins7/*.png
build/clock-cycle7/previews/*.png
build/clock-cycle7/previews/*.gif
build/clock-cycle7/previews/*.json
build/clock-cycle7/visibility-preview/*.png
build/clock-cycle7/visibility-preview/*.gif
build/clock-cycle7/visibility-preview/*.json
build/fatigue7/screens/*.ppm
build/narrative8-render/*.json
build/narrative8-render/*.jsonl
build/narrative8-render/*.html
build/narrative8-render/*.txt
build/narrative8-render/*.log
build/narrative8-render/png/*.png
build/narrative8-render/contact-sheets/*.jpg
build/narrative8-events/*.ppm
build/pvp-presentation/previews/*.png
build/pvp-presentation/previews/*.gif
build/pvp-presentation/capture-manifest.json
build/pvp-presentation/screens/*.txt
build/pvp-presentation/sanitized/*.txt
build/pvp-review/screens-normal/*.ppm
build/pvp-review/screens-sanitizer/*.ppm
build/bulk-shop/screens/*.png
build/bulk-shop/screens/*.ppm
build/bulk-shop/sanitized/*.ppm
build/ration-limit10-oldreader/frozen/*.h
build/coop-training-native/**/*.log
build/coop-training-native/**/*.json
build/coop-training-native/**/*.txt
build/coop-training-native/**/*.d
build/coop-training-render/**/*.png
build/coop-training-render/**/*.ppm
build/coop-training-render/**/*.json
build/coop-training-render/**/*.log
build/coop-training-render/**/*.tsv
build/coop-touch-queue/*.log
build/coop-touch-queue/*.json
build/coop-invite-countdown/*.log
build/coop-invite-countdown/*.json
build/coop-radio-recovery/*.log
build/coop-radio-recovery/*.json
build/coop-radio-device/*.log
build/coop-radio-device/*.json
build/coop-radio-runtime-ui/*.log
build/coop-radio-runtime-ui/*.json
build/coop-radio-render/**/*.json
build/coop-radio-render/**/*.log
build/coop-radio-render/**/*.png
build/coop-radio-render/**/*.ppm
build/coop-radio-render/**/*.tsv
build/coop-radio-render/**/*.gif
build/wifi-radio-test/**/*.json
build/wifi-radio-test/**/*.log
build/wifi-radio-test/**/*.png
build/wifi-radio-test/**/*.ppm
build/wifi-radio-test/**/*.tsv
build/wifi-radio-test/**/*.csv'''.splitlines()
CORE = '''arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.12'''
COMPILE = 'test "$(git rev-parse HEAD)" = "$GITHUB_SHA"\npython3 tools/check_wifitest77_scope.py\narduino-cli version | tee build/wifitest77-esp32-toolchain.txt\narduino-cli core list | tee -a build/wifitest77-esp32-toolchain.txt\nfor variant in coop1-radio1 coop1-radio0 coop0-radio1; do\n  extra=()\n  case "$variant" in\n    coop1-radio1) ;;\n    coop1-radio0) extra+=(--build-property \'compiler.cpp.extra_flags=-DRPG_ENABLE_WIFI_RADIO_TEST=0\'); ;;\n    coop0-radio1) extra+=(--build-property \'compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=0\'); ;;\n  esac\n  arduino-cli compile --verbose --jobs 4 \\\n    --fqbn \'esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,EraseFlash=none\' \\\n    "${extra[@]}" \\\n    --build-path "$RUNNER_TEMP/esp32-wifitest77/$variant" \\\n    --export-binaries=false firmware/RPG_POKET_2 2>&1 | tee "build/wifitest77-esp32-$variant.log"\ndone\ncp "$RUNNER_TEMP/esp32-wifitest77/coop1-radio1/RPG_POKET_2.ino.bin" build/RPG_POKET_2.ino.bin\npython3 tools/check_wifitest77_scope.py'
VERIFY = '''python3 tools/release_wifitest77_manifest.py
python3 tools/verify_wifitest77_release.py
python3 tests/wifitest77_release_verifier.py
python3 tools/test_wifitest77_release.py release
python3 tools/test_wifitest77_release.py release --sanitizers
python3 tools/check_wifitest77_scope.py'''
GATE = '''test "$GITHUB_REPOSITORY" = Oflodorporto/RPG-POKET-2
test "$GITHUB_EVENT_NAME" = push
test "$GITHUB_REF" = refs/heads/main
gh api "repos/$GITHUB_REPOSITORY/actions/workflows/wifitest77-check.yml/runs?branch=feat%2Fwifitest77-release&head_sha=$GITHUB_SHA&status=success&per_page=100" > "$RUNNER_TEMP/qualification.json"
python3 tools/check_wifitest77_release_gate.py "$RUNNER_TEMP/qualification.json"
test "$(gh api "repos/$GITHUB_REPOSITORY/git/matching-refs/tags/v2026.10.17-wifitest77" --jq 'length')" = 0'''
PUBLISH = GATE+'''\npython3 -c 'import sys,subprocess; sys.path.insert(0,"tools"); from record_wifitest77_identity import assert_generated_only; assert_generated_only(subprocess.check_output(["git","status","--porcelain","--untracked-files=all"],text=True).strip())'
gh api "repos/$GITHUB_REPOSITORY/git/ref/heads/main" > "$RUNNER_TEMP/main-ref.json"
python3 tools/check_wifitest77_release_gate.py "$RUNNER_TEMP/qualification.json" --main-ref "$RUNNER_TEMP/main-ref.json"
gh release create v2026.10.17-wifitest77 release/firmware.bin release/artes.pak release/manifest.json --target "$GITHUB_SHA" --title 'RPG POKET - 7.7 - Teste Wi-Fi' --notes-file docs/RELEASE_WIFITEST77_NOTAS.txt --latest'''
DOWNLOAD = '''mkdir -p build/published-wifitest77
for name in firmware.bin artes.pak manifest.json; do
  curl --fail --location --retry 5 --retry-all-errors --output "build/published-wifitest77/$name" "https://github.com/$GITHUB_REPOSITORY/releases/download/v2026.10.17-wifitest77/$name"
  cmp "release/$name" "build/published-wifitest77/$name"
done
python3 tools/verify_wifitest77_release.py build/published-wifitest77
python3 tools/test_wifitest77_release.py build/published-wifitest77
python3 tools/test_wifitest77_release.py build/published-wifitest77 --sanitizers
matched=0
for attempt in $(seq 1 12); do
  if curl --fail --location --retry 2 --output build/published-wifitest77/latest.json "https://github.com/$GITHUB_REPOSITORY/releases/latest/download/manifest.json" && cmp release/manifest.json build/published-wifitest77/latest.json; then matched=1; break; fi
  sleep 10
done
test "$matched" = 1'''


class UniqueLoader(yaml.BaseLoader):
    """Reject duplicate YAML keys rather than checking a silently overwritten map."""
    def construct_mapping(self, node, deep=False):
        result = {}
        for key_node, value_node in node.value:
            key = self.construct_object(key_node, deep=deep)
            assert key not in result, 'Duplicate YAML key: '+str(key)
            result[key] = self.construct_object(value_node, deep=deep)
        return result


def validate(document, publishing):
    assert set(document) == {'name', 'on', 'permissions', 'concurrency', 'jobs'}
    branch = 'main' if publishing else 'feat/wifitest77-release'
    assert document['on'] == {'push': {'branches': [branch], 'paths': PATHS}}
    assert document['permissions'] == ({'contents': 'write', 'actions': 'read'} if publishing else {'contents': 'read'})
    assert document['concurrency'] == {'group': 'wifitest77-main' if publishing else 'wifitest77-${{ github.ref }}', 'cancel-in-progress': 'false' if publishing else 'true'}
    name = 'release' if publishing else 'validate'
    assert set(document['jobs']) == {name}
    job = document['jobs'][name]
    assert set(job) == {'if', 'runs-on', 'timeout-minutes', 'defaults', 'env', 'steps'}
    guard = f"github.repository == '{REPOSITORY}' && github.event_name == 'push' && github.ref == 'refs/heads/{branch}'"
    assert job['if'] == guard and job['runs-on'] == 'ubuntu-latest' and job['timeout-minutes'] == '180'
    assert job['defaults'] == {'run': {'shell': 'bash --noprofile --norc -euo pipefail {0}'}}
    assert job['env'] == {'RPG_DETECT_LEAKS': '1', 'PYTHONDONTWRITEBYTECODE': '1'}
    expected = [{'uses': 'actions/checkout@v4', 'with': {'fetch-depth': '0', 'persist-credentials': 'false'}}]
    def run(command, token=False, condition=False):
        step = {'run': command}
        if token: step['env'] = {'GH_TOKEN': '${{ github.token }}'}
        if condition: step['if'] = guard
        expected.append(step)
    if publishing: run(GATE, token=True)
    run("python3 -m pip install 'Pillow==11.3.0' 'PyYAML==6.0.3'")
    run('mkdir -p build\nbash tools/test_wifitest77.sh 2>&1 | tee build/wifitest77-tests.log')
    expected.append({'uses': 'arduino/setup-arduino-cli@v2'})
    run(CORE)
    run(COMPILE)
    run(VERIFY)
    run('python3 tools/record_wifitest77_identity.py --build-root "$RUNNER_TEMP/esp32-wifitest77"')
    expected.append({'uses': 'actions/upload-artifact@v4', 'with': {
        'name': 'wifitest77-'+('publisher-' if publishing else '')+'${{ github.sha }}',
        'path': '\n'.join(ARTIFACTS), 'if-no-files-found': 'error', 'retention-days': '14'}})
    if publishing:
        run(PUBLISH, token=True, condition=True)
        run(DOWNLOAD)
    actual = copy.deepcopy(job['steps'])
    for step in actual:
        step.pop('name', None)
        if 'run' in step: step['run'] = step['run'].rstrip('\n')
        if 'with' in step and 'path' in step['with']: step['with']['path'] = step['with']['path'].rstrip('\n')
    assert actual == expected, 'Missing, changed, skipped, reordered or additional workflow operation'
    all_runs = '\n'.join(step.get('run', '') for step in actual)
    for forbidden in ('--output-dir', '--export-binaries=true', 'build.extra_flags=', '-mtext-section-literals', '-mauto-litpools', 'git push', 'git tag', 'git commit', '|| true', 'set +e', 'detect_leaks=0', 'actions/download-artifact', 'gh run download'):
        assert forbidden not in all_runs, forbidden
    if not publishing:
        for forbidden in ('gh ', 'curl ', 'GH_TOKEN', 'contents: write'):
            assert forbidden not in all_runs, forbidden
    # The production build has no coop override. Only fallback OFF may add one.
    assert COMPILE.count('RPG_ENABLE_COOP_TRAINING=') == 1
    assert COMPILE.count('RPG_ENABLE_WIFI_RADIO_TEST=') == 1
    assert 'for variant in coop1-radio1 coop1-radio0 coop0-radio1' in all_runs



RADIO_SUITES = ('model','flow','device','races','export','runtime','render-on','render-off')
RADIO_TASKS = [
    ('coop-wifi-radio-'+suite, [
        ['python3','tools/test_wifi_radio_test.py','--suite',suite,'--out','build/wifi-radio-test/'+suite+'/normal'],
        ['python3','tools/test_wifi_radio_test.py','--suite',suite,'--out','build/wifi-radio-test/'+suite+'/sanitizers','--sanitizers'],
    ]) for suite in RADIO_SUITES
]
def validate_aggregate(wrapper, plan, runner):
    from test_coopdiag77_local import tasks as inherited_tasks
    inherited = inherited_tasks(False)
    assert len(inherited) == 156 and len(plan) == len(dict(plan)) == 164
    assert plan == inherited + RADIO_TASKS, 'Dropped, reordered or changed inherited/radio command'
    for name, commands in plan:
        for command in commands:
            if len(command)>1 and command[0] in ('python3','bash') and command[1].startswith(('tools/','tests/')):
                assert (ROOT/command[1]).is_file(), 'Missing aggregate command source: '+command[1]
    expected_wrapper = (ROOT/'tools/test_coopdiag77.sh').read_text().replace('coopdiag77','wifitest77')
    expected_wrapper = expected_wrapper.replace('set -euo pipefail','set -euo pipefail\ntest "${PYTHONOPTIMIZE:-0}" = 0')
    expected_wrapper = expected_wrapper.replace('python3 tests/wifitest77_scope.py','python3 tests/wifitest77_scope.py\npython3 tests/wifitest77_optimization.py')
    assert wrapper == expected_wrapper, 'Mandatory wrapper operation changed'
    for required in (
        'import test_coopdiag77_local as prior', 'inherited = prior.inherited', 'INHERITED_TASKS = inherited.tasks',
        'plan = prior.tasks(False)', 'return plan + RADIO_TASKS',
        "assert focused is False", "assert '--focused' not in sys.argv",
        "assert os.environ.get('RPG_DETECT_LEAKS', '1') == '1'", 'if not __debug__:',
        'inherited.tasks = tasks', 'inherited.main()', 'finally:', 'inherited.tasks = INHERITED_TASKS',
    ): assert required in runner, required
    for forbidden in ('subprocess', 'detect_leaks=0', 'exec(', 'compile(', 'shell=True'):
        assert forbidden not in runner, forbidden

def aggregate_probes():
    import test_wifitest77_local as local
    import coopdiag77_qualification_config as historical
    count = historical.aggregate_probes()
    wrapper = (ROOT/'tools/test_wifitest77.sh').read_text()
    runner = (ROOT/'tools/test_wifitest77_local.py').read_text()
    plan = local.tasks(False)
    validate_aggregate(wrapper,plan,runner);count+=1
    def rejects(w=wrapper,p=plan,r=runner):
        nonlocal count
        try: validate_aggregate(w,p,r)
        except (AssertionError, ValueError, KeyError): count+=1
        else: raise AssertionError('Accepted changed Wi-Fi qualification plan')
    for index,(name,commands) in enumerate(plan):
        rejects(p=plan[:index]+plan[index+1:])
        bad=copy.deepcopy(plan);bad[index][1][0].append('--unapproved');rejects(p=bad)
    rejects(p=list(reversed(plan)))
    for old,new in (
        ('test "$RPG_DETECT_LEAKS" = 1','true'),('--sanitizers',''),
        ('python3 tests/wifitest77_build_evidence.py','true'),
        ('python3 tests/wifitest77_optimization.py','true'),
        ('python3 tests/package12_coop_config.py','true'),
        ('python3 tools/capture_bulk_commerce.py','true'),('set -euo pipefail','set -eu'),
        ('test "${PYTHONOPTIMIZE:-0}" = 0','true'),
    ): rejects(w=wrapper.replace(old,new))
    for old,new in [('inherited.main()','pass'),('assert focused is False','assert True'),
                    ("assert os.environ.get('RPG_DETECT_LEAKS', '1') == '1'",'assert True')]:
        rejects(r=runner.replace(old,new))
    for args,leaks in ((['--focused'],'1'),([],'0')):
        with patch.dict(os.environ,{'RPG_DETECT_LEAKS':leaks}), patch.object(sys,'argv',['runner','--run-id','offline',*args]), patch.object(local.inherited,'main') as intercepted:
            try: local.main()
            except AssertionError: count+=1
            else: raise AssertionError('Accepted focused or leak-suppressed qualification')
            intercepted.assert_not_called()
    with patch.dict(os.environ,{'RPG_DETECT_LEAKS':'1'}), patch.object(sys,'argv',['runner','--run-id','offline']), patch.object(local.inherited,'main') as intercepted:
        local.main();intercepted.assert_called_once();count+=1
    assert local.inherited.tasks is local.INHERITED_TASKS
    integrated=[name for name,_ in plan if name.startswith('coop-')]
    assert integrated[-8:]==[name for name,_ in RADIO_TASKS]
    assert integrated.index('coop-render-noalloc')<integrated.index('coop-wifi-radio-model')
    return count

def artifact_matches(path, pattern):
    """Match relative upload globs component-wise, including zero-depth **."""
    parts = path.split('/'); glob = pattern.split('/')
    def match(left, right):
        if not right: return not left
        if right[0] == '**':
            return match(left, right[1:]) or bool(left) and match(left[1:], right)
        return bool(left) and fnmatch.fnmatchcase(left[0],right[0]) and match(left[1:],right[1:])
    return match(parts,glob)

def selected_artifact(path, patterns=None):
    return any(artifact_matches(path,pattern) for pattern in (ARTIFACTS if patterns is None else patterns))

def artifact_selection_probes():
    checks = 0
    # Every advertised evidence pattern remains selectable; no directory or
    # extensionless wildcard can silently re-admit compiler products.
    assert len(ARTIFACTS)==len(set(ARTIFACTS))
    for pattern in ARTIFACTS:
        assert not pattern.endswith('*'), 'Unbounded artifact selection: '+pattern
        sample=pattern.replace('**/','nested/').replace('*','proof')
        assert selected_artifact(sample), sample
        checks += 1
        if '**/' in pattern:
            assert selected_artifact(pattern.replace('**/','').replace('*','proof'))
            checks += 1
    forbidden = {
        'build/ration-limit10-oldreader/oldreader-normal',
        'build/ration-limit10-oldreader/oldreader-san',
        'build/d20-camp77/previews/d20_camp77_capture',
        'build/RPG_POKET_2.ino.bin', 'release/firmware-off.bin',
        'build/coop-training-render/coop-training-render',
        'build/coop-training-render/coop-training-render-san',
        'build/coop-touch-queue/regression-normal',
        'build/coop-touch-queue/regression-sanitizers',
        'build/coop-invite-countdown/regression-normal',
        'build/coop-invite-countdown/regression-sanitizers',
        'build/coop-training-native/normal/controller',
        'build/coop-training-native/sanitizers/controller',
        'build/coop-training-native/host-default-off.o',
        'build/coop-training-native/arduino-explicit-off.o',
        'build/coop-training-native/runtime-arduino-default-on.o',
        'build/coop-training-native/coop_update_install_linkage-normal',
        'build/wifitest77-esp32/flag-0/RPG_POKET_2.ino.bin',
        'build/wifitest77-esp32-flag-0.bin',
    }
    # Exercise every pattern's directory and filename prefix, not merely the
    # three concrete ELF paths found in the previous published artifact.
    for pattern in ARTIFACTS:
        sample=pattern.replace('**/','nested/').replace('*','proof')
        stem=str(Path(sample).with_suffix(''))
        for suffix in ('','.o','.obj','.elf','.ELF','.a','.so','.dylib','.dll','.exe','.bin'):
            candidate=stem+suffix
            if candidate not in ('release/firmware.bin','release/SHA256SUMS'): forbidden.add(candidate)
    for path in sorted(forbidden):
        assert not selected_artifact(path), 'Executable/object/OFF-image selected: '+path
        checks += 1
    # Positive fixtures cover the evidence next to the formerly leaked ELFs.
    required = ['build/ration-limit10-oldreader/frozen/Persistence.h',
                'build/ration-limit10-oldreader/frozen/Camp.h',
                'build/d20-camp77/previews/capture-report.json',
                'build/d20-camp77/previews/d20-rolling.gif',
                'build/d20-camp77/previews/d20-all-faces.png',
                'build/d20-camp77/previews/roll-00.ppm',
                'build/package12-validation/ci-SHA/ration-limit10-oldreader-normal.log',
                'build/package12-validation/ci-SHA/ration-limit10-oldreader-sanitizer.log',
                'build/package12-validation/ci-SHA/checkpoints.json',
                'build/coop-training-render/screens/invites/invite-host-30s.ppm',
                'build/coop-training-render/sanitized/invites/invite-host-30s-labels.tsv',
                'build/coop-training-render/invites/contactsheet.png']
    for path in required:
        assert selected_artifact(path), 'Required test evidence omitted: '+path
        checks += 1
    print(f'PASS: {checks} artifact-selection probes; headers/images/reports retained; host executables, objects, ELFs and OFF firmware excluded.')
    return checks

def main():
    checks = 0
    for publishing, path in ((False, '.github/workflows/wifitest77-check.yml'), (True, '.github/workflows/release.yml')):
        document = yaml.load((ROOT/path).read_text(), Loader=UniqueLoader)
        validate(document, publishing); checks += 1
        job_name = 'release' if publishing else 'validate'
        def rejected(mutator, label):
            nonlocal checks
            bad = copy.deepcopy(document)
            mutator(bad)
            try: validate(bad, publishing)
            except (AssertionError, KeyError, TypeError, ValueError): checks += 1
            else: raise AssertionError('Accepted '+label)
        def runs_replace(d, old, new):
            found = False
            for step in d['jobs'][job_name]['steps']:
                if old in step.get('run', ''):
                    step['run'] = step['run'].replace(old, new); found = True
            assert found, 'Mutation does not reach workflow: '+old
        rejected(lambda d: d['on']['push'].update(branches=['**']), 'wildcard branch')
        rejected(lambda d: d['on'].update(workflow_dispatch={}), 'manual qualification')
        rejected(lambda d: d['on'].update(pull_request={}), 'PR qualification')
        rejected(lambda d: d['on']['push'].update(paths=['firmware/**']), 'ignored gate changes')
        rejected(lambda d: d.update(permissions={'contents': 'write'}), 'unbounded permissions')
        rejected(lambda d: d['jobs'][job_name].update(**{'if': 'always()'}), 'missing exact branch/repository guard')
        rejected(lambda d: d['jobs'][job_name]['env'].update(RPG_DETECT_LEAKS='0'), 'disabled LeakSanitizer')
        rejected(lambda d: d['jobs'][job_name]['env'].update(CXXFLAGS='-DNDEBUG'), 'injected compiler flags')
        rejected(lambda d: d['jobs'][job_name]['defaults']['run'].update(shell='bash'), 'missing strict shell/pipefail')
        rejected(lambda d: d['jobs'][job_name]['steps'][0]['with'].update(**{'fetch-depth': '1'}), 'shallow ancestry')
        rejected(lambda d: d['jobs'][job_name]['steps'][0]['with'].update(**{'persist-credentials': 'true'}), 'stored checkout credential')
        for index in range(len(document['jobs'][job_name]['steps'])):
            rejected(lambda d, i=index: d['jobs'][job_name]['steps'].pop(i), 'removed mandatory step')
            rejected(lambda d, i=index: d['jobs'][job_name]['steps'][i].update(**{'if': 'false'}), 'conditionally skipped step')
            rejected(lambda d, i=index: d['jobs'][job_name]['steps'][i].update(**{'continue-on-error': 'true'}), 'ignored step failure')
        mutations = [
            ('bash tools/test_wifitest77.sh', 'bash tools/test_package11.sh'),
            ('bash tools/test_wifitest77.sh', 'bash tools/test_wifitest77.sh --focused'),
            ('core install esp32:esp32@3.3.12', 'core install esp32:esp32'),
            ('for variant in coop1-radio1 coop1-radio0 coop0-radio1', 'for variant in coop1-radio1'),
            ('for variant in coop1-radio1 coop1-radio0 coop0-radio1', 'for variant in coop1-radio0 coop0-radio1'),
            ('RPG_ENABLE_WIFI_RADIO_TEST=0','RPG_ENABLE_WIFI_RADIO_TEST=1'),
            ('extra=()', "extra=(--build-property 'compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=1')"),
            ('RPG_ENABLE_COOP_TRAINING=0', 'RPG_ENABLE_COOP_TRAINING=1'),
            ('compiler.cpp.extra_flags=', 'build.extra_flags='),
            ('arduino-cli compile --verbose --jobs 4', 'arduino-cli compile --jobs 4'),
            ('PSRAM=opi', 'PSRAM=disabled'),
            ('EraseFlash=none', 'EraseFlash=all'),
            ('PartitionScheme=app3M_fat9M_16MB', 'PartitionScheme=default'),
            ('--build-path "$RUNNER_TEMP/esp32-wifitest77/$variant"', '--build-path firmware/RPG_POKET_2/build'),
            ('--build-path "$RUNNER_TEMP/esp32-wifitest77/$variant"', '--output-dir build'),
            ('--export-binaries=false', '--export-binaries=true'),
            ('--export-binaries=false', ''),
            ('"$RUNNER_TEMP/esp32-wifitest77/coop1-radio1/RPG_POKET_2.ino.bin"', '"$RUNNER_TEMP/esp32-wifitest77/coop1-radio0/RPG_POKET_2.ino.bin"'),
            ('test "$(git rev-parse HEAD)" = "$GITHUB_SHA"', 'true'),
            ('python3 tools/check_wifitest77_scope.py', 'true'),
            ('python3 tools/release_wifitest77_manifest.py', 'true'),
            ('python3 tools/verify_wifitest77_release.py', 'true'),
            ('python3 tests/wifitest77_release_verifier.py', 'true'),
            ('python3 tools/test_wifitest77_release.py release\n', 'true\n'),
            ('python3 tools/test_wifitest77_release.py release --sanitizers', 'true'),
            ('python3 tools/record_wifitest77_identity.py', 'true'),
        ]
        if publishing:
            mutations += [
                ('head_sha=$GITHUB_SHA', 'head_sha=main'),
                ('branch=feat%2Fwifitest77-release', 'branch=main'),
                ('wifitest77-check.yml/runs', 'package11-check.yml/runs'),
                ('python3 tools/check_wifitest77_release_gate.py "$RUNNER_TEMP/qualification.json"', 'true'),
                ('gh api "repos/$GITHUB_REPOSITORY/git/ref/heads/main" > "$RUNNER_TEMP/main-ref.json"', 'true'),
                ('--main-ref "$RUNNER_TEMP/main-ref.json"', ''),
                ('git/ref/heads/main', 'git/ref/heads/feat/wifitest77-release'),
                ('--target "$GITHUB_SHA"', '--target main'),
                ('v2026.10.17-wifitest77', 'v2026.10.14-shop77'),
                ('cmp "release/$name" "build/published-wifitest77/$name"', 'true'),
                ('cmp release/manifest.json build/published-wifitest77/latest.json', 'true'),
                ('python3 tools/test_wifitest77_release.py build/published-wifitest77 --sanitizers', 'true'),
                ('test "$matched" = 1', 'true'),
            ]
        for old, new in mutations:
            rejected(lambda d, old=old, new=new: runs_replace(d, old, new), 'unsafe command mutation '+old)
        for index, item in enumerate(ARTIFACTS):
            rejected(lambda d, item=item: next(s for s in d['jobs'][job_name]['steps'] if s.get('uses') == 'actions/upload-artifact@v4')['with'].update(path='\n'.join(p for p in ARTIFACTS if p != item)), 'removed evidence '+item)
        rejected(lambda d: d['jobs'][job_name]['steps'].append({'run': 'gh release create unsafe'}), 'extra publishing step')
    for bad in ('on:\n  push: {}\non:\n  workflow_dispatch: {}\n', 'jobs:\n  validate:\n    run: true\n    run: false\n'):
        try: yaml.load(bad, Loader=UniqueLoader)
        except AssertionError: checks += 1
        else: raise AssertionError('Accepted duplicate YAML mapping key')
    # Prior package11 tooling and all prior qualification workflows stay frozen.
    historical = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', BASELINE], cwd=ROOT, text=True).splitlines()
    for name in historical:
        if any(s in Path(name).name.lower() for s in ('package11','package12','coopfix77','coopdiag77')) or (name.startswith('.github/workflows/') and name != '.github/workflows/release.yml'):
            assert (ROOT/name).read_bytes() == subprocess.check_output(['git', 'show', BASELINE+':'+name], cwd=ROOT), 'Historical gate changed: '+name
            checks += 1
    from check_package12_scope import assert_rules_compatibility, BASELINE as RULES_BASELINE
    rules_path='firmware/RPG_POKET_2/Rules.h'
    before=subprocess.check_output(['git','show',RULES_BASELINE+':'+rules_path],cwd=ROOT)
    after=(ROOT/rules_path).read_bytes()
    assert_rules_compatibility(before,after)
    checks += 1
    for changed in (before, after+b'\n', after.replace(b'day[]={6,25,6,26}',b'day[]={6,25,6,27}'),
                    after.replace(b'dark[]={28,7,27,28}',b'dark[]={28,7,27,7}'),
                    after.replace(b':day[n%4]',b':day[n%3]'),
                    after.replace(b'return night?dark',b'return !night?dark'),
                    after.replace(b'x^=x<<13',b'x^=x<<12'),
                    after.replace(b'RATION_LIMIT=10',b'RATION_LIMIT=11')):
        assert changed != after
        try: assert_rules_compatibility(before,changed)
        except AssertionError: checks += 1
        else: raise AssertionError('Accepted unrelated Rules mutation')
    try: assert_rules_compatibility(before+b'\n',after)
    except AssertionError: checks += 1
    else: raise AssertionError('Accepted altered Rules baseline')
    checks += aggregate_probes()
    checks += artifact_selection_probes()
    print(f'PASS: {checks} wifitest77 workflow mutation/frozen-history checks; exact branch/SHA, complete retest, LSan1, default COOP1/RADIO1 + both explicit-OFF target variants, no export, public/latest byte comparison.')


if __name__ == '__main__':
    main()
