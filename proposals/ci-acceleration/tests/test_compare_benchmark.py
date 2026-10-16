from copy import deepcopy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import compare_benchmark as target


def fixture():
    identity = {key: 'fixed-observed-version' for key in target.IDENTITY}
    identity.update(source_commit=target.SOURCE_COMMIT, source_tree=target.SOURCE_TREE, orchestrator_sha='c'*40,
                    source_inventory_sha256=target.SOURCE_INVENTORY_SHA256, reviewed_plan_sha256=target.PLAN_SHA256,
                    adapter_sha256='f'*64, dependency_lock_sha256='1'*64,
                    host_jobs=3, esp32_jobs=4, arduino_cli='1.5.2-rc.1',
                    esp32_core='3.3.12', pillow='11.3.0', pyyaml='6.0.3',
                    runner_image='ubuntu-24.04', runner_image_version='20261004.327.1',
                    runner_os='Linux', runner_arch='X64', python='3.12.3',
                    host_compiler=target.HOST_COMPILER, target_compiler='esp-x32/2601', fqbn=target.FQBN)
    cold = {'schema': 1, 'kind': 'benchmark-only-cold', 'identity': identity,
            'run': {'repository': 'Oflodorporto/RPG-POKET-2', 'id': 1, 'attempt': 1,
                    'job_id': 11, 'branch': 'bench/coopdiag77-cache-promotion', 'status': 'success'},
            'cache': {'key': target.cache_key(identity, 'benchmark-fixture', 1, 1), 'benchmark_salt': 'benchmark-fixture',
                      'hit': False, 'restored': False,
                      'writer_run_id': 1, 'writer_job_id': 11, 'contents_verified': True,
                      'forbidden_paths': [], 'bytes': 50},
            'gates': {key: 'passed' for key in target.GATES},
            'coverage': {'groups': 157, 'skipped': 0, 'resumed': 0, 'lsan': 'detect_leaks=1',
                         'asan': True, 'ubsan_halt_on_error': True, 'assertions': True},
            'phases_ns': {key: 100 for key in target.PHASES}, 'job_ns': 2000,
            'release_files': {key: {'bytes': 10, 'sha256': 'a'*64} for key in
                              ('firmware.bin', 'artes.pak', 'manifest.json')},
            'publish_authorized': False}
    warm = deepcopy(cold)
    warm['kind'] = 'benchmark-only-warm'
    warm['run']['job_id'] = 12
    warm['cache'].update(hit=True, restored=True)
    warm['phases_ns']['dependency_install'] = 50
    warm['job_ns'] = 1950
    return cold, warm


class Tests(unittest.TestCase):
    def test_consistent_pair_is_not_authorization(self):
        result = target.compare(*fixture())
        self.assertEqual(result['job_saved_ns'], 50)
        self.assertIs(result['publish_authorized'], False)
        self.assertIs(result['authenticated_provenance'], False)

    def test_rejects_all_identity_drift(self):
        for key in target.IDENTITY:
            cold, warm = fixture()
            warm['identity'][key] = 'drift'
            with self.subTest(key=key), self.assertRaises(target.Rejected):
                target.compare(cold, warm)

    def test_rejects_missing_gate(self):
        for key in target.GATES:
            cold, warm = fixture()
            warm['gates'][key] = 'skipped'
            with self.subTest(key=key), self.assertRaises(target.Rejected):
                target.compare(cold, warm)

    def test_rejects_checkpoint_reuse_and_disabled_lsan(self):
        for key, value in [('groups', 143), ('skipped', 1), ('resumed', 1),
                           ('lsan', 'detect_leaks=0'), ('asan', False),
                           ('ubsan_halt_on_error', False), ('assertions', False), ('skipped', False)]:
            cold, warm = fixture()
            warm['coverage'][key] = value
            with self.subTest(key=key), self.assertRaises(target.Rejected):
                target.compare(cold, warm)

    def test_cache_miss_or_wrong_writer_is_not_warm(self):
        for key, value in [('hit', False), ('restored', False), ('writer_run_id', 2),
                           ('writer_job_id', 999), ('key', 'ci-downloads-v1-benchmark-other-' + '1'*64),
                           ('contents_verified', False), ('forbidden_paths', ['build/checkpoints.json']), ('bytes', 60)]:
            cold, warm = fixture()
            warm['cache'][key] = value
            with self.subTest(key=key), self.assertRaises(target.Rejected):
                target.compare(cold, warm)

    def test_missing_overhead_and_bad_durations(self):
        for value in [-1, True, 1.1]:
            cold, warm = fixture()
            warm['phases_ns']['cache_save'] = value
            with self.assertRaises(target.Rejected):
                target.compare(cold, warm)
        cold, warm = fixture()
        warm['job_ns'] = 5
        with self.assertRaises(target.Rejected):
            target.compare(cold, warm)

    def test_slower_warm_run_stays_negative(self):
        cold, warm = fixture()
        warm['job_ns'] = 2500
        self.assertEqual(target.compare(cold, warm)['job_saved_ns'], -500)

    def test_distinct_builds_need_not_have_equal_hashes(self):
        cold, warm = fixture()
        warm['release_files']['firmware.bin']['sha256'] = 'b'*64
        target.compare(cold, warm)

    def test_same_job_is_not_two_measurements(self):
        cold, warm = fixture()
        warm['run']['job_id'] = cold['run']['job_id']
        with self.assertRaises(target.Rejected):
            target.compare(cold, warm)

    def test_key_uses_real_cache_policy_dimensions(self):
        cold, warm = fixture()
        self.assertEqual(cold['cache']['key'], target.cache_key(cold['identity'], 'benchmark-fixture', 1, 1))
        cold['cache']['key'] = warm['cache']['key'] = 'ci-downloads-v1-benchmark--' + '1'*64
        with self.assertRaises(target.Rejected):
            target.compare(cold, warm)

    def test_unknown_identity_in_both_reports_is_not_controlled(self):
        for key in ('runner_image', 'runner_image_version', 'runner_os', 'runner_arch', 'python',
                    'host_compiler', 'target_compiler', 'fqbn'):
            cold, warm = fixture()
            cold['identity'][key] = warm['identity'][key] = 'unknown'
            with self.subTest(key=key), self.assertRaises(target.Rejected):
                target.compare(cold, warm)

    def test_cache_overhead_cannot_be_omitted(self):
        for side, key in ((0, 'cache_save'), (1, 'dependency_restore')):
            pair = fixture()
            pair[side]['phases_ns'][key] = 0
            with self.assertRaises(target.Rejected):
                target.compare(*pair)


if __name__ == '__main__':
    unittest.main(verbosity=2)
