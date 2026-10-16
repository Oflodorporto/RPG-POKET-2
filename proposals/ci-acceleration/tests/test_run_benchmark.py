#!/usr/bin/env python3
"""Offline mock/fixture tests only. Never execute ESP32, OTA or host product gates."""
import copy
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve()
BASE = HERE.parents[1]
REPO = HERE.parents[3]
sys.path.insert(0, str(BASE))
import run_benchmark as r


class Fixture(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.source, self.orchestrator = self.root / 'source', self.root / 'orchestrator'
        self.temp = self.root / 'runner-temp'
        self.work = self.temp / 'bench-work'
        for path in (self.source, self.orchestrator, self.temp):
            path.mkdir()
        self.output = self.root / 'github-output'
        self.output.touch()
        base = self.orchestrator / 'proposals/ci-acceleration'
        base.mkdir(parents=True)
        for name in ('source-contract.json', 'reviewed-plan.json', 'benchmark_adapter.py',
                     'dependencies/PIN.json', 'dependencies/downloads.lock.json'):
            path = base / name
            path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(BASE / name, path)
        contract = r.read_json(BASE / 'source-contract.json')
        for name in contract['original_scripts_sha256']:
            path = self.source / name
            path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(REPO / name, path)
        workflow = self.orchestrator / r.adapter.WORKFLOW
        workflow.parent.mkdir(parents=True, exist_ok=True)
        workflow.write_text('name: Offline benchmark workflow fixture\n')
        self.env = {
            'GITHUB_ACTIONS': 'true', 'GITHUB_REPOSITORY': r.adapter.REPOSITORY,
            'GITHUB_REPOSITORY_ID': '123', 'GITHUB_SHA': '1' * 40,
            'GITHUB_REF': r.adapter.BENCHMARK_REF,
            'GITHUB_WORKFLOW_REF': r.adapter.REPOSITORY + '/' + r.adapter.WORKFLOW + '@' + r.adapter.BENCHMARK_REF,
            'GITHUB_WORKFLOW_SHA': '1' * 40, 'GITHUB_EVENT_NAME': 'push',
            'GITHUB_RUN_ID': '456', 'GITHUB_RUN_ATTEMPT': '2',
            'RPG_DETECT_LEAKS': '1', 'PYTHONDONTWRITEBYTECODE': '1',
            'RUNNER_OS': 'Linux', 'RUNNER_ARCH': 'X64', 'ImageOS': 'ubuntu24',
            'ImageVersion': '20261010.1.0', 'RUNNER_TEMP': str(self.temp),
            'GITHUB_OUTPUT': str(self.output), 'PATH': '/usr/bin:/bin',
        }
        # The source and orchestrator are labeled fixtures, not claimed Git checkouts.
        def git(root, *args):
            if args == ('rev-parse', '--is-shallow-repository'):
                return 'false'
            if args == ('rev-parse', 'HEAD^{tree}'):
                return '2' * 40
            if args == ('rev-parse', 'HEAD'):
                return r.adapter.SOURCE['commit']
            raise AssertionError((root, args))
        self.patches = [patch.object(r, '__file__', str(self.orchestrator / r.RUNNER_REL)),
                        patch.object(r, 'require_runtime'), patch.object(r.adapter, 'validate_checkouts'),
                        patch.object(r.adapter, 'git', side_effect=git)]
        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)

    def prepare(self, phase='cold'):
        return r.prepare(self.source, self.orchestrator, self.work, phase, self.env)

    def installed_fixture(self, state):
        workspace = Path(state['paths']['dependency_work'])
        for folder in ('venv/bin', 'cli', 'data/packages/esp32/tools'):
            (workspace / folder).mkdir(parents=True, exist_ok=True)
        for name in ('venv/bin/python3', 'cli/arduino-cli', 'arduino-cli.yaml', 'install.log'):
            (workspace / name).write_text('offline fixture\n')
        Path(state['paths']['cache_root']).mkdir()
        pin = r.read_json(BASE / r.PIN_REL)
        report = {
            'schema': 1, 'status': 'verified_and_installed', 'phase': state['phase'],
            'context': r.LOCK_CONTEXT, 'lock_sha256': state['lock_sha256'],
            'cache_key': state['cache_key'], 'cache_hit': state['phase'] == 'warm',
            'benchmark_salt': r.SALT, 'cache_bytes': 123,
            'dependency_install_ns': 20, 'dependency_verify_ns': 10,
            'python': str(workspace / 'venv/bin/python3'), 'cli': str(workspace / 'cli/arduino-cli'),
            'config': str(workspace / 'arduino-cli.yaml'), 'tools_dir': str(workspace / 'data/packages/esp32/tools'),
            'tool_identity': {'python': '3.12.3', 'pillow': '11.3.0', 'pyyaml': '6.0.3',
                              'arduino_cli_version': '1.5.2-rc.1', 'arduino_cli_commit': pin['arduino_cli_commit'],
                              'esp32_core_version': '3.3.12'},
        }
        report['paths'] = {'python': report['python'], 'arduino_cli': report['cli'],
                           'arduino_config': report['config'], 'downloads': state['paths']['cache_root']}
        report['cache'] = {'key': state['cache_key'], 'hit': report['cache_hit'],
                           'restored': report['cache_hit'], 'lock_sha256': state['lock_sha256'], 'files': 27, 'bytes': 123}
        r.write_json(self.work / 'evidence/dependencies.json', report)
        return report


