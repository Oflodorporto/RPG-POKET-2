#!/usr/bin/env python3
"""Offline fixture tests. No genuine CI context, qualification or publication."""
import ast
import copy
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve()
REPO = HERE.parents[3]
SPEC = importlib.util.spec_from_file_location('benchmark_adapter', HERE.parents[1] / 'benchmark_adapter.py')
a = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(a)


def context():
    sha = '1' * 40
    ref = a.BENCHMARK_REF
    return {'schema': 1, 'source': dict(a.SOURCE), 'orchestrator': {
        'repository': a.REPOSITORY, 'repository_id': '123', 'commit': sha, 'tree': '2' * 40,
        'ref': ref, 'workflow_ref': a.REPOSITORY + '/' + a.WORKFLOW + '@' + ref,
        'workflow_sha': sha, 'workflow_sha256': a.digest(b'name: offline fixture\n')},
        'run': {'event': 'push', 'id': '456', 'attempt': '1'},
        'benchmark': {'comparison_id': 'fixture-pair', 'phase': 'cold', 'aggregate_run_id': 'ci-' + sha}}


def environment(c):
    o, r = c['orchestrator'], c['run']
    return {'GITHUB_ACTIONS': 'true', 'GITHUB_REPOSITORY': o['repository'],
            'GITHUB_REPOSITORY_ID': o['repository_id'], 'GITHUB_SHA': o['commit'],
            'GITHUB_REF': o['ref'], 'GITHUB_WORKFLOW_REF': o['workflow_ref'],
            'GITHUB_WORKFLOW_SHA': o['workflow_sha'], 'GITHUB_EVENT_NAME': r['event'],
            'GITHUB_RUN_ID': r['id'], 'GITHUB_RUN_ATTEMPT': r['attempt'],
            'RPG_DETECT_LEAKS': '1', 'PYTHONDONTWRITEBYTECODE': '1'}


class AdapterTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.source, self.orchestrator, self.output = root / 'source', root / 'orchestrator', root / 'output'
        self.source.mkdir()
        self.orchestrator.mkdir()
        for name in a.PINNED:
            destination = self.source / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes((REPO / name).read_bytes())
        workflow = self.orchestrator / a.WORKFLOW
        workflow.parent.mkdir(parents=True)
        workflow.write_bytes(b'name: offline fixture\n')
        adapter = self.orchestrator / a.ADAPTER
        adapter.parent.mkdir(parents=True)
        adapter.write_bytes((HERE.parents[1] / 'benchmark_adapter.py').read_bytes())
        self.context = context()
        self.env = environment(self.context)

    def generate(self):
        # Explicit fixture isolation. Never used in production runtime code.
        with patch.object(a, 'validate_checkouts'):
            return a.materialize(self.source, self.orchestrator, self.output, self.context, self.env)

    def verify(self):
        with patch.object(a, 'validate_checkouts'):
            return a.verify_materialized(self.output, self.env)

    def test_valid_context_and_no_environment_mutation(self):
        before = copy.deepcopy(self.env)
        a.validate_context(self.context, self.env)
        self.assertEqual(self.env, before)

    def test_every_environment_binding_is_required(self):
        for key in self.env:
            with self.subTest(key=key):
                bad = dict(self.env)
                bad[key] = 'untrusted'
                with self.assertRaises(a.Rejected):
                    a.validate_context(self.context, bad)

    def test_reject_source_identity_changes(self):
        for field in a.SOURCE:
            bad = copy.deepcopy(self.context)
            bad['source'][field] = '0' * 40
            with self.assertRaises(a.Rejected):
                a.validate_context(bad, self.env)

    def test_reject_policy_relaxations(self):
        mutations = [('orchestrator', 'ref', 'refs/heads/benchmark/coopdiag77-cache'),
                     ('orchestrator', 'ref', a.BENCHMARK_REF + '-other'),
                     ('orchestrator', 'ref', 'refs/heads/main'),
                     ('orchestrator', 'ref', 'refs/heads/feat/coopdiag77-release'),
                     ('orchestrator', 'repository', 'other/fork'),
                     ('orchestrator', 'workflow_sha', '3' * 40),
                     ('orchestrator', 'commit', a.SOURCE['commit']),
                     ('orchestrator', 'repository_id', True),
                     ('run', 'event', 'workflow_dispatch'), ('run', 'event', 'pull_request'),
                     ('run', 'attempt', '0'), ('benchmark', 'phase', 'focused'),
                     ('benchmark', 'aggregate_run_id', 'ci-' + a.SOURCE['commit'])]
        for section, field, value in mutations:
            with self.subTest(section=section, field=field, value=value):
                bad = copy.deepcopy(self.context)
                bad[section][field] = value
                with self.assertRaises(a.Rejected):
                    a.validate_context(bad, self.env)

    def test_reject_unknown_fields_and_schema_bool(self):
        for section in (None, 'orchestrator', 'run', 'benchmark'):
            bad = copy.deepcopy(self.context)
            (bad if section is None else bad[section])['extra'] = True
            with self.assertRaises(a.Rejected):
                a.validate_context(bad, self.env)
        bad = copy.deepcopy(self.context)
        bad['schema'] = True
        with self.assertRaises(a.Rejected):
            a.validate_context(bad, self.env)

    def test_checkout_identity_and_cleanliness(self):
        responses = {}
        for root, sha, tree, status in ((self.source, a.SOURCE['commit'], a.SOURCE['tree'], ''),
                                       (self.orchestrator, '1' * 40, '2' * 40, '')):
            responses[(root, ('rev-parse', 'HEAD'))] = sha
            responses[(root, ('rev-parse', 'HEAD^{tree}'))] = tree
            responses[(root, ('status', '--porcelain', '--untracked-files=all'))] = status
        responses[(self.source, ('rev-list', '--parents', '-n', '1', 'HEAD'))] = a.SOURCE['commit'] + ' ' + a.SOURCE['parent']
        reader = lambda root, *args: responses[(root, args)]
        a.validate_checkouts(self.context, self.source, self.orchestrator, reader)
        for key in responses:
            saved = responses[key]
            responses[key] = 'bad identity or dirty tree'
            with self.assertRaises(a.Rejected):
                a.validate_checkouts(self.context, self.source, self.orchestrator, reader)
            responses[key] = saved
        responses[(self.source, ('status', '--porcelain', '--untracked-files=all'))] = '?? release/firmware.bin'
        a.validate_checkouts(self.context, self.source, self.orchestrator, reader)
        (self.orchestrator / a.WORKFLOW).write_text('changed')
        with self.assertRaises(a.Rejected):
            a.validate_checkouts(self.context, self.source, self.orchestrator, reader)

    def test_reject_overlapping_and_symlink_roots(self):
        for bad in (self.source, self.source / 'nested'):
            with self.assertRaises(a.Rejected):
                a.validate_roots(self.source, self.orchestrator, bad)
        alias = self.source.parent / 'alias'
        alias.symlink_to(self.source, target_is_directory=True)
        with self.assertRaises(a.Rejected):
            a.validate_roots(alias, self.orchestrator, self.output)

    def test_hash_pins_and_anchor_counts_fail_closed(self):
        for name in a.PINNED:
            path = self.source / name
            saved = path.read_bytes()
            path.write_bytes(saved + b'\n')
            with self.assertRaises(a.Rejected):
                a.load_originals(self.source)
            path.write_bytes(saved)
        with self.assertRaises(a.Rejected):
            a.replace_exact('one', 'missing', 'other')
        with self.assertRaises(a.Rejected):
            a.replace_exact('one one', 'one', 'other')

    def test_materialization_preserves_source_and_records_full_patches(self):
        before = {n: (self.source / n).read_bytes() for n in a.PINNED}
        manifest = self.generate()
        self.assertEqual(before, {n: (self.source / n).read_bytes() for n in a.PINNED})
        self.assertEqual(manifest, self.verify())
        self.assertTrue(manifest['cannot_publish'])
        self.assertTrue(manifest['benchmark_only'])
        self.assertIs(manifest['published'], False)
        self.assertEqual(set(manifest['complete_patches']), {'check_scope.py', 'record_identity.py', 'test_coopdiag77.sh'})
        for patch_text in manifest['complete_patches'].values():
            self.assertTrue(patch_text.startswith('--- tools/'))
            self.assertIn('\n+++ ', patch_text)
        for name in ('check_scope.py', 'record_identity.py', 'benchmark_runtime.py'):
            ast.parse((self.output / name).read_text())
        subprocess.run(['bash', '-n', str(self.output / 'test_coopdiag77.sh')], check=True)
        with self.assertRaises(a.Rejected), patch.object(a, 'validate_checkouts'):
            a.materialize(self.source, self.orchestrator, self.output, self.context, self.env)

    def test_initial_materialization_rejects_existing_build_and_generated_release(self):
        build = self.source / 'build'
        build.mkdir()
        (build / 'old-checkpoints.json').write_text('{}')
        with self.assertRaisesRegex(a.Rejected, 'prior checkpoints'):
            self.generate()
        (build / 'old-checkpoints.json').unlink()
        build.rmdir()
        (self.source / 'release').mkdir()
        for name in a.GENERATED:
            path = self.source / name
            path.write_bytes(b'old generated result')
            with self.subTest(name=name), self.assertRaisesRegex(a.Rejected, 'generated release'):
                self.generate()
            path.unlink()
        # A dangling build or release symlink is also evidence of a reused path.
        build.symlink_to(self.source / 'missing-build', target_is_directory=True)
        with self.assertRaisesRegex(a.Rejected, 'prior checkpoints'):
            self.generate()
        build.unlink()
        release_file = self.source / 'release/firmware.bin'
        release_file.symlink_to(self.source / 'missing-firmware')
        with self.assertRaisesRegex(a.Rejected, 'generated release'):
            self.generate()

    def test_runtime_verification_allows_new_build_and_generated_release(self):
        self.generate()
        build = self.source / 'build'
        build.mkdir()
        (build / 'new-checkpoints.json').write_text('{}')
        (self.source / 'release').mkdir()
        for name in a.GENERATED:
            (self.source / name).write_bytes(b'new result from this run')
        self.verify()

    def test_wrapper_changes_exactly_two_scope_lines(self):
        self.generate()
        old = (self.source / 'tools/test_coopdiag77.sh').read_text().splitlines()
        new = (self.output / 'test_coopdiag77.sh').read_text().splitlines()
        self.assertEqual(len(old), len(new))
        changed = [(x, y) for x, y in zip(old, new) if x != y]
        self.assertEqual(len(changed), 2)
        self.assertTrue(all(x == 'python3 tools/check_coopdiag77_scope.py' for x, _ in changed))
        self.assertIn('--run-id "ci-${GITHUB_SHA:-local-$(date +%s)}" --jobs 3', '\n'.join(new))
        self.assertNotIn('export GITHUB_', '\n'.join(new))

    def test_scope_original_assertions_are_all_preserved(self):
        self.generate()
        old = ast.parse((self.source / 'tools/check_coopdiag77_scope.py').read_text())
        new = ast.parse((self.output / 'check_scope.py').read_text())
        assertions = lambda tree: [ast.dump(node) for node in ast.walk(tree) if isinstance(node, ast.Assert)]
        self.assertEqual(assertions(old), assertions(new))
        self.assertIn("os.environ.get('GITHUB_ACTIONS')=='true',_benchmark['context']['source']['commit'])", (self.output / 'check_scope.py').read_text())

    def test_identity_all_non_context_assertions_preserved(self):
        self.generate()
        old = ast.parse((self.source / 'tools/record_coopdiag77_identity.py').read_text())
        new = ast.parse((self.output / 'record_identity.py').read_text())
        functions = lambda tree: {n.name: n for n in tree.body if isinstance(n, ast.FunctionDef)}
        old_f, new_f = functions(old), functions(new)
        self.assertEqual(set(old_f), set(new_f))
        for name in old_f.keys() - {'main'}:
            self.assertEqual(ast.dump(old_f[name]), ast.dump(new_f[name]), name)
        assertions = lambda tree: [ast.dump(node) for node in ast.walk(tree) if isinstance(node, ast.Assert)]
        # Only the original first three identity/context assertions are adapted.
        self.assertEqual(assertions(old_f['main'])[3:], assertions(new_f['main'])[2:])
        text = (self.output / 'record_identity.py').read_text()
        self.assertIn('benchmark_only=True,cannot_publish=True,published=False', text)
        self.assertIn('coopdiag77-benchmark-identity.json', text)
        self.assertNotIn("Path('build/coopdiag77-source-identity.json')", text)

    def test_reject_every_materialized_file_change(self):
        self.generate()
        for path in self.output.iterdir():
            saved = path.read_bytes()
            path.write_bytes(saved + b'\n# unapproved\n')
            with self.subTest(path=path.name), self.assertRaises((a.Rejected, json.JSONDecodeError)):
                self.verify()
            path.write_bytes(saved)

    def test_manifest_field_types_are_strict(self):
        self.generate()
        path = self.output / 'adapter-manifest.json'
        original = json.loads(path.read_text())
        for key, value in (('schema', True), ('benchmark_only', 1), ('cannot_publish', 1), ('published', 0)):
            altered = copy.deepcopy(original)
            altered[key] = value
            path.write_text(json.dumps(altered))
            with self.assertRaises(a.Rejected):
                self.verify()
        path.write_text(json.dumps(original))
        self.verify()

    def test_manifest_cannot_approve_new_generated_bytes(self):
        self.generate()
        target = self.output / 'check_scope.py'
        target.write_text(target.read_text().replace('assert ', '# removed assertion: '))
        manifest_path = self.output / 'adapter-manifest.json'
        manifest = json.loads(manifest_path.read_text())
        manifest['generated_sha256']['check_scope.py'] = a.digest(target.read_bytes())
        manifest_path.write_text(json.dumps(manifest))
        with self.assertRaises(a.Rejected):
            self.verify()

    def test_reject_extra_file_or_symlink(self):
        self.generate()
        extra = self.output / 'unapproved.txt'
        extra.write_text('extra')
        with self.assertRaises(a.Rejected):
            self.verify()
        extra.unlink()
        path = self.output / 'check_scope.py'
        content = path.read_bytes()
        path.unlink()
        elsewhere = self.output.parent / 'elsewhere.py'
        elsewhere.write_bytes(content)
        path.symlink_to(elsewhere)
        with self.assertRaises(a.Rejected):
            self.verify()

    def test_optimized_runtime_rejects_before_any_gate(self):
        self.generate()
        result = subprocess.run([sys.executable, '-O', str(self.output / 'check_scope.py')],
                                cwd=self.source, env={**os.environ, **self.env}, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('optimized Python is forbidden', result.stderr)

    def test_duplicate_context_json_rejected(self):
        path = self.source.parent / 'bad.json'
        path.write_text('{"schema": 1, "schema": 1}')
        with self.assertRaises(a.Rejected):
            a.json_read(path)


class AggregateTranscriptTests(unittest.TestCase):
    def setUp(self):
        plan = json.loads((HERE.parents[1] / 'reviewed-plan.json').read_text())
        self.commands = {'preparation': plan['preparation'], **dict(plan['tasks'])}
        self.summary = {'source_sha256': '7' * 64, 'all_passed': True, 'tasks': 157,
                        'failed': [], 'lsan': 'detect_leaks=1', 'esp32_build': False, 'publication': False}
        # The frozen preflight runs 4-, 5- and 9-group mocked plans plus seven
        # intentional resume probes. Some names overlap the real aggregate.
        self.prelude = ''.join('RUN preparation\nPASSED preparation\n' for _ in range(18))
        self.prelude += 'RESUME skip preparation\n' * 7
        self.prelude += json.dumps({**self.summary, 'source_sha256': 'a' * 64, 'tasks': 9}) + '\n'
        self.events = ''.join('RUN ' + name + '\nPASSED ' + name + '\n' for name in self.commands)
        self.finish = json.dumps(self.summary) + '\n'
        self.log = self.prelude + a.AGGREGATE_START + '\n' + self.events + self.finish + 'PASS trailing gates\n'

    def verify(self, log):
        a.validate_aggregate_transcript(log, self.commands, self.summary)

    def test_nested_resume_and_runs_do_not_contaminate_fresh_aggregate(self):
        self.assertEqual(self.log.count('RUN '), 175)
        self.assertEqual(self.log.count('RESUME skip '), 7)
        self.verify(self.log)

    def test_concurrent_tasks_may_interleave_after_preparation(self):
        names = list(self.commands)[1:]
        events = ('RUN preparation\nPASSED preparation\n' +
                  ''.join('RUN ' + name + '\n' for name in names) +
                  ''.join('PASSED ' + name + '\n' for name in reversed(names)))
        self.verify(self.log.replace(self.events, events))

    def test_boundaries_summary_and_exact_coverage_fail_closed(self):
        boundary = a.AGGREGATE_START + '\n'
        changed_summary = json.dumps({**self.summary, 'tasks': 156}) + '\n'
        variants = {
            'missing start': self.log.replace(boundary, ''),
            'duplicate start': self.log + boundary,
            'changed start': self.log.replace('PASS: 113 exact-SHA', 'PASS: 114 exact-SHA'),
            'missing summary': self.log.replace(self.finish, ''),
            'duplicate summary': self.log + self.finish,
            'wrong summary': self.log.replace(self.finish, changed_summary),
            'reversed boundaries': self.finish + boundary + self.events,
            'wrong source summary': self.log.replace(self.finish, self.finish.replace('7' * 64, '8' * 64)),
        }
        for label, log in variants.items():
            with self.subTest(label=label), self.assertRaises(a.Rejected):
                self.verify(log)

    def test_progress_rejects_real_resume_missing_duplicate_unknown_and_reordered_events(self):
        name = list(self.commands)[1]
        variants = {
            'real resume': self.events.replace('RUN ' + name, 'RESUME skip ' + name),
            'extra resume': 'RESUME skip preparation\n' + self.events,
            'missing run': self.events.replace('RUN ' + name + '\n', ''),
            'missing pass': self.events.replace('PASSED ' + name + '\n', ''),
            'duplicate run': self.events + 'RUN ' + name + '\n',
            'duplicate pass': self.events.replace('RUN ' + name, 'PASSED ' + name),
            'duplicate task': self.events.replace(name, 'preparation'),
            'unknown task': self.events.replace(name, 'unknown-task'),
            'completion before start': self.events.replace('RUN ' + name + '\nPASSED ' + name,
                                                           'PASSED ' + name + '\nRUN ' + name),
            'failed task': self.events.replace('PASSED ' + name, 'FAILED ' + name),
            'unexpected stdout': self.events.replace('RUN ' + name, 'unframed output'),
            'blank line': self.events + '\n',
            'late preparation': self.events.replace('RUN preparation\nPASSED preparation\n', '') +
                                'RUN preparation\nPASSED preparation\n',
        }
        for label, events in variants.items():
            with self.subTest(label=label), self.assertRaises(a.Rejected):
                self.verify(self.log.replace(self.events, events))


if __name__ == '__main__':
    unittest.main(verbosity=2)
