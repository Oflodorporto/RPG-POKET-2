#!/usr/bin/env python3
"""Synthetic unit fixtures only. Never uses fabricated context for a real run."""
import copy
from datetime import datetime, timezone
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
HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))
import rehearse as r
import setup_host


def fixture():
    p = r.PIN
    repo = {'id': p['repository_id'], 'full_name': p['repository']}
    run = {'id': p['run_id'], 'run_attempt': p['attempt'], 'workflow_id': p['workflow_id'],
        'path': p['workflow_path'], 'event': 'push', 'head_sha': p['source_commit'], 'head_branch': p['branch'],
        'status': 'completed', 'conclusion': 'success', 'repository': repo.copy(), 'head_repository': repo.copy()}
    steps = [{'status': 'completed', 'conclusion': 'success'} for _ in range(12)]
    job = {'id': p['job_id'], 'run_id': p['run_id'], 'run_attempt': p['attempt'],
        'head_sha': p['source_commit'], 'head_branch': p['branch'], 'name': p['job_name'],
        'status': 'completed', 'conclusion': 'success', 'steps': steps,
        'started_at': '2026-10-15T15:39:15Z', 'completed_at': '2026-10-15T16:30:58Z'}
    artifact = {'id': p['artifact_id'], 'name': p['artifact_name'], 'size_in_bytes': p['zip_bytes'],
        'digest': 'sha256:'+p['zip_sha256'], 'expired': False, 'created_at': '2026-10-15T16:30:55Z',
        'expires_at': '2026-10-29T16:30:44Z', 'workflow_run': {'id': p['run_id'],
        'repository_id': p['repository_id'], 'head_repository_id': p['repository_id'],
        'head_branch': p['branch'], 'head_sha': p['source_commit']}}
    commit = {'sha': p['source_commit'], 'tree': {'sha': p['source_tree']}, 'parents': [{'sha': p['source_parent']}]}
    return [run, copy.deepcopy(run), [{'total_count': 1, 'jobs': [job]}],
        [{'total_count': 1, 'artifacts': [copy.deepcopy(artifact)]}], artifact, commit,
        datetime(2026, 10, 16, tzinfo=timezone.utc)]