class PrepareTests(Fixture):
    def test_prepare_records_context_outputs_and_only_external_adapters(self):
        before = {p.relative_to(self.source): p.read_bytes() for p in self.source.rglob('*') if p.is_file()}
        environment = copy.deepcopy(self.env)
        state = self.prepare()
        self.assertEqual(state['status'], 'prepared')
        self.assertTrue(state['benchmark_only'])
        self.assertTrue(state['cannot_publish'])
        self.assertTrue(state['cache_key'].endswith('-run-456-attempt-2'))
        self.assertEqual(state['artifact_name'], 'benchmark-' + r.adapter.SOURCE['commit'] + '-' + '1' * 40 + '-456-2-cold')
        self.assertEqual(self.env, environment)
        self.assertEqual(before, {p.relative_to(self.source): p.read_bytes() for p in self.source.rglob('*') if p.is_file()})
        self.assertEqual(dict(line.split('=', 1) for line in self.output.read_text().splitlines()), r.output_values(state))
        self.assertEqual(r.read_json(self.work / 'context.json'), state['context'])
        self.assertFalse((self.source / 'build').exists())
        self.assertFalse(Path(state['paths']['dependency_work']).exists())

    def test_prepare_is_single_use(self):
        self.prepare()
        with self.assertRaisesRegex(r.Rejected, 'Fresh path'):
            self.prepare()

    def test_reject_existing_build_and_release_files(self):
        for name in ['build', *sorted(r.adapter.GENERATED)]:
            with self.subTest(name=name):
                path = self.source / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('old output')
                with self.assertRaisesRegex(r.Rejected, 'Fresh path'):
                    self.prepare()
                path.unlink()

    def test_reject_dangling_build_symlink(self):
        (self.source / 'build').symlink_to(self.source / 'missing')
        with self.assertRaisesRegex(r.Rejected, 'Fresh path'):
            self.prepare()

    def test_reject_all_preexisting_auxiliary_workspaces(self):
        for name in ('download-cache', 'dependency-work', 'esp32-coopdiag77'):
            path = self.temp / name
            path.mkdir()
            with self.assertRaisesRegex(r.Rejected, 'Fresh path'):
                self.prepare()
            path.rmdir()

    def test_reject_relative_overlapping_and_symlink_roots(self):
        for source, work in ((Path('source'), self.work), (self.source, self.source / 'nested')):
            with self.assertRaises((r.Rejected, r.adapter.Rejected)):
                r.prepare(source, self.orchestrator, work, 'cold', self.env)
        alias = self.root / 'alias'
        alias.symlink_to(self.source, target_is_directory=True)
        with self.assertRaisesRegex(r.Rejected, 'canonical'):
            r.prepare(alias, self.orchestrator, self.work, 'cold', self.env)

    def test_reject_shallow_checkout(self):
        with patch.object(r.adapter, 'git', side_effect=lambda root, *args: 'true' if '--is-shallow-repository' in args else '2' * 40):
            with self.assertRaisesRegex(r.Rejected, 'Full Git history'):
                self.prepare()

    def test_reject_changed_source_recipe_and_plan(self):
        path = self.source / r.ORIGINAL_WORKFLOW
        path.write_text(path.read_text() + '\n# changed\n')
        with self.assertRaisesRegex(r.Rejected, 'Original script changed'):
            self.prepare()
        shutil.copyfile(REPO / r.ORIGINAL_WORKFLOW, path)
        path = self.orchestrator / 'proposals/ci-acceleration/reviewed-plan.json'
        path.write_text(path.read_text() + '\n')
        with self.assertRaisesRegex(r.Rejected, 'plan digest'):
            self.prepare()

    def test_reject_wrong_branch_event_or_identity_environment(self):
        for key, value in (('GITHUB_REF', 'refs/heads/main'), ('GITHUB_EVENT_NAME', 'workflow_dispatch'),
                           ('GITHUB_SHA', r.adapter.SOURCE['commit']), ('GITHUB_RUN_ID', '01')):
            bad = {**self.env, key: value}
            with self.subTest(key=key), self.assertRaises(r.adapter.Rejected):
                r.prepare(self.source, self.orchestrator, self.work, 'cold', bad)

    def test_reject_changed_lock_and_pin(self):
        path = self.orchestrator / 'proposals/ci-acceleration' / r.LOCK_REL
        path.write_bytes(path.read_bytes() + b'\n')
        with self.assertRaises(r.cache_policy.PolicyError):
            self.prepare()

    def test_both_phases_share_key_but_not_artifact_name(self):
        cold = self.prepare()
        other = self.temp / 'warm-work'
        warm = r.prepare(self.source, self.orchestrator, other, 'warm', self.env)
        self.assertEqual(cold['cache_key'], warm['cache_key'])
        self.assertNotEqual(cold['artifact_name'], warm['artifact_name'])
        self.assertNotEqual(cold['context']['benchmark']['phase'], warm['context']['benchmark']['phase'])


