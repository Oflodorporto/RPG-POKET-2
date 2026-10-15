#!/usr/bin/env python3
"""Fail-closed coopfix77 workflow, inherited plan and evidence wiring probes.

This configuration test never executes sanitizer runtimes, Arduino or a remote
write. The actual full runner, target builds and real-byte OTA are separate gates.
"""
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
BASELINE = 'd767538669137ca08a2dd53f3805e8ba1be006b8'
FQBN = 'esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,EraseFlash=none'
PATHS = ['firmware/**', 'assets/**', 'cartao/**', 'docs/**', 'tests/**', 'tools/**', 'README.md', 'COOPFIX77_STAGING_ONLY.txt', '.github/workflows/coopfix77-check.yml', '.github/workflows/release.yml']
ARTIFACTS = '''release/firmware.bin
release/artes.pak
release/manifest.json
release/SHA256SUMS
build/coopfix77-scope-budget.json
build/coopfix77-source-identity.json
build/coopfix77-tests.log
build/coopfix77-esp32*.json
build/coopfix77-esp32*.log
build/coopfix77-esp32*.txt
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
build/coop-invite-countdown/*.json'''.splitlines()
CORE = '''arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.12'''
COMPILE = f'''test "$(git rev-parse HEAD)" = "$GITHUB_SHA"
python3 tools/check_coopfix77_scope.py
arduino-cli version | tee build/coopfix77-esp32-toolchain.txt
arduino-cli core list | tee -a build/coopfix77-esp32-toolchain.txt
for flag in 1 0; do
  extra=()
  if [[ "$flag" = 0 ]]; then extra+=(--build-property 'compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=0'); fi
  arduino-cli compile --verbose --jobs 4 \\
    --fqbn '{FQBN}' \\
    "${{extra[@]}}" \\
    --build-path "$RUNNER_TEMP/esp32-coopfix77/flag-$flag" \\
    --export-binaries=false firmware/RPG_POKET_2 2>&1 | tee "build/coopfix77-esp32-flag-$flag.log"
done
cp "$RUNNER_TEMP/esp32-coopfix77/flag-1/RPG_POKET_2.ino.bin" build/RPG_POKET_2.ino.bin
python3 tools/check_coopfix77_scope.py'''
VERIFY = '''python3 tools/release_coopfix77_manifest.py
python3 tools/verify_coopfix77_release.py
python3 tests/coopfix77_release_verifier.py
python3 tools/test_coopfix77_release.py release
python3 tools/test_coopfix77_release.py release --sanitizers
python3 tools/check_coopfix77_scope.py'''
GATE = '''test "$GITHUB_REPOSITORY" = Oflodorporto/RPG-POKET-2
test "$GITHUB_EVENT_NAME" = push
test "$GITHUB_REF" = refs/heads/main
gh api "repos/$GITHUB_REPOSITORY/actions/workflows/coopfix77-check.yml/runs?branch=feat%2Fcoopfix77-release&head_sha=$GITHUB_SHA&status=success&per_page=100" > "$RUNNER_TEMP/qualification.json"
python3 tools/check_coopfix77_release_gate.py "$RUNNER_TEMP/qualification.json"
test "$(gh api "repos/$GITHUB_REPOSITORY/git/matching-refs/tags/v2026.10.15-coopfix77" --jq 'length')" = 0'''
PUBLISH = GATE+'''\npython3 -c 'import sys,subprocess; sys.path.insert(0,"tools"); from record_coopfix77_identity import assert_generated_only; assert_generated_only(subprocess.check_output(["git","status","--porcelain","--untracked-files=all"],text=True).strip())'
gh api "repos/$GITHUB_REPOSITORY/git/ref/heads/main" > "$RUNNER_TEMP/main-ref.json"
python3 tools/check_coopfix77_release_gate.py "$RUNNER_TEMP/qualification.json" --main-ref "$RUNNER_TEMP/main-ref.json"
gh release create v2026.10.15-coopfix77 release/firmware.bin release/artes.pak release/manifest.json --target "$GITHUB_SHA" --title 'RPG POKET - 7.7 - Treino cooperativo: toque, saida e convite' --notes-file docs/RELEASE_COOPFIX77_NOTAS.txt --latest'''
DOWNLOAD = '''mkdir -p build/published-coopfix77
for name in firmware.bin artes.pak manifest.json; do
  curl --fail --location --retry 5 --retry-all-errors --output "build/published-coopfix77/$name" "https://github.com/$GITHUB_REPOSITORY/releases/download/v2026.10.15-coopfix77/$name"
  cmp "release/$name" "build/published-coopfix77/$name"
done
python3 tools/verify_coopfix77_release.py build/published-coopfix77
python3 tools/test_coopfix77_release.py build/published-coopfix77
python3 tools/test_coopfix77_release.py build/published-coopfix77 --sanitizers
matched=0
for attempt in $(seq 1 12); do
  if curl --fail --location --retry 2 --output build/published-coopfix77/latest.json "https://github.com/$GITHUB_REPOSITORY/releases/latest/download/manifest.json" && cmp release/manifest.json build/published-coopfix77/latest.json; then matched=1; break; fi
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
    branch = 'main' if publishing else 'feat/coopfix77-release'
    assert document['on'] == {'push': {'branches': [branch], 'paths': PATHS}}
    assert document['permissions'] == ({'contents': 'write', 'actions': 'read'} if publishing else {'contents': 'read'})
    assert document['concurrency'] == {'group': 'coopfix77-main' if publishing else 'coopfix77-${{ github.ref }}', 'cancel-in-progress': 'false' if publishing else 'true'}
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
    run('mkdir -p build\nbash tools/test_coopfix77.sh 2>&1 | tee build/coopfix77-tests.log')
    expected.append({'uses': 'arduino/setup-arduino-cli@v2'})
    run(CORE)
    run(COMPILE)
    run(VERIFY)
    run('python3 tools/record_coopfix77_identity.py --build-root "$RUNNER_TEMP/esp32-coopfix77"')
    expected.append({'uses': 'actions/upload-artifact@v4', 'with': {
        'name': 'coopfix77-'+('publisher-' if publishing else '')+'${{ github.sha }}',
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
    assert 'for flag in 1 0' in all_runs



HOTFIX_TASKS = [('coop-touch-invite-terminal-regression', [
    ['python3', 'tools/test_coop_touch_queue_regression.py', '--mode', 'both', '--output-dir', 'build/coop-touch-queue'],
    ['python3', 'tools/test_coop_invite_countdown_regression.py', '--mode', 'both', '--output-dir', 'build/coop-invite-countdown'],
    ['python3', 'tools/check_coopfix77_invite_render.py'],
])]

def validate_aggregate(wrapper, plan, runner):
    from test_package12_local import tasks as inherited_tasks
    inherited = inherited_tasks(False)
    assert len(inherited) == 151 and len(plan) == len(dict(plan)) == 152
    assert plan == inherited + HOTFIX_TASKS, 'Dropped, reordered or changed inherited/hotfix command'
    expected_wrapper = (ROOT/'tools/test_package12.sh').read_text().replace('package12', 'coopfix77')
    expected_wrapper = expected_wrapper.replace('tests/coopfix77_coop_config.py', 'tests/package12_coop_config.py')
    expected_wrapper = expected_wrapper.replace('python3 tests/coopfix77_qualification_config.py\n', 'python3 tests/coopfix77_qualification_config.py\npython3 tests/coopfix77_scope.py\npython3 tests/coopfix77_invite_render.py\n')
    assert wrapper == expected_wrapper, 'Mandatory wrapper operation changed'
    for required in (
        'import test_package12_local as inherited', 'INHERITED_TASKS = inherited.tasks',
        'plan = INHERITED_TASKS(False)', 'return plan + [HOTFIX_TASK]',
        "assert focused is False", "assert '--focused' not in sys.argv",
        "assert os.environ.get('RPG_DETECT_LEAKS', '1') == '1'",
        'inherited.tasks = tasks', 'inherited.main()',
        'finally:', 'inherited.tasks = INHERITED_TASKS',
    ):
        assert required in runner, required
    for forbidden in ('subprocess', 'detect_leaks=0', 'exec(', 'compile(', 'shell=True'):
        assert forbidden not in runner, forbidden

def aggregate_probes():
    import test_coopfix77_local as local
    import package12_qualification_config as historical
    # Execute every frozen package12 plan/resume/serialization mutation probe.
    count = historical.aggregate_probes()
    wrapper = (ROOT/'tools/test_coopfix77.sh').read_text()
    runner = (ROOT/'tools/test_coopfix77_local.py').read_text()
    plan = local.tasks(False)
    validate_aggregate(wrapper, plan, runner); count += 1
    def rejects(w=wrapper, p=plan, r=runner):
        nonlocal count
        try: validate_aggregate(w, p, r)
        except (AssertionError, ValueError, KeyError): count += 1
        else: raise AssertionError('Accepted changed hotfix qualification plan')
    for index, (name, commands) in enumerate(plan):
        rejects(p=plan[:index]+plan[index+1:])
        bad=copy.deepcopy(plan);bad[index][1][0].append('--unapproved');rejects(p=bad)
    rejects(p=list(reversed(plan)))
    for old, new in (
        ('test "$RPG_DETECT_LEAKS" = 1', 'true'), ('--sanitizers', ''),
        ('python3 tests/coopfix77_build_evidence.py', 'true'),
        ('python3 tests/package12_coop_config.py', 'true'),
        ('python3 tools/capture_bulk_commerce.py', 'true'),
        ('set -euo pipefail', 'set -eu'),
    ): rejects(w=wrapper.replace(old,new))
    rejects(r=runner.replace('inherited.main()', 'pass'))
    rejects(r=runner.replace("assert focused is False", 'assert True'))
    rejects(r=runner.replace("assert os.environ.get('RPG_DETECT_LEAKS', '1') == '1'", 'assert True'))
    for args, leaks in ((['--focused'], '1'), ([], '0')):
        with patch.dict(os.environ, {'RPG_DETECT_LEAKS': leaks}), patch.object(sys, 'argv', ['runner','--run-id','offline',*args]), patch.object(local.inherited, 'main') as intercepted:
            try: local.main()
            except AssertionError: count += 1
            else: raise AssertionError('Accepted focused or leak-suppressed hotfix qualification')
            intercepted.assert_not_called()
    with patch.dict(os.environ, {'RPG_DETECT_LEAKS':'1'}), patch.object(sys, 'argv', ['runner','--run-id','offline']), patch.object(local.inherited, 'main') as intercepted:
        local.main();intercepted.assert_called_once();count += 1
    assert local.inherited.tasks is local.INHERITED_TASKS
    # Exercise the unchanged real scheduler with mocked commands: the appended
    # coop- group may inspect frames only after inherited coop-render-noalloc.
    integrated = [name for name, _ in plan if name.startswith('coop-')]
    assert integrated == ['coop-integrated-on-off', 'coop-ota-installer-multitu',
                          'coop-render-noalloc', HOTFIX_TASKS[0][0]]
    with tempfile.TemporaryDirectory(prefix='coopfix77-render-order-') as temporary:
        folder = Path(temporary); seen = []; ready = [False]
        selected = [('legacy-a', [['probe','a']]), ('legacy-b', [['probe','b']]),
                    ('coop-render-noalloc', [['probe','render']]), HOTFIX_TASKS[0]]
        fingerprint = {'sha256':'a'*64,'files':{},'detect_leaks':'1'}
        def pretend_run(command, **kwargs):
            if command[0] == 'probe':
                if command[1] == 'render':
                    assert set(seen) == {'a','b'}, 'Renderer raced inherited pool'
                    ready[0] = True
                seen.append(command[1])
            elif len(command)>1 and command[1].startswith('tools/test_coop_'):
                assert ready[0], 'Hotfix regression raced renderer'
                seen.append(command[1])
            elif len(command)>1 and command[1] == 'tools/check_coopfix77_invite_render.py':
                assert ready[0] and seen[-2:] == [HOTFIX_TASKS[0][1][0][1],HOTFIX_TASKS[0][1][1][1]]
                seen.append('verified-invite-render')
            return subprocess.CompletedProcess(command,0)
        cwd=Path.cwd()
        try:
            with patch.dict(os.environ, {'RPG_DETECT_LEAKS':'1'}), patch.object(sys,'argv',['runner','--run-id','offline-order','--jobs','3']), patch.object(local.inherited,'ROOT',folder), patch.object(local.inherited,'tasks',return_value=selected), patch.object(local.inherited,'inventory',return_value=fingerprint), patch.object(local.inherited.subprocess,'run',side_effect=pretend_run):
                local.inherited.main()
            summary=json.loads((folder/'build/package12-validation/offline-order/summary.json').read_text())
            assert summary['all_passed'] is True and summary['tasks']==5 and seen[-1]=='verified-invite-render'
            count += 1
        finally: os.chdir(cwd)
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
        'build/coopfix77-esp32/flag-0/RPG_POKET_2.ino.bin',
        'build/coopfix77-esp32-flag-0.bin',
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
    for publishing, path in ((False, '.github/workflows/coopfix77-check.yml'), (True, '.github/workflows/release.yml')):
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
            ('bash tools/test_coopfix77.sh', 'bash tools/test_package11.sh'),
            ('bash tools/test_coopfix77.sh', 'bash tools/test_coopfix77.sh --focused'),
            ('core install esp32:esp32@3.3.12', 'core install esp32:esp32'),
            ('for flag in 1 0', 'for flag in 1'),
            ('for flag in 1 0', 'for flag in 0'),
            ('extra=()', "extra=(--build-property 'compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=1')"),
            ('RPG_ENABLE_COOP_TRAINING=0', 'RPG_ENABLE_COOP_TRAINING=1'),
            ('compiler.cpp.extra_flags=', 'build.extra_flags='),
            ('arduino-cli compile --verbose --jobs 4', 'arduino-cli compile --jobs 4'),
            ('PSRAM=opi', 'PSRAM=disabled'),
            ('EraseFlash=none', 'EraseFlash=all'),
            ('PartitionScheme=app3M_fat9M_16MB', 'PartitionScheme=default'),
            ('--build-path "$RUNNER_TEMP/esp32-coopfix77/flag-$flag"', '--build-path firmware/RPG_POKET_2/build'),
            ('--build-path "$RUNNER_TEMP/esp32-coopfix77/flag-$flag"', '--output-dir build'),
            ('--export-binaries=false', '--export-binaries=true'),
            ('--export-binaries=false', ''),
            ('"$RUNNER_TEMP/esp32-coopfix77/flag-1/RPG_POKET_2.ino.bin"', '"$RUNNER_TEMP/esp32-coopfix77/flag-0/RPG_POKET_2.ino.bin"'),
            ('test "$(git rev-parse HEAD)" = "$GITHUB_SHA"', 'true'),
            ('python3 tools/check_coopfix77_scope.py', 'true'),
            ('python3 tools/release_coopfix77_manifest.py', 'true'),
            ('python3 tools/verify_coopfix77_release.py', 'true'),
            ('python3 tests/coopfix77_release_verifier.py', 'true'),
            ('python3 tools/test_coopfix77_release.py release\n', 'true\n'),
            ('python3 tools/test_coopfix77_release.py release --sanitizers', 'true'),
            ('python3 tools/record_coopfix77_identity.py', 'true'),
        ]
        if publishing:
            mutations += [
                ('head_sha=$GITHUB_SHA', 'head_sha=main'),
                ('branch=feat%2Fcoopfix77-release', 'branch=main'),
                ('coopfix77-check.yml/runs', 'package11-check.yml/runs'),
                ('python3 tools/check_coopfix77_release_gate.py "$RUNNER_TEMP/qualification.json"', 'true'),
                ('gh api "repos/$GITHUB_REPOSITORY/git/ref/heads/main" > "$RUNNER_TEMP/main-ref.json"', 'true'),
                ('--main-ref "$RUNNER_TEMP/main-ref.json"', ''),
                ('git/ref/heads/main', 'git/ref/heads/feat/coopfix77-release'),
                ('--target "$GITHUB_SHA"', '--target main'),
                ('v2026.10.15-coopfix77', 'v2026.10.14-shop77'),
                ('cmp "release/$name" "build/published-coopfix77/$name"', 'true'),
                ('cmp release/manifest.json build/published-coopfix77/latest.json', 'true'),
                ('python3 tools/test_coopfix77_release.py build/published-coopfix77 --sanitizers', 'true'),
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
        if any(s in Path(name).name.lower() for s in ('package11','package12')) or (name.startswith('.github/workflows/') and name != '.github/workflows/release.yml'):
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
    print(f'PASS: {checks} coopfix77 workflow mutation/frozen-history checks; exact branch/SHA, complete retest, LSan1, defaultON + explicitOFF target build, no export, public/latest byte comparison.')


if __name__ == '__main__':
    main()
