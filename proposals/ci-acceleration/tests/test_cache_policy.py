"""Synthetic, offline-only fixtures. No distribution here is installable."""
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import cache_policy as policy


class CachePolicyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="cache-policy-test-")
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.cache = self.base / "untrusted-cache"
        self.cache.mkdir()
        self.lock_path = self.base / "reviewed-lock.json"
        self.context = {
            "runner_os": "Linux", "runner_arch": "X64", "python_version": "3.12.14",
            "arduino_cli_version": "1.5.2-rc.1", "esp32_core_version": "3.3.12",
            "pillow_version": "11.3.0", "pyyaml_version": "6.0.3",
            "fqbn": "esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,PSRAM=opi",
            "benchmark_salt": "benchmark-synthetic-tests-v1",
        }
        self.payloads = {
            "wheels/Pillow-11.3.0-py3-none-any.whl": b"SYNTHETIC WHEEL FIXTURE; DO NOT INSTALL\n",
            "archives/esp32-3.3.12.zip": b"SYNTHETIC ARCHIVE FIXTURE; DO NOT EXTRACT\n",
        }
        urls = {
            "wheels/Pillow-11.3.0-py3-none-any.whl": "https://files.pythonhosted.org/packages/synthetic/Pillow-11.3.0-py3-none-any.whl",
            "archives/esp32-3.3.12.zip": "https://github.com/espressif/arduino-esp32/releases/download/3.3.12/esp32-3.3.12.zip",
        }
        self.data = {"schema": 1, "context": self.context.copy(), "artifacts": []}
        for path, raw in self.payloads.items():
            target = self.cache / path
            target.parent.mkdir(exist_ok=True)
            target.write_bytes(raw)
            self.data["artifacts"].append({"path": path, "url": urls[path], "bytes": len(raw),
                                           "sha256": hashlib.sha256(raw).hexdigest()})
        self.write_lock()

    def write_lock(self, data=None, raw=None):
        if raw is None:
            raw = json.dumps(self.data if data is None else data, sort_keys=True).encode()
        self.lock_path.write_bytes(raw)
        self.lock_sha = hashlib.sha256(raw).hexdigest()

    def load(self):
        return policy.load_trusted_lock(self.lock_path, self.lock_sha, self.cache, self.context)

    def key(self):
        return policy.exact_cache_key(self.load())

    def evaluate(self, key=None):
        return policy.evaluate_cache(self.cache, self.lock_path, self.lock_sha, self.context,
                                     self.key() if key is None else key)

    def assertMiss(self):
        result = self.evaluate()
        self.assertFalse(result.hit)
        self.assertEqual("clean_download", result.action)
        self.assertEqual(0, result.verified_files)
        return result

    def test_valid_exact_cache(self):
        result = self.evaluate()
        self.assertTrue(result.hit)
        self.assertEqual("use_verified_downloads", result.action)
        self.assertEqual(2, result.verified_files)

    def test_every_context_dimension_changes_key(self):
        original = self.key()
        alternatives = {
            "runner_os": "macOS", "runner_arch": "ARM64", "python_version": "3.12.15",
            "arduino_cli_version": "1.5.2", "esp32_core_version": "3.3.13",
            "pillow_version": "11.3.1", "pyyaml_version": "6.0.4",
            "fqbn": self.context["fqbn"] + ",FlashSize=16M",
            "benchmark_salt": "benchmark-separate-run",
        }
        for field, value in alternatives.items():
            with self.subTest(field=field):
                context = dict(self.context, **{field: value})
                lock = policy.TrustedLock(self.lock_sha, policy.validate_context(context), self.load().artifacts)
                self.assertNotEqual(original, policy.exact_cache_key(lock))

    def test_raw_lock_hash_changes_key(self):
        original = self.key()
        self.write_lock(raw=self.lock_path.read_bytes() + b"\n")
        self.assertNotEqual(original, self.key())

    def test_exact_key_not_prefix_or_extended_or_cross_os(self):
        key = self.key()
        for other in ("", key[:-1], key + "-suffix", key.replace("Linux", "Windows")):
            with self.subTest(key=other):
                self.assertFalse(self.evaluate(other).hit)
        self.assertFalse(policy.evaluate_cache(self.cache, self.lock_path, self.lock_sha, self.context, None).hit)

    def test_missing_cache_is_miss(self):
        shutil.rmtree(self.cache)
        self.assertMiss()

    def test_missing_file_is_miss(self):
        (self.cache / next(iter(self.payloads))).unlink()
        self.assertMiss()

    def test_same_size_corruption_is_miss(self):
        path = self.cache / next(iter(self.payloads))
        path.write_bytes(b"X" * path.stat().st_size)
        self.assertIn("digest", self.assertMiss().reason)

    def test_wrong_size_is_miss(self):
        path = self.cache / next(iter(self.payloads))
        path.write_bytes(path.read_bytes() + b"extra")
        self.assertIn("size", self.assertMiss().reason)

    def test_unexpected_file_is_miss_even_with_valid_expected_files(self):
        (self.cache / "wheels/unreviewed.whl").write_bytes(b"untrusted")
        self.assertMiss()

    def test_all_non_download_root_paths_rejected(self):
        for name in ("build", "results", "checkpoints", "logs", "libcache", "bin", "installed", ".venv", ".arduino15", "release"):
            with self.subTest(name=name):
                path = self.cache / name
                path.mkdir()
                self.assertMiss()
                path.rmdir()

    def test_unexpected_empty_directory_is_miss(self):
        (self.cache / "wheels/extra").mkdir()
        self.assertMiss()

    def test_cache_lock_cannot_reauthorize_poison(self):
        target = self.cache / next(iter(self.payloads))
        target.write_bytes(b"P" * target.stat().st_size)
        forged = copy.deepcopy(self.data)
        forged["artifacts"][0]["sha256"] = hashlib.sha256(target.read_bytes()).hexdigest()
        (self.cache / "reviewed-lock.json").write_text(json.dumps(forged))
        self.assertMiss()

    def test_lock_inside_cache_is_policy_error(self):
        copied = self.cache / "lock.json"
        copied.write_bytes(self.lock_path.read_bytes())
        with self.assertRaises(policy.PolicyError):
            policy.load_trusted_lock(copied, self.lock_sha, self.cache, self.context)

    def test_lock_hash_mismatch_stops(self):
        self.lock_path.write_bytes(self.lock_path.read_bytes() + b"\n")
        with self.assertRaisesRegex(policy.PolicyError, "SHA-256 mismatch"):
            self.load()

    def test_context_mismatch_stops(self):
        self.data["context"]["pillow_version"] = "11.3.1"
        self.write_lock()
        with self.assertRaisesRegex(policy.PolicyError, "context mismatch"):
            self.load()

    def test_exact_context_fields_and_versions(self):
        for field in self.context:
            with self.subTest(missing=field):
                bad = self.context.copy()
                del bad[field]
                with self.assertRaises(policy.PolicyError):
                    policy.validate_context(bad)
        for field, value in (("python_version", "3.12"), ("arduino_cli_version", "latest"),
                             ("runner_os", "any"), ("benchmark_salt", "production"),
                             ("fqbn", "esp32:*"), ("pillow_version", True)):
            with self.subTest(field=field, value=value), self.assertRaises(policy.PolicyError):
                policy.validate_context(dict(self.context, **{field: value}))

    def test_unknown_lock_and_artifact_fields_rejected(self):
        for where in ("root", "artifact"):
            bad = copy.deepcopy(self.data)
            (bad if where == "root" else bad["artifacts"][0])["restore-prefixes"] = ["unsafe-"]
            self.write_lock(bad)
            with self.subTest(where=where), self.assertRaises(policy.PolicyError):
                self.load()

    def test_duplicate_json_keys_rejected(self):
        raw = self.lock_path.read_bytes().replace(b'"schema": 1', b'"schema": 1, "schema": 1')
        self.write_lock(raw=raw)
        with self.assertRaisesRegex(policy.PolicyError, "duplicate JSON key"):
            self.load()

    def test_invalid_json_and_constants_rejected(self):
        for raw in (b"{", b"\xff", b'NaN', b'{"schema": Infinity}'):
            self.write_lock(raw=raw)
            with self.subTest(raw=raw), self.assertRaises(policy.PolicyError):
                self.load()

    def test_boolean_schema_and_size_rejected(self):
        for field in ("schema", "bytes"):
            bad = copy.deepcopy(self.data)
            (bad if field == "schema" else bad["artifacts"][0])[field] = True
            self.write_lock(bad)
            with self.subTest(field=field), self.assertRaises(policy.PolicyError):
                self.load()

    def test_nonpositive_huge_and_noninteger_sizes_rejected(self):
        for size in (0, -1, 1.5, "10", policy.MAX_ARTIFACT_BYTES + 1):
            bad = copy.deepcopy(self.data)
            bad["artifacts"][0]["bytes"] = size
            self.write_lock(bad)
            with self.subTest(size=size), self.assertRaises(policy.PolicyError):
                self.load()

    def test_invalid_sha_and_unbounded_locks_rejected(self):
        for sha in ("0" * 63, "A" * 64, "not-a-hash", None):
            bad = copy.deepcopy(self.data)
            bad["artifacts"][0]["sha256"] = sha
            self.write_lock(bad)
            with self.subTest(sha=sha), self.assertRaises(policy.PolicyError):
                self.load()
        self.write_lock(raw=b" " * (policy.MAX_LOCK_BYTES + 1))
        with self.assertRaises(policy.PolicyError):
            self.load()

    def test_empty_duplicate_and_too_many_artifacts_rejected(self):
        for artifacts in ([], self.data["artifacts"] * 2,
                          [self.data["artifacts"][0]] * (policy.MAX_ARTIFACTS + 1)):
            bad = copy.deepcopy(self.data)
            bad["artifacts"] = artifacts
            self.write_lock(bad)
            with self.subTest(count=len(artifacts)), self.assertRaises(policy.PolicyError):
                self.load()

    def test_case_colliding_paths_rejected(self):
        bad = copy.deepcopy(self.data)
        duplicate = bad["artifacts"][0].copy()
        duplicate["path"] = duplicate["path"].replace("Pillow", "pillow")
        duplicate["url"] = duplicate["url"].replace("Pillow", "pillow")
        bad["artifacts"].append(duplicate)
        self.write_lock(bad)
        with self.assertRaises(policy.PolicyError):
            self.load()

    def test_traversal_absolute_nested_and_executable_paths_rejected(self):
        for path in ("../a.whl", "/wheels/a.whl", "wheels/../a.whl", "wheels/sub/a.whl",
                     "wheels\\a.whl", "wheels/.hidden.whl", "wheels/CON.whl",
                     "archives/tool.exe", "bin/tool.zip", "build/result.zip", "wheels/a.whl/", "wheels/a.whl\x00"):
            bad = copy.deepcopy(self.data)
            bad["artifacts"][0]["path"] = path
            self.write_lock(bad)
            with self.subTest(path=path), self.assertRaises(policy.PolicyError):
                self.load()

    def test_url_confusion_and_unofficial_hosts_rejected(self):
        filename = self.data["artifacts"][1]["path"].split("/")[1]
        for url in (
            f"http://download.arduino.cc/{filename}", f"https://download.arduino.cc.evil.test/{filename}",
            f"https://evil.test/{filename}", f"https://user@download.arduino.cc/{filename}",
            f"https://download.arduino.cc:443/{filename}", f"https://download.arduino.cc/a/../{filename}",
            f"https://download.arduino.cc//{filename}", f"https://download.arduino.cc/%2e%2e/{filename}",
            f"https://download.arduino.cc/{filename}?redirect=evil", f"https://download.arduino.cc/{filename}#x",
            f"https://github.com/arduino/other/releases/download/v1/{filename}",
            f"https://github.com/espressif-evil/repo/{filename}",
            f"https://github.com/Arduino/arduino-cli/releases/download/v1/{filename}",
            f"https://download.arduino.cc\\@evil.test/{filename}",
            f"https://download.arduino.cc/\n{filename}", "https://download.arduino.cc/different.zip",
        ):
            bad = copy.deepcopy(self.data)
            bad["artifacts"][1]["url"] = url
            self.write_lock(bad)
            with self.subTest(url=url), self.assertRaises(policy.PolicyError):
                self.load()

    def test_all_official_archive_origins_allowed_by_reviewed_lock(self):
        filename = self.data["artifacts"][1]["path"].split("/")[1]
        for prefix in ("https://download.arduino.cc/", "https://downloads.arduino.cc/",
                       "https://github.com/arduino/arduino-cli/releases/download/test/",
                       "https://github.com/espressif/arduino-esp32/releases/download/test/"):
            bad = copy.deepcopy(self.data)
            bad["artifacts"][1]["url"] = prefix + filename
            self.write_lock(bad)
            with self.subTest(prefix=prefix):
                self.assertTrue(self.evaluate().hit)

    def test_wheels_and_archives_cannot_swap_origins(self):
        for index, url in ((0, "https://download.arduino.cc/"), (1, "https://files.pythonhosted.org/packages/")):
            bad = copy.deepcopy(self.data)
            bad["artifacts"][index]["url"] = url + bad["artifacts"][index]["path"].split("/")[1]
            self.write_lock(bad)
            with self.subTest(index=index), self.assertRaises(policy.PolicyError):
                self.load()

    @unittest.skipUnless(hasattr(os, "symlink"), "symlinks unavailable")
    def test_symlinked_distribution_rejected_even_if_bytes_match(self):
        target = self.cache / next(iter(self.payloads))
        outside = self.base / "outside.whl"
        target.rename(outside)
        target.symlink_to(outside)
        self.assertMiss()

    @unittest.skipUnless(hasattr(os, "symlink"), "symlinks unavailable")
    def test_symlinked_directory_rejected_even_if_all_bytes_match(self):
        outside = self.base / "outside-wheels"
        (self.cache / "wheels").rename(outside)
        (self.cache / "wheels").symlink_to(outside, target_is_directory=True)
        self.assertMiss()

    @unittest.skipUnless(hasattr(os, "symlink"), "symlinks unavailable")
    def test_symlinked_cache_root_rejected(self):
        outside = self.base / "outside-root"
        self.cache.rename(outside)
        self.cache.symlink_to(outside, target_is_directory=True)
        self.assertMiss()

    @unittest.skipUnless(hasattr(os, "symlink"), "symlinks unavailable")
    def test_symlinked_root_ancestor_rejected(self):
        alias = self.base / "alias"
        alias.symlink_to(self.cache.parent, target_is_directory=True)
        self.cache = alias / self.cache.name
        self.assertMiss()

    @unittest.skipUnless(hasattr(os, "symlink"), "symlinks unavailable")
    def test_lock_symlink_and_symlinked_ancestor_rejected(self):
        actual = self.base / "actual-lock.json"
        self.lock_path.rename(actual)
        self.lock_path.symlink_to(actual)
        with self.assertRaises(policy.PolicyError):
            self.load()
        self.lock_path.unlink()
        actual.rename(self.lock_path)
        alias = self.base / "alias"
        alias.symlink_to(self.base, target_is_directory=True)
        self.lock_path = alias / self.lock_path.name
        with self.assertRaises(policy.PolicyError):
            self.load()

    def test_hardlinked_distribution_and_lock_rejected(self):
        target = self.cache / next(iter(self.payloads))
        outside = self.base / "hardlink"
        os.link(target, outside)
        self.assertMiss()
        outside.unlink()
        os.link(self.lock_path, outside)
        with self.assertRaises(policy.PolicyError):
            self.load()

    def test_executable_mode_on_distribution_rejected(self):
        (self.cache / next(iter(self.payloads))).chmod(0o755)
        self.assertMiss()

    @unittest.skipUnless(hasattr(os, "mkfifo"), "FIFO unavailable")
    def test_fifo_is_rejected_without_opening_or_blocking(self):
        target = self.cache / next(iter(self.payloads))
        target.unlink()
        os.mkfifo(target)
        self.assertMiss()

    def test_directory_in_place_of_file_rejected(self):
        target = self.cache / next(iter(self.payloads))
        target.unlink()
        target.mkdir()
        self.assertMiss()

    def test_mutation_during_file_read_rejected(self):
        original = os.read
        fired = False
        def mutate(fd, amount):
            nonlocal fired
            raw = original(fd, amount)
            if not fired:
                fired = True
                target = self.cache / "archives/esp32-3.3.12.zip"
                with target.open("ab") as stream:
                    stream.write(b"poison")
            return raw
        with mock.patch.object(policy.os, "read", side_effect=mutate):
            self.assertMiss()
        self.assertTrue(fired)

    def test_unexpected_file_added_during_verification_rejected(self):
        original = policy._verify_file
        def mutate(directory_fd, name, item):
            result = original(directory_fd, name, item)
            (self.cache / "wheels/injected.whl").write_bytes(b"poison")
            return result
        with mock.patch.object(policy, "_verify_file", side_effect=mutate):
            self.assertMiss()

    def test_cache_root_replaced_during_verification_rejected(self):
        original = policy._verify_file
        fired = False
        def replace_root(directory_fd, name, item):
            nonlocal fired
            result = original(directory_fd, name, item)
            if not fired:
                fired = True
                self.cache.rename(self.base / "detached-cache")
                self.cache.mkdir()
            return result
        with mock.patch.object(policy, "_verify_file", side_effect=replace_root):
            self.assertMiss()
        self.assertTrue(fired)

    def test_lock_total_size_bound_rejected(self):
        bad = copy.deepcopy(self.data)
        first = bad["artifacts"][1]
        bad["artifacts"] = []
        for index in range(5):
            item = first.copy()
            item["path"] = f"archives/large-{index}.zip"
            item["url"] = f"https://download.arduino.cc/large-{index}.zip"
            item["bytes"] = policy.MAX_ARTIFACT_BYTES
            bad["artifacts"].append(item)
        self.write_lock(bad)
        with self.assertRaisesRegex(policy.PolicyError, "total exceeds"):
            self.load()

    def test_only_exact_frozen_upstream_core_dependencies_are_allowed(self):
        for url, (size, digest) in policy.FROZEN_CORE_DEPENDENCY_URLS.items():
            item = {"path": "archives/" + url.rsplit("/", 1)[1], "url": url,
                    "bytes": size, "sha256": digest}
            with self.subTest(url=url):
                self.assertEqual(url, policy._validate_artifact(item).url)
                for bad in (dict(item, bytes=size + 1), dict(item, sha256="0" * 64),
                            dict(item, url=url.replace("/releases/", "/unreviewed/"))):
                    with self.assertRaises(policy.PolicyError):
                        policy._validate_artifact(bad)

    def test_policy_source_has_no_assert_statements(self):
        import ast
        tree = ast.parse(Path(policy.__file__).read_text())
        self.assertFalse(any(isinstance(node, ast.Assert) for node in ast.walk(tree)))

    def test_cli_exit_codes_and_optimized_poison_rejection(self):
        args = [str(Path(policy.__file__)), "verify", "--lock", str(self.lock_path),
                "--expected-lock-sha256", self.lock_sha, "--cache-root", str(self.cache),
                "--context-json", json.dumps(self.context), "--restored-key", self.key()]
        env = dict(os.environ, PYTHONDONTWRITEBYTECODE="1")
        for optimized in ([], ["-O"]):
            with self.subTest(optimized=optimized):
                result = subprocess.run([sys.executable, *optimized, *args], capture_output=True, text=True, env=env)
                self.assertEqual(0, result.returncode, result.stderr)
                self.assertTrue(json.loads(result.stdout)["hit"])
        target = self.cache / next(iter(self.payloads))
        target.write_bytes(b"X" * target.stat().st_size)
        for optimized in ([], ["-O"]):
            result = subprocess.run([sys.executable, *optimized, *args], capture_output=True, text=True, env=env)
            self.assertEqual(3, result.returncode, result.stderr)
            self.assertEqual("clean_download", json.loads(result.stdout)["action"])
        args[args.index("--expected-lock-sha256") + 1] = "0" * 64
        result = subprocess.run([sys.executable, "-O", *args], capture_output=True, text=True, env=env)
        self.assertEqual(2, result.returncode)
        self.assertEqual("stop", json.loads(result.stderr)["action"])


if __name__ == "__main__":
    unittest.main()