class API(unittest.TestCase):
    def test_good_historical_identity_without_current_environment(self):
        before = dict(os.environ)
        self.assertEqual(r.validate_api(*fixture())['id'], r.PIN['job_id'])
        self.assertEqual(dict(os.environ), before)

    def test_run_mutations(self):
        for record in (0, 1):
            for field, value in [('id', 1), ('run_attempt', 2), ('run_attempt', True), ('workflow_id', 1),
                ('path', '.github/workflows/release.yml'), ('event', 'pull_request'), ('head_sha', 'a'*40),
                ('head_branch', 'main'), ('status', 'in_progress'), ('conclusion', 'cancelled')]:
                with self.subTest(record=record, field=field, value=value):
                    f = fixture(); f[record][field] = value
                    with self.assertRaises(r.Rejected): r.validate_api(*f)

    def test_fork_and_repository_mutations(self):
        for index in (0, 1):
            for key in ('repository', 'head_repository'):
                f = fixture(); f[index][key]['id'] = 1
                with self.assertRaises(r.Rejected): r.validate_api(*f)
                f = fixture(); f[index][key]['full_name'] = 'someone/fork'
                with self.assertRaises(r.Rejected): r.validate_api(*f)

    def test_job_mutations(self):
        for field, value in [('id', 1), ('run_id', 1), ('run_attempt', 2), ('head_sha', '0'*40),
            ('head_branch', 'main'), ('name', 'other'), ('status', 'queued'), ('conclusion', 'failure'),
            ('completed_at', '2026-10-17T00:00:00Z'), ('started_at', '2026-10-15T17:00:00Z')]:
            with self.subTest(field=field):
                f = fixture(); f[2][0]['jobs'][0][field] = value
                with self.assertRaises(r.Rejected): r.validate_api(*f)

    def test_step_failure(self):
        f = fixture(); f[2][0]['jobs'][0]['steps'][8]['conclusion'] = 'skipped'
        with self.assertRaises(r.Rejected): r.validate_api(*f)

    def test_artifact_mutations(self):
        for field, value in [('id', 1), ('name', 'movable-name'), ('size_in_bytes', 1), ('digest', None),
            ('expired', True), ('expired', 0), ('created_at', '2026-10-15T17:00:00Z'),
            ('expires_at', '2026-10-16T00:09:00Z')]:
            with self.subTest(field=field):
                f = fixture(); f[4][field] = value; f[3][0]['artifacts'][0] = copy.deepcopy(f[4])
                with self.assertRaises(r.Rejected): r.validate_api(*f)

    def test_artifact_list_detail_disagreement(self):
        f = fixture(); f[4]['id'] = 1
        with self.assertRaises(r.Rejected): r.validate_api(*f)

    def test_artifact_run_binding(self):
        for field in fixture()[4]['workflow_run']:
            f = fixture(); f[4]['workflow_run'][field] = 'bad'; f[3][0]['artifacts'][0] = copy.deepcopy(f[4])
            with self.assertRaises(r.Rejected): r.validate_api(*f)

    def test_commit_tree_parent(self):
        for edit in ('sha', 'tree', 'parents'):
            f = fixture(); f[5][edit] = {'sha': '0'*40} if edit == 'tree' else [] if edit == 'parents' else '0'*40
            with self.assertRaises(r.Rejected): r.validate_api(*f)

    def test_missing_duplicate_or_incomplete_pages(self):
        for index, key in ((2, 'jobs'), (3, 'artifacts')):
            f = fixture(); f[index] = []
            with self.assertRaises(r.Rejected): r.validate_api(*f)
            f = fixture(); f[index][0]['total_count'] = 2
            with self.assertRaises(r.Rejected): r.validate_api(*f)
            f = fixture(); f[index][0][key] *= 2; f[index][0]['total_count'] = 2
            with self.assertRaises(r.Rejected): r.validate_api(*f)


class CurrentIdentity(unittest.TestCase):
    def context(self):
        # Synthetic test dictionary, never installed into the real environment.
        commit = '1' * 40
        return {'GITHUB_ACTIONS': 'true', 'GITHUB_REPOSITORY': r.PIN['repository'],
            'GITHUB_REPOSITORY_ID': str(r.PIN['repository_id']), 'GITHUB_SHA': commit,
            'GITHUB_WORKFLOW_SHA': commit, 'GITHUB_EVENT_NAME': 'push', 'GITHUB_REF': r.REF,
            'GITHUB_WORKFLOW_REF': r.PIN['repository'] + '/' + r.WORKFLOW + '@' + r.REF,
            'GITHUB_RUN_ID': '999999', 'GITHUB_RUN_ATTEMPT': '1'}

    def git_fixture(self, root, *args):
        return '1' * 40 if args == ('rev-parse', 'HEAD') else ''

    def test_distinct_current_identity(self):
        env = self.context()
        with patch.object(r, 'git', side_effect=self.git_fixture):
            result = r.current_identity(Path('/synthetic-test'), env)
        self.assertEqual(result['GITHUB_RUN_ID'], '999999')
        self.assertNotEqual(result['GITHUB_SHA'], r.PIN['source_commit'])

    def test_each_current_identity_field_is_checked(self):
        for key in self.context():
            with self.subTest(key=key), patch.object(r, 'git', side_effect=self.git_fixture):
                env = self.context(); env[key] = 'wrong'
                with self.assertRaises(r.Rejected): r.current_identity(Path('/synthetic-test'), env)

    def test_historical_run_cannot_be_current(self):
        env = self.context(); env['GITHUB_RUN_ID'] = str(r.PIN['run_id'])
        with patch.object(r, 'git', side_effect=self.git_fixture), self.assertRaises(r.Rejected):
            r.current_identity(Path('/synthetic-test'), env)

    def test_no_ignored_source_exclusion(self):
        import inspect
        self.assertNotIn('--exclude-standard', inspect.getsource(r.validate_source))