class DependencyTests(Fixture):
    def test_report_requires_installed_and_exact_context_tool_versions(self):
        state = self.prepare()
        report = self.installed_fixture(state)
        self.assertEqual(r.verify_dependency_report(report, state, Path(state['paths']['dependency_work'])), report)
        for key, value in (('status', 'failed'), ('phase', 'warm'), ('cache_hit', 0), ('lock_sha256', '0' * 64),
                           ('cache_key', state['cache_key'] + '-other'), ('cache_bytes', True), ('dependency_verify_ns', 0)):
            with self.subTest(key=key), self.assertRaises(r.Rejected):
                r.verify_dependency_report({**report, key: value}, state, Path(state['paths']['dependency_work']))
        for key in report['tool_identity']:
            bad = copy.deepcopy(report)
            bad['tool_identity'][key] = 'changed'
            with self.subTest(tool=key), self.assertRaises(r.Rejected):
                r.verify_dependency_report(bad, state, Path(state['paths']['dependency_work']))

    def test_accepts_only_final_venv_python_symlink(self):
        state = self.prepare()
        report = self.installed_fixture(state)
        python = Path(report['python'])
        python.unlink()
        python.symlink_to(sys.executable)
        r.verify_dependency_report(report, state, Path(state['paths']['dependency_work']))
        cli = Path(report['cli'])
        cli.unlink()
        cli.symlink_to(sys.executable)
        with self.assertRaises(r.Rejected):
            r.verify_dependency_report(report, state, Path(state['paths']['dependency_work']))

    def test_path_configuration_never_mutates_github_context(self):
        state = self.prepare()
        report = self.installed_fixture(state)
        old = copy.deepcopy(self.env)
        new = r.dependency_environment(self.env, report)
        self.assertEqual(self.env, old)
        self.assertEqual(new['ARDUINO_CONFIG_FILE'], report['config'])
        self.assertTrue(new['PATH'].startswith(str(Path(report['python']).parent) + os.pathsep))
        self.assertEqual({k: v for k, v in new.items() if k.startswith('GITHUB_')},
                         {k: v for k, v in old.items() if k.startswith('GITHUB_')})

    def test_cold_refuses_restore_and_warm_requires_exact_hit(self):
        state = self.prepare()
        self.installed_fixture(state)
        for phase, hit, key, valid in [('cold', 'false', '', True), ('cold', 'true', state['cache_key'], False),
                                     ('cold', 'false', state['cache_key'], False), ('warm', 'true', state['cache_key'], True),
                                     ('warm', 'false', '', False), ('warm', 'true', state['cache_key'] + '-prefix', False)]:
            candidate = {**state, 'phase': phase}
            environment = {**self.env, 'BENCHMARK_CACHE_HIT': hit, 'BENCHMARK_CACHE_KEY': key}
            with self.subTest(phase=phase, hit=hit, key=key):
                if valid:
                    r.check_fresh_qualification(candidate, environment)
                else:
                    with self.assertRaises(r.Rejected):
                        r.check_fresh_qualification(candidate, environment)

    def test_qualification_refuses_resume_even_after_failure(self):
        state = self.prepare()
        self.installed_fixture(state)
        for status in ('running', 'failed', 'passed'):
            with self.subTest(status=status), self.assertRaisesRegex(r.Rejected, 'single-use'):
                r.check_fresh_qualification({**state, 'status': status}, self.env)


