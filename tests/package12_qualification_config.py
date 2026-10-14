#!/usr/bin/env python3
"""Fail-closed package12 workflow, inherited plan and evidence wiring probes.

This configuration test never executes sanitizer runtimes, Arduino or a remote
write. The actual full runner, target builds and real-byte OTA are separate gates.
"""
from pathlib import Path
import copy
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
BASELINE = '136c748fd35aebd2df7c9bdaddd01d224646fddd'
FQBN = 'esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,EraseFlash=none'
PATHS = ['firmware/**', 'assets/**', 'cartao/**', 'docs/**', 'tests/**', 'tools/**', 'README.md', 'PACKAGE12_STAGING_ONLY.txt', '.github/workflows/package12-check.yml', '.github/workflows/release.yml']
ARTIFACTS = '''release/firmware.bin
release/artes.pak
release/manifest.json
release/SHA256SUMS
build/package12-scope-budget.json
build/package12-source-identity.json
build/package12-tests.log
build/package12-esp32*
build/package12-validation/**
build/package7-differential*.json
build/screens/procedural6/*
build/screens/package6-integration/*
build/screens/camp-identity77/*
build/d20-camp77/previews/*
build/ruins-bag77/ruins-bag-*.png
build/ruins7/*.png
build/clock-cycle7/previews/*
build/clock-cycle7/visibility-preview/*
build/fatigue7/screens/*
build/narrative8-render/*.json
build/narrative8-render/*.jsonl
build/narrative8-render/*.html
build/narrative8-render/*.txt
build/narrative8-render/*.log
build/narrative8-render/png/*
build/narrative8-render/contact-sheets/*
build/narrative8-events/*
build/pvp-presentation/previews/*
build/pvp-presentation/capture-manifest.json
build/pvp-presentation/screens/*.txt
build/pvp-presentation/sanitized/*.txt
build/pvp-review/screens-normal/*
build/pvp-review/screens-sanitizer/*
build/bulk-shop/screens/*.png
build/bulk-shop/screens/*.ppm
build/bulk-shop/sanitized/*.ppm
build/ration-limit10-oldreader/**
build/coop-training-native/**/*.log
build/coop-training-native/**/*.json
build/coop-training-native/**/*.txt
build/coop-training-native/**/*.d
build/coop-training-render/**/*.png
build/coop-training-render/**/*.ppm
build/coop-training-render/**/*.json
build/coop-training-render/**/*.log
build/coop-training-render/**/*.tsv'''.splitlines()
CORE = '''arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.12'''
COMPILE = f'''test "$(git rev-parse HEAD)" = "$GITHUB_SHA"
python3 tools/check_package12_scope.py
arduino-cli version | tee build/package12-esp32-toolchain.txt
arduino-cli core list | tee -a build/package12-esp32-toolchain.txt
for flag in 1 0; do
  extra=()
  if [[ "$flag" = 0 ]]; then extra+=(--build-property 'compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=0'); fi
  arduino-cli compile --verbose --jobs 4 \\
    --fqbn '{FQBN}' \\
    "${{extra[@]}}" \\
    --build-path "$RUNNER_TEMP/esp32-package12/flag-$flag" \\
    --export-binaries=false firmware/RPG_POKET_2 2>&1 | tee "build/package12-esp32-flag-$flag.log"
done
cp "$RUNNER_TEMP/esp32-package12/flag-1/RPG_POKET_2.ino.bin" build/RPG_POKET_2.ino.bin
python3 tools/check_package12_scope.py'''
VERIFY = '''python3 tools/release_package12_manifest.py
python3 tools/verify_package12_release.py
python3 tests/package12_release_verifier.py
python3 tools/test_package12_release.py release
python3 tools/test_package12_release.py release --sanitizers
python3 tools/check_package12_scope.py'''
GATE = '''test "$GITHUB_REPOSITORY" = Oflodorporto/RPG-POKET-2
test "$GITHUB_EVENT_NAME" = push
test "$GITHUB_REF" = refs/heads/main
gh api "repos/$GITHUB_REPOSITORY/actions/workflows/package12-check.yml/runs?branch=feat%2Fpackage12-release&head_sha=$GITHUB_SHA&status=success&per_page=100" > "$RUNNER_TEMP/qualification.json"
python3 tools/check_package12_release_gate.py "$RUNNER_TEMP/qualification.json"
test "$(gh api "repos/$GITHUB_REPOSITORY/git/matching-refs/tags/v2026.10.14-coop77" --jq 'length')" = 0'''
PUBLISH = GATE+'''\npython3 -c 'import sys,subprocess; sys.path.insert(0,"tools"); from record_package12_identity import assert_generated_only; assert_generated_only(subprocess.check_output(["git","status","--porcelain","--untracked-files=all"],text=True).strip())'
gh api "repos/$GITHUB_REPOSITORY/git/ref/heads/main" > "$RUNNER_TEMP/main-ref.json"
python3 tools/check_package12_release_gate.py "$RUNNER_TEMP/qualification.json" --main-ref "$RUNNER_TEMP/main-ref.json"
gh release create v2026.10.14-coop77 release/firmware.bin release/artes.pak release/manifest.json --target "$GITHUB_SHA" --title 'RPG POKET - 7.7 - Treino cooperativo experimental' --notes-file docs/RELEASE_PACKAGE12_NOTAS.txt --latest'''
DOWNLOAD = '''mkdir -p build/published-package12
for name in firmware.bin artes.pak manifest.json; do
  curl --fail --location --retry 5 --retry-all-errors --output "build/published-package12/$name" "https://github.com/$GITHUB_REPOSITORY/releases/download/v2026.10.14-coop77/$name"
  cmp "release/$name" "build/published-package12/$name"
done
python3 tools/verify_package12_release.py build/published-package12
python3 tools/test_package12_release.py build/published-package12
python3 tools/test_package12_release.py build/published-package12 --sanitizers
matched=0
for attempt in $(seq 1 12); do
  if curl --fail --location --retry 2 --output build/published-package12/latest.json "https://github.com/$GITHUB_REPOSITORY/releases/latest/download/manifest.json" && cmp release/manifest.json build/published-package12/latest.json; then matched=1; break; fi
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
    branch = 'main' if publishing else 'feat/package12-release'
    assert document['on'] == {'push': {'branches': [branch], 'paths': PATHS}}
    assert document['permissions'] == ({'contents': 'write', 'actions': 'read'} if publishing else {'contents': 'read'})
    assert document['concurrency'] == {'group': 'package12-main' if publishing else 'package12-${{ github.ref }}', 'cancel-in-progress': 'false' if publishing else 'true'}
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
    run('mkdir -p build\nbash tools/test_package12.sh 2>&1 | tee build/package12-tests.log')
    expected.append({'uses': 'arduino/setup-arduino-cli@v2'})
    run(CORE)
    run(COMPILE)
    run(VERIFY)
    run('python3 tools/record_package12_identity.py --build-root "$RUNNER_TEMP/esp32-package12"')
    expected.append({'uses': 'actions/upload-artifact@v4', 'with': {
        'name': 'package12-'+('publisher-' if publishing else '')+'${{ github.sha }}',
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



COOP_TASKS = {
    'coop-integrated-on-off': [['python3', 'tools/test_package12_coop.py', '--mode', 'both', '--output-dir', 'build/coop-training-native']],
    'coop-ota-installer-multitu': [['python3', 'tools/test_coop_update_install.py', '--mode', 'both', '--output-dir', 'build/coop-training-native']],
    'coop-render-noalloc': [['python3', 'tools/capture_package12_coop.py', '--out', 'build/coop-training-render', '--art', 'cartao/RPGPOKET/artes.pak']],
}
OTA_CLIENTS = {'ota_recovery', 'ota_art_readback', 'update_adapter', 'enemy_ota', 'ota_sd_diagnostics'}


def validate_aggregate(wrapper, plan, runner):
    from test_package11_local import tasks as inherited_tasks
    inherited = dict(inherited_tasks(False))
    current = dict(plan)
    assert len(plan) == len(current) == 151 and len(inherited) == 148
    assert set(current) == set(inherited) | set(COOP_TASKS)
    for name, commands in inherited.items():
        expected = []
        for command in commands:
            command = [part.replace('package11-binaries', 'package12-binaries') for part in command]
            if name in OTA_CLIENTS and command[0] == 'g++':
                source_index = command.index(f'tests/{name}.cpp')
                added = ['firmware/RPG_POKET_2/UpdateInstall.cpp']
                if name in {'ota_recovery', 'ota_art_readback'}:
                    added.append('-DRPG_TEST_REAL_SHA256=')
                command[source_index+1:source_index+1] = added
            expected.append(command)
        assert current[name] == expected, 'Inherited coverage/flags/fixture changed: '+name
    for name, commands in COOP_TASKS.items():
        assert current[name] == commands, 'Missing integrated normal+LSan/render gate: '+name
    assert wrapper.count('python3 -u tools/test_package12_local.py') == 1 and '--focused' not in wrapper
    for required in (
        'set -euo pipefail', 'test "$#" = 0',
        'export RPG_DETECT_LEAKS="${RPG_DETECT_LEAKS:-1}"',
        'test "$RPG_DETECT_LEAKS" = 1', 'export PYTHONDONTWRITEBYTECODE=1',
        'python3 tests/package12_qualification_config.py',
        'python3 tests/package12_coop_config.py',
        'python3 tests/package12_build_evidence.py',
        'python3 tests/coop_update_install_equivalence.py',
        'python3 tests/package12_release_gate.py',
        'python3 tests/package12_release_verifier.py --synthetic',
        'python3 tools/test_package12_release.py --synthetic-fixture\n',
        'python3 tools/test_package12_release.py --synthetic-fixture --sanitizers',
        'from test_package12_release import published_lineage; published_lineage()',
    ):
        assert required in wrapper, required
    for forbidden in ('detect_leaks=0', '|| true', 'set +e', 'test_package11.sh', 'tests/package11_qualification_config.py', 'tools/check_coop_training_scope.py', 'bash tools/test_coop_training.sh'):
        assert forbidden not in wrapper, forbidden
    # Preserve every post-aggregate historical capture and frame assertion.
    old = (ROOT/'tools/test_package11.sh').read_text()
    marker = 'python3 tools/capture_d20_camp77.py'
    assert wrapper[wrapper.index(marker):] == old[old.index(marker):].replace('package11', 'package12')
    assert wrapper.count('python3 tools/check_package12_scope.py') == 2
    for required in (
        "assert os.environ.get('RPG_DETECT_LEAKS','1') == '1'",
        "assert not a.focused", "os.environ.get('RPG_DETECT_LEAKS') == '1'",
        "LSAN_OPTIONS='exitcode=23'", "NARRATIVE8_SKIP_IMAGES='1'",
        'Checkpoint log missing or changed', 'Checkpoint commands changed',
        'Frozen input changed; create a new run-id', 'Source changed during validation',
        "with oldreader_lock if name.startswith('ration-limit10-oldreader-')",
        "legacy=[item for item in selected if not item[0].startswith('coop-')]",
        "integrated=[item for item in selected if item[0].startswith('coop-')]",
        'pool.map(run,legacy)', 'results.extend(run(item) for item in integrated)',
    ):
        assert required in runner, required
    assert runner.index('pool.map(run,legacy)') < runner.index('results.extend(run(item) for item in integrated)')
    old_runner = (ROOT/'tools/test_package11_local.py').read_text()
    def preparation(text):
        return text[text.index('    prep='):text.index("    if not run(('preparation',prep))")]
    assert preparation(runner) == preparation(old_runner), 'Inherited preparation changed'


def aggregate_probes():
    import test_package12_local as local
    wrapper = (ROOT/'tools/test_package12.sh').read_text()
    runner = (ROOT/'tools/test_package12_local.py').read_text()
    plan = local.tasks(False)
    validate_aggregate(wrapper, plan, runner)
    count = 1
    def rejects(w=wrapper, p=plan, r=runner):
        nonlocal count
        try: validate_aggregate(w, p, r)
        except (AssertionError, ValueError, KeyError): count += 1
        else: raise AssertionError('Accepted missing/changed inherited or integrated gate')
    for name, _ in plan:
        rejects(p=[item for item in plan if item[0] != name])
    for old, new in (
        ('test "$RPG_DETECT_LEAKS" = 1', 'true'),
        ('--sanitizers', ''),
        ('python3 tests/package12_coop_config.py', 'true'),
        ('python3 tests/package12_build_evidence.py', 'true'),
        ('python3 tests/coop_update_install_equivalence.py', 'true'),
        ('python3 tools/capture_bulk_commerce.py', 'true'),
        ('rendered_frames"] == 3481', 'rendered_frames"] > 0'),
        ('set -euo pipefail', 'set -eu'),
    ):
        assert old in wrapper
        rejects(w=wrapper.replace(old, new))
    for name in COOP_TASKS:
        bad = copy.deepcopy(plan)
        index = next(i for i, item in enumerate(bad) if item[0] == name)
        command = bad[index][1][0]
        if '--mode' in command: command[command.index('--mode')+1] = 'normal'
        else: command += ['--no-sanitize']
        rejects(p=bad)
    for old, new in (("LSAN_OPTIONS='exitcode=23'", "LSAN_OPTIONS='exitcode=0'"), ('results.extend(run(item) for item in integrated)', 'pass'), ('Checkpoint log missing or changed', 'unused warning')):
        rejects(r=runner.replace(old, new))
    # Probe executable runner restrictions without launching any compiler/runtime.
    for focused, leaks in ((True, '1'), (False, '0')):
        arguments = ['runner', '--run-id', 'config-only']+(['--focused'] if focused else [])
        with patch.dict(os.environ, {'GITHUB_ACTIONS': 'true', 'RPG_DETECT_LEAKS': leaks}), patch.object(sys, 'argv', arguments):
            try: local.main()
            except AssertionError: count += 1
            else: raise AssertionError('Accepted focused CI or disabled LSan')
    # Exercise resumability, hashes, immutable plan, fixture serialization and
    # post-inherited integrated ordering using only mocked commands in tempdir.
    with tempfile.TemporaryDirectory(prefix='package12-checkpoints-') as temporary:
        folder = Path(temporary)
        fingerprint = {'sha256': 'a'*64, 'files': {'frozen.cpp': 'b'*64}, 'detect_leaks': '1'}
        selected = [('ration-limit10-oldreader-normal', [['probe', 'normal']]), ('ration-limit10-oldreader-sanitizer', [['probe', 'sanitizer']]), ('coop-integrated-on-off', [['probe', 'coop']])]
        seen = []; active = [0]; mutex = threading.Lock()
        def pretend_run(command, **kwargs):
            if command[0] == 'probe':
                with mutex:
                    active[0] += 1
                    assert active[0] == 1, 'Shared fixture readers raced'
                    if command[1] == 'coop': assert sorted(seen) == ['normal', 'sanitizer'], 'Integrated stage ran before inherited completion'
                time.sleep(0.005)
                with mutex: seen.append(command[1]); active[0] -= 1
            return subprocess.CompletedProcess(command, 0)
        cwd = Path.cwd()
        try:
            with patch.dict(os.environ, {'GITHUB_ACTIONS': 'false', 'RPG_DETECT_LEAKS': '1'}), patch.object(sys, 'argv', ['runner', '--run-id', 'fixture', '--jobs', '2']), patch.object(local, 'ROOT', folder), patch.object(local, 'tasks', return_value=selected), patch.object(local, 'inventory', return_value=fingerprint), patch.object(local.subprocess, 'run', side_effect=pretend_run):
                local.main(); count += 1
                output = folder/'build/package12-validation/fixture'
                summary = json.loads((output/'summary.json').read_text())
                assert summary['all_passed'] and summary['tasks'] == 4 and summary['lsan'] == 'detect_leaks=1'
                local.main(); assert len(seen) == 3; count += 1
                def invalid_run():
                    nonlocal count
                    try: local.main()
                    except AssertionError: count += 1
                    else: raise AssertionError('Accepted corrupted checkpoint identity')
                with patch.object(local, 'inventory', return_value={'sha256': 'c'*64}): invalid_run()
                with patch.object(local, 'tasks', return_value=selected+[('extra', [['probe', 'extra']])]): invalid_run()
                log = output/'ration-limit10-oldreader-normal.log'
                original = log.read_bytes(); log.write_bytes(b'forged success'); invalid_run(); log.write_bytes(original)
                checkpoint = output/'checkpoints.json'; state = json.loads(checkpoint.read_text())
                state['ration-limit10-oldreader-normal']['commands'] = [['probe', 'changed']]
                checkpoint.write_text(json.dumps(state)); invalid_run()
        finally: os.chdir(cwd)
    return count

def main():
    checks = 0
    for publishing, path in ((False, '.github/workflows/package12-check.yml'), (True, '.github/workflows/release.yml')):
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
            ('bash tools/test_package12.sh', 'bash tools/test_package11.sh'),
            ('bash tools/test_package12.sh', 'bash tools/test_package12.sh --focused'),
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
            ('--build-path "$RUNNER_TEMP/esp32-package12/flag-$flag"', '--build-path firmware/RPG_POKET_2/build'),
            ('--build-path "$RUNNER_TEMP/esp32-package12/flag-$flag"', '--output-dir build'),
            ('--export-binaries=false', '--export-binaries=true'),
            ('--export-binaries=false', ''),
            ('"$RUNNER_TEMP/esp32-package12/flag-1/RPG_POKET_2.ino.bin"', '"$RUNNER_TEMP/esp32-package12/flag-0/RPG_POKET_2.ino.bin"'),
            ('test "$(git rev-parse HEAD)" = "$GITHUB_SHA"', 'true'),
            ('python3 tools/check_package12_scope.py', 'true'),
            ('python3 tools/release_package12_manifest.py', 'true'),
            ('python3 tools/verify_package12_release.py', 'true'),
            ('python3 tests/package12_release_verifier.py', 'true'),
            ('python3 tools/test_package12_release.py release\n', 'true\n'),
            ('python3 tools/test_package12_release.py release --sanitizers', 'true'),
            ('python3 tools/record_package12_identity.py', 'true'),
        ]
        if publishing:
            mutations += [
                ('head_sha=$GITHUB_SHA', 'head_sha=main'),
                ('branch=feat%2Fpackage12-release', 'branch=main'),
                ('package12-check.yml/runs', 'package11-check.yml/runs'),
                ('python3 tools/check_package12_release_gate.py "$RUNNER_TEMP/qualification.json"', 'true'),
                ('gh api "repos/$GITHUB_REPOSITORY/git/ref/heads/main" > "$RUNNER_TEMP/main-ref.json"', 'true'),
                ('--main-ref "$RUNNER_TEMP/main-ref.json"', ''),
                ('git/ref/heads/main', 'git/ref/heads/feat/package12-release'),
                ('--target "$GITHUB_SHA"', '--target main'),
                ('v2026.10.14-coop77', 'v2026.10.14-shop77'),
                ('cmp "release/$name" "build/published-package12/$name"', 'true'),
                ('cmp release/manifest.json build/published-package12/latest.json', 'true'),
                ('python3 tools/test_package12_release.py build/published-package12 --sanitizers', 'true'),
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
        if 'package11' in Path(name).name.lower() or (name.startswith('.github/workflows/') and name != '.github/workflows/release.yml'):
            assert (ROOT/name).read_bytes() == subprocess.check_output(['git', 'show', BASELINE+':'+name], cwd=ROOT), 'Historical gate changed: '+name
            checks += 1
    from check_package12_scope import assert_rules_compatibility
    rules_path='firmware/RPG_POKET_2/Rules.h'
    before=subprocess.check_output(['git','show',BASELINE+':'+rules_path],cwd=ROOT)
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
    print(f'PASS: {checks} package12 workflow mutation/frozen-history checks; exact branch/SHA, complete retest, LSan1, defaultON + explicitOFF target build, no export, public/latest byte comparison.')


if __name__ == '__main__':
    main()
