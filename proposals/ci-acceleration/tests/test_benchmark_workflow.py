#!/usr/bin/env python3
"""Offline workflow contract tests, not a qualification or hosted CI run."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

import yaml

HERE = Path(__file__).resolve().parents[1]
WORKFLOW = HERE / 'ci-acceleration-benchmark.yml'
SOURCE = '6026fb49db4fe874b9c3f0571cbe4fdb9e66adb3'
REPOSITORY = 'Oflodorporto/RPG-POKET-2'
BRANCH = 'bench/coopdiag77-cache-promotion'
CHECKOUT = 'actions/checkout@11d5960a326750d5838078e36cf38b85af677262'
SAVE = 'actions/cache/save@5a3ec84eff668545956fd18022155c47e93e2684'
RESTORE = 'actions/cache/restore@5a3ec84eff668545956fd18022155c47e93e2684'
UPLOAD = 'actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02'


class StrictLoader(yaml.BaseLoader):
    """Keep Actions' 'on' key a string; reject duplicate mapping keys."""
    def construct_mapping(self, node, deep=False):
        result = {}
        for key_node, value_node in node.value:
            key = self.construct_object(key_node, deep=deep)
            if key in result:
                raise ValueError('Duplicate YAML key: ' + str(key))
            result[key] = self.construct_object(value_node, deep=deep)
        return result


def load_workflow():
    return yaml.load(WORKFLOW.read_text(), Loader=StrictLoader)


def named(job, name):
    result = [step for step in job['steps'] if step.get('name') == name]
    if len(result) != 1:
        raise ValueError('Expected one step named ' + name)
    return result[0]


def env_entries(node):
    if isinstance(node, dict):
        yield from node.get('env', {}).items()
        for value in node.values():
            yield from env_entries(value)
    elif isinstance(node, list):
        for value in node:
            yield from env_entries(value)