class RecipeTests(unittest.TestCase):
    def test_exact_original_recipe_and_original_upload_inventory(self):
        workflow = (REPO / r.ORIGINAL_WORKFLOW).read_text()
        r.validate_recipe(workflow)
        patterns = r.upload_patterns(workflow)
        self.assertIn('build/coopdiag77-source-identity.json', patterns)
        self.assertIn('build/coop-radio-render/**/*.gif', patterns)
        self.assertIn('build/package12-validation/**/*.log', patterns)
        self.assertIn('release/firmware.bin', patterns)

    def test_exact_on_off_flags_and_external_build_paths(self):
        root = Path('/runner-temp/esp32-coopdiag77')
        common = ['arduino-cli', 'compile', '--verbose', '--jobs', '4', '--fqbn', r.comparison.FQBN]
        for flag in (1, 0):
            expected = common + ([] if flag else ['--build-property', 'compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=0'])
            expected += ['--build-path', str(root / ('flag-' + str(flag))), '--export-binaries=false', 'firmware/RPG_POKET_2']
            self.assertEqual(r.compile_command(flag, root), expected)
        self.assertNotIn('RPG_ENABLE_COOP_TRAINING', ' '.join(r.compile_command(1, root)))
        with self.assertRaises(r.Rejected):
            r.compile_command(True, root)

    def test_literal_recipe_changes_fail_closed(self):
        original = (REPO / r.ORIGINAL_WORKFLOW).read_text()
        for before, after in [('--jobs 4', '--jobs 3'), ('for flag in 1 0;', 'for flag in 0 1;'),
                              ('--export-binaries=false', '--export-binaries=true'),
                              ('release --sanitizers', 'release'), (r.comparison.FQBN, 'esp32:esp32:other')]:
            with self.subTest(before=before), self.assertRaises(r.Rejected):
                r.validate_recipe(original.replace(before, after))

    def test_final_bytes_recipe_retains_all_five_original_commands(self):
        self.assertEqual(r.FINAL_COMMANDS, (
            ('python3', 'tools/release_coopdiag77_manifest.py'),
            ('python3', 'tools/verify_coopdiag77_release.py'),
            ('python3', 'tests/coopdiag77_release_verifier.py'),
            ('python3', 'tools/test_coopdiag77_release.py', 'release'),
            ('python3', 'tools/test_coopdiag77_release.py', 'release', '--sanitizers')))

    def test_runtime_rejects_optimized_python_before_work(self):
        result = subprocess.run([sys.executable, '-O', str(BASE / 'run_benchmark.py'), 'prepare',
                                 '--source', '/missing/source', '--orchestrator', '/missing/orchestrator',
                                 '--work', '/missing/work', '--phase', 'cold'],
                                env={**os.environ, 'PYTHONDONTWRITEBYTECODE': '1'}, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('optimized runtime is forbidden', result.stderr)

    def test_runtime_platform_and_inherited_assertion_flags(self):
        environment = {'RUNNER_OS': 'Linux', 'RUNNER_ARCH': 'X64', 'ImageOS': 'ubuntu24', 'ImageVersion': '20261010.1.0'}
        with patch.object(r.sys, 'flags', SimpleNamespace(optimize=0)), patch.object(r.platform, 'python_version', return_value='3.12.3'), patch.object(r.subprocess, 'check_output', return_value=r.comparison.HOST_COMPILER + '\n'):
            r.require_runtime(environment)
            for key, value in (('RUNNER_OS', 'Windows'), ('RUNNER_ARCH', 'ARM64'), ('ImageOS', 'ubuntu22'),
                               ('ImageVersion', 'unknown'), ('PYTHONOPTIMIZE', '1')):
                with self.subTest(key=key), self.assertRaises(r.Rejected):
                    r.require_runtime({**environment, key: value})
        with patch.object(r.sys, 'flags', SimpleNamespace(optimize=0)), patch.object(r.platform, 'python_version', return_value='3.12.4'):
            with self.assertRaisesRegex(r.Rejected, '3.12.3'):
                r.require_runtime(environment)


    def test_runtime_checks_host_compiler_before_expensive_work(self):
        environment = {'RUNNER_OS': 'Linux', 'RUNNER_ARCH': 'X64', 'ImageOS': 'ubuntu24', 'ImageVersion': '20261010.1.0'}
        for output in ('', 'g++ changed version\n'):
            with self.subTest(output=output), patch.object(r.sys, 'flags', SimpleNamespace(optimize=0)), patch.object(r.platform, 'python_version', return_value='3.12.3'), patch.object(r.subprocess, 'check_output', return_value=output) as command:
                with self.assertRaisesRegex(r.Rejected, 'Host compiler changed'):
                    r.require_runtime(environment)
                command.assert_called_once_with(['g++', '--version'], text=True, env=environment)


class RecorderTests(unittest.TestCase):
    def test_subprocess_logs_exit_codes_and_monotonic_measurement(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / 'logs').mkdir()
            recorder = r.Recorder(root, root, {'GITHUB_SHA': 'true-context'})
            def fake(argv, **kwargs):
                self.assertFalse(kwargs['check'])
                self.assertEqual(kwargs['env']['GITHUB_SHA'], 'true-context')
                kwargs['stdout'].write(b'offline stdout and stderr fixture\n')
                return SimpleNamespace(returncode=0)
            with patch.object(r.subprocess, 'run', side_effect=fake):
                recorder.phase('aggregate', lambda: recorder.run('one', ['not-executed', '--arg'], product_log=root / 'product.log'))
            self.assertEqual((root / 'logs/one.log').read_bytes(), (root / 'product.log').read_bytes())
            self.assertGreater(recorder.phases['aggregate'], 0)
            self.assertEqual(recorder.commands[0]['exit_code'], 0)
            self.assertGreater(recorder.commands[0]['elapsed_ns'], 0)

    def test_command_failures_and_launch_errors_are_recorded(self):
        for result in (SimpleNamespace(returncode=23), OSError('offline missing executable')):
            with tempfile.TemporaryDirectory() as td:
                root = Path(td)
                (root / 'logs').mkdir()
                recorder = r.Recorder(root, root, {})
                kwargs = {'side_effect': result} if isinstance(result, Exception) else {'return_value': result}
                with patch.object(r.subprocess, 'run', **kwargs):
                    with self.assertRaises((r.CommandFailed, OSError)):
                        recorder.phase('identity', lambda: recorder.run('failure', ['not-executed']))
                self.assertEqual(len(r.read_json(root / 'commands.json')), 1)
                self.assertGreater(recorder.phases['identity'], 0)
                self.assertEqual(recorder.commands[0]['exit_code'], None if isinstance(result, Exception) else 23)


class QualificationTests(Fixture):
    def run_fixture(self, fail_label=None):
        state = self.prepare()
        report = self.installed_fixture(state)
        lock = r.load_lock(self.orchestrator, Path(state['paths']['cache_root']))
        report['cache_bytes'] = sum(x.bytes for x in lock.artifacts)
        report['cache']['bytes'] = report['cache_bytes']
        r.write_json(self.work / 'evidence/dependencies.json', report)
        environment = {**self.env, 'BENCHMARK_CACHE_HIT': 'false', 'BENCHMARK_CACHE_KEY': ''}
        calls = []
        def subprocess_fixture(argv, **kwargs):
            calls.append(argv)
            self.assertEqual(kwargs['env']['GITHUB_SHA'], self.env['GITHUB_SHA'])
            if argv == ['arduino-cli', 'version']:
                kwargs['stdout'].write(b'arduino-cli Version: 1.5.2-rc.1 Commit: fixture\n')
            elif argv == ['arduino-cli', 'core', 'list']:
                kwargs['stdout'].write(b'esp32:esp32 3.3.12 3.3.12 ESP32\n')
            elif argv[:2] == ['arduino-cli', 'compile']:
                output = Path(argv[argv.index('--build-path') + 1])
                output.mkdir(parents=True)
                (output / 'RPG_POKET_2.ino.bin').write_bytes(b'offline non-firmware fixture')
            elif str(self.work / 'adapter/record_identity.py') in argv:
                r.write_json(self.source / 'build/coopdiag77-benchmark-identity.json', {'offline_fixture': True})
            if fail_label and any(fail_label in arg for arg in argv):
                return SimpleNamespace(returncode=17)
            return SimpleNamespace(returncode=0)
        with patch.object(r.subprocess, 'run', side_effect=subprocess_fixture), \
             patch.object(r.cache_policy, '_verify_tree'), \
             patch.object(r, 'verify_aggregate', return_value={'offline_fixture': True}), \
             patch.object(r, 'make_partial', return_value={'offline_fixture': True}) as partial, \
             patch.object(r, 'stage_evidence'):
            if fail_label:
                with self.assertRaises(r.CommandFailed):
                    r.qualify(self.source, self.orchestrator, self.work, 'cold', environment)
                self.assertFalse(partial.called)
            else:
                r.qualify(self.source, self.orchestrator, self.work, 'cold', environment)
        return calls

    def test_full_orchestration_uses_existing_dependency_step_and_exact_recipe(self):
        calls = self.run_fixture()
        aggregate = ['bash', str(self.work / 'adapter/test_coopdiag77.sh')]
        self.assertIn(aggregate, calls)
        self.assertFalse(any('install_dependencies.py' in ' '.join(call) for call in calls))
        compile_calls = [call for call in calls if call[:2] == ['arduino-cli', 'compile']]
        self.assertEqual(compile_calls, [r.compile_command(1, self.temp / 'esp32-coopdiag77'), r.compile_command(0, self.temp / 'esp32-coopdiag77')])
        final_calls = [call for call in calls if tuple(call) in r.FINAL_COMMANDS]
        self.assertEqual(final_calls, [list(command) for command in r.FINAL_COMMANDS])
        self.assertEqual(calls[-1], ['python3', str(self.work / 'adapter/record_identity.py'),
                         '--build-root', str(self.temp / 'esp32-coopdiag77'),
                         '--tools-dir', str(self.temp / 'dependency-work/data/packages/esp32/tools')])
        self.assertEqual(sum(call == ['python3', str(self.work / 'adapter/check_scope.py')] for call in calls), 3)
        self.assertEqual(r.read_json(self.work / 'state.json')['status'], 'passed')
        self.assertTrue((self.work / 'evidence/measurement.partial.json').exists())
        self.assertEqual(set(r.read_json(self.work / 'evidence/phase-times.json')), set(r.PARTIAL_PHASES))

    def test_failure_stops_before_downstream_gates_and_never_writes_success_partial(self):
        calls = self.run_fixture('test_coopdiag77.sh')
        self.assertFalse(any(call[:2] == ['arduino-cli', 'compile'] for call in calls))
        self.assertEqual(r.read_json(self.work / 'state.json')['status'], 'failed')
        self.assertEqual(r.read_json(self.work / 'evidence/runner-status.json')['status'], 'failed')
        self.assertFalse((self.work / 'evidence/measurement.partial.json').exists())

    def test_missing_dependency_report_stops_without_any_product_command(self):
        state = self.prepare()
        self.installed_fixture(state)
        (self.work / 'evidence/dependencies.json').unlink()
        env = {**self.env, 'BENCHMARK_CACHE_HIT': 'false', 'BENCHMARK_CACHE_KEY': ''}
        with patch.object(r.subprocess, 'run') as run:
            with self.assertRaisesRegex(r.Rejected, 'Missing/nonregular file'):
                r.qualify(self.source, self.orchestrator, self.work, 'cold', env)
            self.assertFalse(run.called)
        self.assertEqual(r.read_json(self.work / 'state.json')['status'], 'failed')
        self.assertFalse((self.work / 'evidence/measurement.partial.json').exists())


class AggregateTests(Fixture):
    def aggregate_fixture(self):
        state = self.prepare()
        plan = r.read_json(BASE / 'reviewed-plan.json')
        root = self.source / 'build/package12-validation' / state['context']['benchmark']['aggregate_run_id']
        root.mkdir(parents=True)
        manifest = {'files': {}, 'compiler': r.comparison.HOST_COMPILER, 'detect_leaks': '1',
                    'assertions': True, 'ubsan_halt': True}
        manifest['sha256'] = r.hashlib.sha256(json.dumps(manifest, sort_keys=True).encode()).hexdigest()
        summary = {'all_passed': True, 'failed': [], 'tasks': 157, 'lsan': 'detect_leaks=1',
                   'source_sha256': manifest['sha256']}
        commands = {'preparation': plan['preparation'], **dict(plan['tasks'])}
        checkpoints = {}
        for name, argv in commands.items():
            (root / (name + '.log')).write_text('Offline fixture log for ' + name)
            checkpoints[name] = {'status': 'passed', 'commands': argv, 'log_sha256': r.digest(root / (name + '.log'))}
        r.write_json(root / 'summary.json', summary)
        r.write_json(root / 'source-manifest.json', manifest)
        r.write_json(root / 'checkpoints.json', checkpoints)
        r.write_json(root / 'plan.json', {'focused': False, 'tasks': plan['tasks'], 'source_sha256': manifest['sha256']})
        (self.source / 'build/coopdiag77-tests.log').write_text(''.join('RUN ' + name + '\n' for name in commands))
        return state, plan, root, summary

    def test_all_157_fresh_groups_plan_and_log_digests_required(self):
        state, plan, root, summary = self.aggregate_fixture()
        with patch.object(r.comparison, 'SOURCE_INVENTORY_SHA256', summary['source_sha256']):
            self.assertEqual(r.verify_aggregate(self.source, state, plan), summary)
            log = root / 'preparation.log'
            log.write_text('Changed fixture log')
            with self.assertRaisesRegex(r.Rejected, 'Checkpoint mismatch'):
                r.verify_aggregate(self.source, state, plan)

    def test_resume_skips_and_missing_run_are_rejected(self):
        state, plan, root, summary = self.aggregate_fixture()
        log = self.source / 'build/coopdiag77-tests.log'
        original = log.read_text()
        with patch.object(r.comparison, 'SOURCE_INVENTORY_SHA256', summary['source_sha256']):
            for text in (original + 'RESUME skip preparation\n', original.replace('RUN preparation\n', ''),
                         original + 'RUN preparation\n'):
                log.write_text(text)
                with self.assertRaises(r.Rejected):
                    r.verify_aggregate(self.source, state, plan)

    def test_groups_sanitizers_and_reviewed_source_inventory_fail_closed(self):
        state, plan, root, summary = self.aggregate_fixture()
        with self.assertRaisesRegex(r.Rejected, 'Reviewed source inventory'):
            r.verify_aggregate(self.source, state, plan)
        with patch.object(r.comparison, 'SOURCE_INVENTORY_SHA256', summary['source_sha256']):
            for key, value in (('tasks', 156), ('tasks', True), ('lsan', 'detect_leaks=0'), ('all_passed', 1), ('failed', ['one'])):
                r.write_json(root / 'summary.json', {**summary, key: value})
                with self.subTest(key=key, value=value), self.assertRaises(r.Rejected):
                    r.verify_aggregate(self.source, state, plan)


class ArtifactTests(Fixture):
    def stage_fixture(self):
        state = self.prepare()
        for name in ('release/firmware.bin', 'release/artes.pak', 'release/manifest.json', 'release/SHA256SUMS',
                     'build/coopdiag77-benchmark-identity.json', 'build/coopdiag77-tests.log',
                     'build/coop-radio-render/normal/example.ppm', 'build/package12-validation/example/summary.json'):
            path = self.source / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('Offline artifact fixture: ' + name)
        return state

    def test_stages_original_patterns_and_benchmark_identity_without_production_name(self):
        self.stage_fixture()
        workflow = (self.source / r.ORIGINAL_WORKFLOW).read_text()
        inventory = r.stage_evidence(self.source, self.work, workflow)
        self.assertIn('release/firmware.bin', inventory)
        self.assertIn('build/coop-radio-render/normal/example.ppm', inventory)
        self.assertIn('build/coopdiag77-benchmark-identity.json', inventory)
        self.assertNotIn('build/coopdiag77-source-identity.json', inventory)
        for name, entry in inventory.items():
            destination = self.work / 'evidence/source' / name
            self.assertEqual(entry, {'bytes': destination.stat().st_size, 'sha256': r.digest(destination)})
        self.assertTrue((self.work / 'evidence/adapter/adapter-manifest.json').is_file())
        self.assertTrue((self.work / 'evidence/adapter/benchmark_runtime.py').is_file())

    def test_rejects_production_identity_and_symlink_artifact(self):
        self.stage_fixture()
        workflow = (self.source / r.ORIGINAL_WORKFLOW).read_text()
        path = self.source / 'build/coopdiag77-source-identity.json'
        path.write_text('not a production qualification')
        with self.assertRaisesRegex(r.Rejected, 'Fresh path'):
            r.stage_evidence(self.source, self.work, workflow)
        path.unlink()
        path = self.source / 'release/firmware.bin'
        path.unlink()
        path.symlink_to(self.source / 'release/artes.pak')
        with self.assertRaisesRegex(r.Rejected, 'Symlink'):
            r.stage_evidence(self.source, self.work, workflow)

    def test_failure_staging_retains_partial_logs_without_claiming_completion(self):
        self.prepare()
        (self.source / 'build').mkdir()
        (self.source / 'build/coopdiag77-tests.log').write_text('Offline failed gate fixture')
        workflow = (self.source / r.ORIGINAL_WORKFLOW).read_text()
        with self.assertRaisesRegex(r.Rejected, 'Missing required final evidence'):
            r.stage_evidence(self.source, self.work, workflow)
        inventory = r.stage_evidence(self.source, self.work, workflow, complete=False)
        self.assertEqual(set(inventory), {'build/coopdiag77-tests.log'})
        self.assertFalse((self.work / 'evidence/measurement.partial.json').exists())

    def test_partial_schema_requires_final_identity_and_actual_file_digests(self):
        state = self.stage_fixture()
        dependencies = {'cache_hit': False, 'cache_bytes': 123}
        summary = {'source_sha256': r.comparison.SOURCE_INVENTORY_SHA256}
        artifacts = {name: {'bytes': (self.source / 'release' / name).stat().st_size,
                            'sha256': r.digest(self.source / 'release' / name)}
                     for name in ('firmware.bin', 'artes.pak', 'manifest.json')}
        identity = {'benchmark_only': True, 'cannot_publish': True, 'published': False,
                    'benchmark_context': state['context'], 'source_commit': r.adapter.SOURCE['commit'],
                    'source_tree': r.adapter.SOURCE['tree'], 'aggregate': summary,
                    'adapter_manifest_sha256': r.digest(self.work / 'adapter/adapter-manifest.json'),
                    'target_builds': {'compiler': 'esp-x32/2601', 'core': '3.3.12', 'fqbn': r.comparison.FQBN},
                    'artifacts': artifacts}
        phases = {name: 10 for name in r.PARTIAL_PHASES}
        partial = r.make_partial(state, self.env, dependencies, summary, identity, phases)
        self.assertEqual(partial['kind'], 'benchmark-partial')
        self.assertEqual(set(partial['identity']), set(r.comparison.IDENTITY))
        self.assertEqual(set(partial['gates']), set(r.comparison.GATES))
        self.assertTrue(partial['cannot_publish'])
        self.assertEqual(partial['release_files'], artifacts)
        self.assertEqual(partial['coverage']['groups'], 157)
        self.assertEqual(partial['coverage']['resumed'], 0)
        for name in ('benchmark_only', 'cannot_publish', 'published'):
            bad = {**identity, name: not identity[name]}
            with self.subTest(name=name), self.assertRaises(r.Rejected):
                r.make_partial(state, self.env, dependencies, summary, bad, phases)
        with self.assertRaisesRegex(r.Rejected, 'Missing phase timing'):
            r.make_partial(state, self.env, dependencies, summary, identity, {**phases, 'identity': 0})
        (self.source / 'release/firmware.bin').write_text('changed after identity')
        with self.assertRaisesRegex(r.Rejected, 'release bytes changed'):
            r.make_partial(state, self.env, dependencies, summary, identity, phases)


if __name__ == '__main__':
    unittest.main()