class Payload(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.source = self.root/'source'; self.source.mkdir()
        self.work = self.root/'work'; self.work.mkdir()
        self.data = {n: ('synthetic-test-only-'+n).encode() for n in r.NAMES}
        self.pin = patch.dict(r.PIN, {'payload': {n: r.descriptor(d) for n,d in self.data.items()}})
        self.pin.start()
        class Archive:
            def __init__(s, data): s.data = data
            def read(s, name): return s.data[name.removeprefix('release/')]
        self.archive = Archive(self.data)

    def tearDown(self):
        self.pin.stop(); self.temp.cleanup()

    def test_copy_only_exact_triple(self):
        r.copy_payload(self.archive, self.source)
        self.assertEqual(r.payload_hashes(self.source/'release'), r.PIN['payload'])
        self.assertEqual({x.name for x in self.source.iterdir()}, {'release'})

    def test_reuse_refused(self):
        r.copy_payload(self.archive, self.source)
        with self.assertRaises(FileExistsError): r.copy_payload(self.archive, self.source)

    def test_symlink_destination_refused(self):
        (self.source/'release').symlink_to(self.work)
        with self.assertRaises(FileExistsError): r.copy_payload(self.archive, self.source)

    def test_tamper_before_copy(self):
        self.data['firmware.bin'] += b'changed'
        with self.assertRaises(r.Rejected): r.copy_payload(self.archive, self.source)

    def test_symlink_file_refused(self):
        r.copy_payload(self.archive, self.source)
        (self.work/'target').write_bytes(self.data['firmware.bin'])
        (self.source/'release/firmware.bin').unlink()
        (self.source/'release/firmware.bin').symlink_to(self.work/'target')
        with self.assertRaises(r.Rejected): r.payload_hashes(self.source/'release')

    def test_extra_payload_refused(self):
        r.copy_payload(self.archive, self.source)
        (self.source/'release/executable').write_text('bad')
        with self.assertRaises(r.Rejected): r.payload_hashes(self.source/'release')

    def gates(self, callback):
        r.copy_payload(self.archive, self.source)
        env = {'RPG_DETECT_LEAKS': '1', 'PYTHONDONTWRITEBYTECODE': '1'}
        with patch.object(r, 'validate_source'), patch.object(r.subprocess, 'run', side_effect=callback):
            return r.run_gates(self.source, self.work, env)

    def test_four_commands_never_manifest_generation(self):
        if sys.flags.optimize:
            self.skipTest('Actual product gates deliberately reject optimized interpreter')
        calls = []
        def command(argv, **kw):
            calls.append(argv); kw['stdout'].write(b'explicit unit fixture\n'); return subprocess.CompletedProcess(argv, 0)
        result = self.gates(command)
        self.assertEqual(len(result), 4)
        self.assertEqual([x['reviewed_command'] for x in result], [list(c) for _,c in r.COMMANDS])
        self.assertTrue(all(c[0] == sys.executable and c[1:4] == ['-B','-s','-E'] for c in calls))
        self.assertFalse(any('release_coopdiag77_manifest.py' in ' '.join(c) for c in calls))
        self.assertEqual(json.loads((self.work/'hashes-before.json').read_text()),json.loads((self.work/'hashes-after.json').read_text()))

    def test_gate_failure_records_hashes_and_stops(self):
        if sys.flags.optimize: self.skipTest('Real gate execution rejects optimized interpreter')
        with self.assertRaises(r.Rejected):
            self.gates(lambda argv, **kw: subprocess.CompletedProcess(argv, 7))
        self.assertEqual(len(json.loads((self.work/'commands.json').read_text())), 1)
        self.assertTrue((self.work/'hashes-after.json').is_file())

    def test_gate_mutation_rejected(self):
        if sys.flags.optimize: self.skipTest('Real gate execution rejects optimized interpreter')
        def mutate(argv, **kw):
            (self.source/'release/manifest.json').write_bytes(b'changed')
            return subprocess.CompletedProcess(argv, 0)
        with self.assertRaisesRegex(r.Rejected, 'mutated'): self.gates(mutate)

    def test_leak_detection_and_optimized_rejected(self):
        for env in ({'RPG_DETECT_LEAKS':'0'}, {'RPG_DETECT_LEAKS':'1','PYTHONOPTIMIZE':'1'}):
            with self.assertRaises(r.Rejected): r.run_gates(self.source,self.work,env)


class SetupAndWorkflow(unittest.TestCase):
    def test_only_two_locked_wheels(self):
        lock, rows = setup_host.wheels()
        self.assertEqual(len(rows),2)
        self.assertEqual(sum(x['bytes'] for x in rows),7453106)
        self.assertTrue(all(x['path'].startswith('wheels/') for x in rows))

    def test_injected_runtime_rejected(self):
        for key in ('CXX','PYTHONPATH','PYTHONHOME','LD_PRELOAD','COMPILER_PATH','CPATH'):
            with self.subTest(key=key), self.assertRaises(r.Rejected): r.gate_env({key:'injected'})

    def test_token_stripped_identity_unchanged(self):
        env = {'PATH':os.environ['PATH'],'GH_TOKEN':'fixture-secret','GITHUB_TOKEN':'fixture-secret','GITHUB_SHA':'test-real-value'}
        got = r.gate_env(env)
        self.assertNotIn('GH_TOKEN',got); self.assertNotIn('GITHUB_TOKEN',got)
        self.assertEqual(got['GITHUB_SHA'],env['GITHUB_SHA'])
        self.assertEqual(env['GH_TOKEN'],'fixture-secret')

    def test_local_mode_does_not_fabricate_github(self):
        text = (HERE/'rehearse.py').read_text()
        self.assertNotIn('os.environ[',text)
        self.assertNotIn('os.environ.update',text)
        self.assertNotIn('adapter.validate_context(',text)
        self.assertIn('Local snapshots forbidden in Actions',text)

    def test_api_uses_only_get_with_pagination(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(r.subprocess, 'run') as call:
            r.api('/repos/test/fixture/actions/jobs?per_page=100',Path(tmp)/'response.json',pages=True)
            argv = call.call_args.args[0]
            self.assertEqual(argv[argv.index('--method')+1],'GET')
            self.assertIn('--paginate',argv); self.assertIn('--slurp',argv)
            self.assertEqual(call.call_args.kwargs['timeout'],180)

    def test_slow_api_acquisition_is_not_fresh(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(r,'api'), patch.object(r.time,'monotonic_ns',side_effect=[0,301_000_000_000]):
            with self.assertRaisesRegex(r.Rejected,'freshness'):
                r.fetch_inputs(Path(tmp))
            self.assertFalse((Path(tmp)/'fetch.json').exists())

    def test_workflow_no_write_or_release_triggers(self):
        text = (HERE/'exact-artifact-rehearsal.yml').read_text()
        self.assertNotIn('contents: write',text)
        self.assertNotIn('actions: write',text)
        self.assertNotIn('workflow_dispatch:',text)
        self.assertNotIn('pull_request',text)
        self.assertNotIn('workflow_run:',text)
        self.assertNotIn('actions/cache',text)
        self.assertNotIn('--local-snapshot',text)
        self.assertNotIn('gh release',text)
        self.assertIn('branches: [bench/coopdiag77-cache-promotion]',text)
        for action in __import__('re').findall(r'uses: ([^\s]+)',text):
            self.assertRegex(action,r'@[0-9a-f]{40}$')
        self.assertEqual(text.count('persist-credentials: false'),2)


class ActivationRouting(unittest.TestCase):
    """Offline evaluation of the exact, intentionally narrow routing expressions.

    This is not a claim that GitHub ran jobs. Paths use only exact names and
    trailing /**; contains() is case-insensitive as in Actions expressions.
    """
    BRANCH = 'bench/coopdiag77-cache-promotion'
    MARKER = '[exact-rehearsal-only]'
    MESSAGE = 'Rehearse exact qualified coopdiag77 artifact [exact-rehearsal-only]'
    REPO = 'Oflodorporto/RPG-POKET-2'
    BASE_JOB_BODY_SHA256 = 'f51dcd97d03dbe9a0e2127bef9a0c9970a166de6d20cea8922f033e44d3118dc'

    def setUp(self):
        import yaml
        self.benchmark = yaml.load((HERE.parent/'ci-acceleration/ci-acceleration-benchmark.yml').read_text(),Loader=yaml.BaseLoader)
        self.rehearsal = yaml.load((HERE/'exact-artifact-rehearsal.yml').read_text(),Loader=yaml.BaseLoader)
        self.base_expression = "github.repository == 'Oflodorporto/RPG-POKET-2' && github.event_name == 'push' && github.ref == 'refs/heads/bench/coopdiag77-cache-promotion'"
        self.guard = " && !contains(github.event.head_commit.message, '[exact-rehearsal-only]')"
        for name,job in self.benchmark['jobs'].items():
            self.assertEqual(job['if'],('always() && ' if name=='compare' else '')+self.base_expression+self.guard)
        self.assertEqual(self.rehearsal['jobs']['rehearse']['if'],self.base_expression)

    def matches(self, workflow, paths, branch=None):
        push=workflow['on']['push']
        if (branch or self.BRANCH) not in push['branches']: return False
        return any(path.startswith(pattern[:-2]) if pattern.endswith('/**') else path==pattern
            for path in paths for pattern in push['paths'])

    def routes(self, paths, message='', branch=None, repo=None, event='push'):
        base=(repo or self.REPO)==self.REPO and event=='push' and (branch or self.BRANCH)==self.BRANCH
        benchmark_event=event=='push' and self.matches(self.benchmark,paths,branch)
        rehearsal_event=event=='push' and self.matches(self.rehearsal,paths,branch)
        return {'benchmark_event':benchmark_event,
            'benchmark_jobs':base and benchmark_event and self.MARKER.lower() not in message.lower(),
            'rehearsal_job':base and rehearsal_event}

    def test_activation_changes_workflow_but_skips_entire_pair(self):
        paths=['.github/workflows/ci-acceleration-benchmark.yml',
            'proposals/ci-acceleration/ci-acceleration-benchmark.yml',
            'proposals/ci-acceleration/tests/test_benchmark_workflow.py',
            '.github/workflows/exact-artifact-rehearsal.yml',
            *['proposals/exact-artifact-rehearsal/'+x for x in ('README.md','historical-pin.json','rehearse.py','setup_host.py','exact-artifact-rehearsal.yml','tests/test_rehearse.py')]]
        self.assertEqual(self.routes(paths,self.MESSAGE),{'benchmark_event':True,'benchmark_jobs':False,'rehearsal_job':True})
        # Without the marker this same activation push really WOULD start a pair.
        self.assertTrue(self.routes(paths,'activation without marker')['benchmark_jobs'])

    def test_rehearsal_only_edits_do_not_trigger_benchmark(self):
        for path in ('.github/workflows/exact-artifact-rehearsal.yml','proposals/exact-artifact-rehearsal/rehearse.py',
            'proposals/exact-artifact-rehearsal/tests/test_rehearse.py','proposals/exact-artifact-rehearsal/README.md'):
            with self.subTest(path=path):
                self.assertEqual(self.routes([path]),{'benchmark_event':False,'benchmark_jobs':False,'rehearsal_job':True})

    def test_real_benchmark_changes_still_run_full_pair_without_marker(self):
        for path in ('.github/workflows/ci-acceleration-benchmark.yml','proposals/ci-acceleration/ci-acceleration-benchmark.yml',
            'proposals/ci-acceleration/run_benchmark.py','proposals/ci-acceleration/collect_benchmark.py',
            'proposals/ci-acceleration/tests/test_benchmark_workflow.py','proposals/ci-acceleration/dependencies/downloads.lock.json'):
            with self.subTest(path=path):
                self.assertEqual(self.routes([path],'Ordinary benchmark change'),{'benchmark_event':True,'benchmark_jobs':True,'rehearsal_job':False})

    def test_unrelated_changes_do_not_schedule_either_experiment(self):
        self.assertEqual(self.routes(['README.md','firmware/RPG_POKET_2/View.h','.github/workflows/release.yml']),
            {'benchmark_event':False,'benchmark_jobs':False,'rehearsal_job':False})

    def test_branch_repository_and_event_guards(self):
        paths=['.github/workflows/ci-acceleration-benchmark.yml','.github/workflows/exact-artifact-rehearsal.yml']
        for override in ({'branch':'main'},{'repo':'other/fork'},{'event':'pull_request'}):
            got=self.routes(paths,**override)
            self.assertFalse(got['benchmark_jobs']);self.assertFalse(got['rehearsal_job'])

    def test_marker_is_not_global_skip_ci_and_does_not_gate_rehearsal(self):
        for marker in (self.MARKER,self.MARKER.upper()):
            got=self.routes(['.github/workflows/ci-acceleration-benchmark.yml','.github/workflows/exact-artifact-rehearsal.yml'],marker)
            self.assertFalse(got['benchmark_jobs']);self.assertTrue(got['rehearsal_job'])
        self.assertNotIn('[skip ci]',self.MESSAGE.lower())
        self.assertNotIn('skip-checks:',self.MESSAGE.lower())
        self.assertNotIn(self.MARKER,self.rehearsal['jobs']['rehearse']['if'])

    def test_qualified_benchmark_job_bodies_are_unchanged(self):
        import hashlib
        body={name:{k:v for k,v in job.items() if k!='if'} for name,job in self.benchmark['jobs'].items()}
        got=hashlib.sha256(json.dumps(body,sort_keys=True,separators=(',',':')).encode()).hexdigest()
        self.assertEqual(got,self.BASE_JOB_BODY_SHA256)
        self.assertEqual(set(body),{'cold','warm','compare'})

    def test_active_and_proposal_workflows_are_byte_identical(self):
        repo=HERE.parents[1]
        for name,proposal in [('ci-acceleration-benchmark.yml',HERE.parent/'ci-acceleration/ci-acceleration-benchmark.yml'),
            ('exact-artifact-rehearsal.yml',HERE/'exact-artifact-rehearsal.yml')]:
            self.assertEqual((repo/'.github/workflows'/name).read_bytes(),proposal.read_bytes())

    def test_skipped_benchmark_jobs_cannot_be_collected_as_pass(self):
        # Explicit synthetic API records, never installed as current GITHUB context.
        context={'run':{'id':'123','attempt':'1'},'orchestrator':{'commit':'a'*40}}
        jobs=[{'id':i+1,'run_id':123,'run_attempt':1,'head_sha':'a'*40,
            'head_branch':self.BRANCH,'name':name,'status':'completed','conclusion':'success',
            'started_at':f'2026-10-16T0{i+1}:00:00Z','completed_at':f'2026-10-16T0{i+1}:30:00Z'}
            for i,name in enumerate(('cold','warm'))]
        self.assertEqual(set(r.common.jobs_from_pages([{'total_count':2,'jobs':jobs}],context)),{'cold','warm'})
        for index in (0,1):
            changed=copy.deepcopy(jobs);changed[index]['conclusion']='skipped'
            with self.subTest(job=jobs[index]['name']),self.assertRaises(r.Rejected):
                r.common.jobs_from_pages([{'total_count':2,'jobs':changed}],context)

    def test_setup_precedes_yaml_test_dependency(self):
        names=[step['name'] for step in self.rehearsal['jobs']['rehearse']['steps']]
        self.assertLess(names.index('Install only fresh pinned host wheels'),names.index('Test read-only rehearsal guards'))
        self.assertLess(names.index('Test read-only rehearsal guards'),names.index('Fetch historical artifact and recheck copied exact bytes'))


if __name__ == '__main__':
    unittest.main(verbosity=2)