class WorkflowTests(unittest.TestCase):
    def setUp(self):
        self.workflow = load_workflow()
        self.jobs = self.workflow['jobs']

    def test_exact_push_branch_is_only_entry_point(self):
        self.assertEqual(self.workflow['on'], {'push': {'branches': [BRANCH],
            'paths': ['.github/workflows/ci-acceleration-benchmark.yml', 'proposals/ci-acceleration/**']}})
        self.assertEqual(set(self.jobs), {'cold', 'warm', 'compare'})
        for job in self.jobs.values():
            expression = job['if']
            self.assertIn("github.repository == '" + REPOSITORY + "'", expression)
            self.assertIn("github.event_name == 'push'", expression)
            self.assertIn("github.ref == 'refs/heads/" + BRANCH + "'", expression)
            self.assertNotIn('||', expression)
            self.assertTrue(expression.endswith(" && !contains(github.event.head_commit.message, '[exact-rehearsal-only]')"))

    def test_least_privilege_and_no_permission_override(self):
        self.assertEqual(self.workflow['permissions'], {'contents': 'read', 'actions': 'read'})
        for job in self.jobs.values():
            self.assertNotIn('permissions', job)
            self.assertNotIn('environment', job)
        self.assertNotIn('secrets.', WORKFLOW.read_text())

    def test_ephemeral_sequential_runners_and_no_matrix(self):
        self.assertNotIn('needs', self.jobs['cold'])
        self.assertEqual(self.jobs['warm']['needs'], 'cold')
        self.assertEqual(self.jobs['compare']['needs'], ['cold', 'warm'])
        self.assertTrue(self.jobs['compare']['if'].startswith('always() && '))
        for job in self.jobs.values():
            self.assertEqual(job['runs-on'], 'ubuntu-24.04')
            self.assertNotIn('strategy', job)
            self.assertNotIn('container', job)
        self.assertEqual(self.workflow['concurrency']['cancel-in-progress'], 'false')

    def test_every_action_is_an_exact_reviewed_sha(self):
        allowed = {CHECKOUT, SAVE, RESTORE, UPLOAD}
        for job in self.jobs.values():
            for step in job['steps']:
                if 'uses' in step:
                    self.assertIn(step['uses'], allowed)
                    self.assertRegex(step['uses'], r'@[a-f0-9]{40}$')

    def test_checkouts_are_siblings_with_full_history_and_no_credentials(self):
        for phase in ('cold', 'warm'):
            checks = [s for s in self.jobs[phase]['steps'] if s.get('uses') == CHECKOUT]
            self.assertEqual(len(checks), 2)
            self.assertEqual(checks[0]['with']['path'], 'orchestrator')
            self.assertEqual(checks[0]['with']['ref'], '${{ github.sha }}')
            self.assertEqual(checks[1]['with']['path'], 'source')
            self.assertEqual(checks[1]['with']['ref'], SOURCE)
        for job in self.jobs.values():
            for step in job['steps']:
                if step.get('uses') == CHECKOUT:
                    self.assertEqual(step['with']['fetch-depth'], '0')
                    self.assertEqual(step['with']['persist-credentials'], 'false')
                    self.assertNotIn('token', step['with'])

    def test_no_real_actions_identity_is_overridden(self):
        for key, value in env_entries(self.workflow):
            self.assertFalse(key.startswith('GITHUB_'), (key, value))
            self.assertNotIn(key, ('ACTIONS_RUNTIME_TOKEN', 'ACTIONS_CACHE_URL'))
        for job in self.jobs.values():
            for step in job['steps']:
                script = step.get('run', '')
                self.assertNotRegex(script, r'(?m)^\s*(?:export\s+)?GITHUB_[A-Z_]+\s*=')
        self.assertEqual(self.workflow['env']['RPG_DETECT_LEAKS'], '1')
        self.assertEqual(self.workflow['env']['PYTHONDONTWRITEBYTECODE'], '1')

    def test_prepare_happens_before_install_or_any_cache_use(self):
        for phase in ('cold', 'warm'):
            steps = self.jobs[phase]['steps']
            prepare = named(self.jobs[phase], 'Prepare isolated benchmark')
            script = prepare['run']
            self.assertIn('run_benchmark.py" prepare', script)
            self.assertIn('--source "$GITHUB_WORKSPACE/source"', script)
            self.assertIn('--orchestrator "$GITHUB_WORKSPACE/orchestrator"', script)
            self.assertIn('--work "$RUNNER_TEMP/bench-work" --phase ' + phase, script)
            for step in steps:
                if step.get('uses') in (SAVE, RESTORE) or step.get('name') == 'Install verified dependencies':
                    self.assertLess(steps.index(prepare), steps.index(step))

    def test_cold_never_restores_and_warm_never_saves(self):
        cold_uses = [s.get('uses') for s in self.jobs['cold']['steps']]
        warm_uses = [s.get('uses') for s in self.jobs['warm']['steps']]
        self.assertNotIn(RESTORE, cold_uses)
        self.assertEqual(cold_uses.count(SAVE), 1)
        self.assertNotIn(SAVE, warm_uses)
        self.assertEqual(warm_uses.count(RESTORE), 1)
        self.assertNotIn('actions/cache@', WORKFLOW.read_text())

    def test_only_download_cache_is_shared_with_no_prefix_restore(self):
        for job in self.jobs.values():
            for step in job['steps']:
                if step.get('uses') in (SAVE, RESTORE):
                    self.assertEqual(step['with']['path'], '${{ runner.temp }}/download-cache')
                    self.assertEqual(step['with']['key'], '${{ steps.prepare.outputs.cache_key }}')
                    self.assertEqual(step['with']['enableCrossOsArchive'], 'false')
                    self.assertNotIn('restore-keys', step['with'])
                    self.assertNotIn('lookup-only', step['with'])
        restore = named(self.jobs['warm'], 'Cache restore')
        self.assertEqual(restore['with']['fail-on-cache-miss'], 'true')

    def test_cache_save_requires_success_and_reverification(self):
        steps = self.jobs['cold']['steps']
        verify = named(self.jobs['cold'], 'Verify distributions before save')
        save = named(self.jobs['cold'], 'Cache save')
        self.assertEqual(steps.index(save), steps.index(verify) + 1)
        self.assertNotIn('if', save)
        self.assertNotIn('continue-on-error', save)
        self.assertIn('install_dependencies.py" verify', verify['run'])
        self.assertIn('--cache-root "$RUNNER_TEMP/download-cache"', verify['run'])
        self.assertIn('--expected-lock-sha256 "$LOCK_SHA256"', verify['run'])
        self.assertEqual(verify['env']['LOCK_SHA256'], '${{ steps.prepare.outputs.lock_sha256 }}')

    def test_dependency_install_and_qualification_are_complete_both_phases(self):
        for phase in ('cold', 'warm'):
            install = named(self.jobs[phase], 'Install verified dependencies')
            self.assertIn('--phase ' + phase, install['run'])
            self.assertIn('--workspace "$RUNNER_TEMP/dependency-work"', install['run'])
            self.assertIn('--report "$RUNNER_TEMP/bench-work/evidence/dependencies.json"', install['run'])
            self.assertIn('--run-id "$GITHUB_RUN_ID" --run-attempt "$GITHUB_RUN_ATTEMPT"', install['run'])
            self.assertIn('--cache-key "$CACHE_KEY"', install['run'])
            self.assertEqual(install['env']['CACHE_KEY'], '${{ steps.prepare.outputs.cache_key }}')
            qualify = named(self.jobs[phase], 'Run complete benchmark qualification')
            self.assertIn('run_benchmark.py" qualify', qualify['run'])
            self.assertIn('--phase ' + phase, qualify['run'])
            self.assertNotIn('if', qualify)
            self.assertNotIn('continue-on-error', qualify)
            self.assertNotIn('--resume', qualify['run'])
            self.assertNotIn('--skip', qualify['run'])
        self.assertEqual(named(self.jobs['cold'], 'Run complete benchmark qualification')['env']['BENCHMARK_CACHE_HIT'], 'false')
        self.assertEqual(named(self.jobs['cold'], 'Run complete benchmark qualification')['env']['BENCHMARK_CACHE_KEY'], '')
        self.assertEqual(named(self.jobs['warm'], 'Run complete benchmark qualification')['env']['BENCHMARK_CACHE_HIT'], '${{ steps.restore.outputs.cache-hit }}')
        self.assertEqual(named(self.jobs['warm'], 'Run complete benchmark qualification')['env']['BENCHMARK_CACHE_KEY'], '${{ steps.restore.outputs.cache-matched-key }}')

    def test_warm_checks_origin_and_exact_hit_before_install(self):
        steps = self.jobs['warm']['steps']
        origin = named(self.jobs['warm'], 'Require cold cache identity')
        hit = named(self.jobs['warm'], 'Require exact warm cache hit')
        self.assertEqual(origin['env']['COLD_KEY'], '${{ needs.cold.outputs.cache_key }}')
        self.assertLess(steps.index(origin), steps.index(named(self.jobs['warm'], 'Cache restore')))
        self.assertLess(steps.index(hit), steps.index(named(self.jobs['warm'], 'Install verified dependencies')))
        for cache_hit, restored, expected, success in (
                ('true', 'exact', 'exact', True), ('false', 'exact', 'exact', False),
                ('', '', 'exact', False), ('true', 'prefix', 'exact', False)):
            result = subprocess.run(['bash', '-euo', 'pipefail', '-c', hit['run']],
                                    env={**os.environ, 'CACHE_HIT': cache_hit,
                                         'RESTORED_KEY': restored, 'EXPECTED_KEY': expected},
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode == 0, success)

    def test_failed_qualification_keeps_evidence_with_unique_names(self):
        for phase in ('cold', 'warm'):
            step = named(self.jobs[phase], 'Upload benchmark evidence')
            self.assertEqual(step['if'], 'always()')
            self.assertEqual(step['uses'], UPLOAD)
            self.assertEqual(step['with']['path'], '${{ runner.temp }}/bench-work/evidence')
            self.assertIn('steps.prepare.outputs.artifact_name', step['with']['name'])
            self.assertIn('-' + phase, step['with']['name'])
            self.assertEqual(step['with']['if-no-files-found'], 'error')
            self.assertEqual(step['with']['overwrite'], 'false')
            self.assertEqual(step['with']['compression-level'], '0')
            self.assertEqual(step['with']['include-hidden-files'], 'false')
            self.assertEqual(self.jobs[phase]['outputs']['artifact_id'], '${{ steps.evidence.outputs.artifact-id }}')

    def test_jobs_are_fetched_for_exact_attempt_with_pagination(self):
        fetch = named(self.jobs['compare'], 'Fetch completed jobs and immutable artifacts')
        self.assertIn('gh api --paginate --slurp', fetch['run'])
        self.assertIn('actions/runs/$GITHUB_RUN_ID/attempts/$GITHUB_RUN_ATTEMPT/jobs?per_page=100', fetch['run'])
        self.assertEqual(fetch['env']['GH_TOKEN'], '${{ github.token }}')
        tokens = [(k, v) for k, v in env_entries(self.workflow) if 'TOKEN' in k]
        self.assertEqual(tokens, [('GH_TOKEN', '${{ github.token }}')])

    def test_artifact_bytes_and_metadata_are_downloaded_by_output_id(self):
        fetch = named(self.jobs['compare'], 'Fetch completed jobs and immutable artifacts')
        self.assertEqual(fetch['env']['COLD_ARTIFACT_ID'], '${{ needs.cold.outputs.artifact_id }}')
        self.assertEqual(fetch['env']['WARM_ARTIFACT_ID'], '${{ needs.warm.outputs.artifact_id }}')
        self.assertIn('[[ ! "$id" =~ ^[1-9][0-9]*$ ]]', fetch['run'])
        self.assertIn('actions/artifacts/$id"', fetch['run'])
        self.assertIn('actions/artifacts/$id/zip"', fetch['run'])
        self.assertNotIn('gh run download', fetch['run'])
        self.assertNotIn('unzip', fetch['run'])
        self.assertNotIn('download-artifact@', WORKFLOW.read_text())

    def test_fetch_shell_keeps_raw_data_and_diagnoses_missing_upload(self):
        # Only the gh executable is a fixture; this runs the actual YAML shell.
        script = named(self.jobs['compare'], 'Fetch completed jobs and immutable artifacts')['run']
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            gh = root / 'gh'
            gh.write_text('#!/bin/bash\nprintf "%s\\n" "$*" >> "$CALLS"\nprintf \'{"fixture":true}\\n\'\n')
            gh.chmod(0o700)
            environment = {**os.environ, 'PATH': str(root) + os.pathsep + os.environ['PATH'],
                           'RUNNER_TEMP': str(root), 'GITHUB_REPOSITORY': 'fixture/repo',
                           'GITHUB_RUN_ID': '123', 'GITHUB_RUN_ATTEMPT': '2',
                           'COLD_ARTIFACT_ID': '456', 'WARM_ARTIFACT_ID': 'invalid/789',
                           'GH_TOKEN': 'offline-fixture-token', 'CALLS': str(root / 'calls')}
            subprocess.run(['bash', '-euo', 'pipefail', '-c', script], env=environment,
                           capture_output=True, text=True, check=True)
            calls = (root / 'calls').read_text()
            self.assertIn('runs/123/attempts/2/jobs?per_page=100', calls)
            self.assertIn('artifacts/456/zip', calls)
            self.assertNotIn('invalid', calls)
            self.assertNotIn('offline-fixture-token', calls)
            self.assertTrue((root / 'comparison/warm.unavailable.txt').is_file())
            self.assertTrue((root / 'comparison/cold.zip').is_file())
            self.assertTrue((root / 'comparison/cold.metadata.json').is_file())
            self.assertTrue((root / 'comparison/jobs.json').is_file())

    def test_compare_only_executes_trusted_orchestrator_code(self):
        step = named(self.jobs['compare'], 'Verify and compare complete pair')
        self.assertEqual(step['if'], 'always()')
        self.assertIn('python3 "$GITHUB_WORKSPACE/orchestrator/proposals/ci-acceleration/collect_benchmark.py"', step['run'])
        for argument in ('--cold-id', '--warm-id', '--cold-zip', '--cold-metadata', '--warm-zip', '--warm-metadata', '--jobs', '--output'):
            self.assertIn(argument, step['run'])
        self.assertEqual(step['env']['COLD_ARTIFACT_ID'], '${{ needs.cold.outputs.artifact_id }}')
        self.assertEqual(step['env']['WARM_ARTIFACT_ID'], '${{ needs.warm.outputs.artifact_id }}')
        output = named(self.jobs['compare'], 'Upload comparison diagnostics')
        self.assertEqual(output['if'], 'always()')
        self.assertNotIn('.zip', output['with']['path'])

    def test_all_shell_blocks_parse_with_fail_closed_defaults(self):
        self.assertEqual(self.workflow['defaults']['run']['shell'], 'bash --noprofile --norc -euo pipefail {0}')
        for job in self.jobs.values():
            for step in job['steps']:
                self.assertNotIn('continue-on-error', step)
                if 'run' in step:
                    subprocess.run(['bash', '-n'], input=step['run'], text=True,
                                   capture_output=True, check=True)

    def test_no_publication_login_installers_or_promoted_candidate(self):
        text = WORKFLOW.read_text()
        for forbidden in ('contents: write', 'actions: write', 'id-token:', 'workflow_dispatch:',
                          'workflow_run:', 'pull_request_target:', 'release-candidate',
                          'gh auth login', 'gh release ', 'git push', 'curl |', 'curl -sSL',
                          'setup-arduino-cli@', 'setup-python@', 'core update-index'):
            self.assertNotIn(forbidden, text)

    def test_duplicate_yaml_keys_are_not_silently_accepted(self):
        with self.assertRaisesRegex(ValueError, 'Duplicate YAML key'):
            yaml.load('permissions:\n  contents: read\n  contents: write\n', Loader=StrictLoader)


if __name__ == '__main__':
    unittest.main()
