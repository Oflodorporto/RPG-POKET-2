"""Real-metadata lock checks plus synthetic byte tests; never install or download."""
import argparse
from dataclasses import replace
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import stat
import sys
import tarfile
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "dependencies"))
import generate_lock as generator
import install_dependencies as installer
import cache_policy as policy

EXPECTED_LOCK = "d03e64207ca2c45c81f01ecf64ff8faf230f6552e12c7d96d552d68491c2fa78"


class OfficialLockTests(unittest.TestCase):
    def test_committed_lock_reproduces_offline(self):
        with mock.patch("urllib.request.urlopen", side_effect=AssertionError("No network")):
            products = generator.verify_generated()
        self.assertEqual(EXPECTED_LOCK, generator.sha(products["downloads.lock.json"]))
        lock = json.loads(products["downloads.lock.json"])
        self.assertEqual(27, len(lock["artifacts"]))
        self.assertEqual(1847821339, sum(a["bytes"] for a in lock["artifacts"]))

    def test_real_lock_accepted_by_cache_policy(self):
        lock = installer.load_lock(EXPECTED_LOCK, "/tmp/nonexistent-lock-test")
        self.assertEqual(27, len(lock.artifacts))
        self.assertEqual(generator.CONTEXT, dict(lock.context))

    def test_exact_context_pins(self):
        self.assertEqual("3.12.3", generator.CONTEXT["python_version"])
        self.assertEqual("Linux", generator.CONTEXT["runner_os"])
        self.assertEqual("X64", generator.CONTEXT["runner_arch"])
        self.assertEqual("1.5.2-rc.1", generator.CONTEXT["arduino_cli_version"])
        self.assertEqual("3.3.12", generator.CONTEXT["esp32_core_version"])
        self.assertEqual(generator.CLI_COMMIT, json.loads((generator.HERE / "PIN.json").read_text())["arduino_cli_commit"])

    def test_all_tool_dependencies_are_closed(self):
        index = json.loads((generator.HERE / "frozen/package_index.json").read_text())
        packages = {p["name"]: p for p in index["packages"]}
        self.assertEqual({"esp32", "arduino", "builtin"}, set(packages))
        core = packages["esp32"]["platforms"][0]
        self.assertEqual(18, len(core["toolsDependencies"]))
        for dep in core["toolsDependencies"]:
            found = [t for t in packages[dep["packager"]]["tools"]
                     if (t["name"], t["version"]) == (dep["name"], dep["version"])]
            self.assertEqual(1, len(found))
            self.assertEqual(1, len(found[0]["systems"]))
        self.assertEqual(generator.BUILTINS, {t["name"]: t["version"] for t in packages["builtin"]["tools"]})
        self.assertEqual({"libraries": []}, json.loads((generator.HERE / "frozen/library_index.json").read_text()))

    def test_every_frozen_index_download_is_in_lock(self):
        index = json.loads((generator.HERE / "frozen/package_index.json").read_text())
        locked = {a["path"]: a for a in json.loads(installer.LOCK.read_text())["artifacts"]}
        records = []
        for package in index["packages"]:
            records.extend(package["platforms"])
            records.extend(s for t in package["tools"] for s in t["systems"])
        self.assertEqual(24, len(records))
        for record in records:
            item = generator.archive(record)
            self.assertEqual(item, locked[item["path"]])

    def test_wheels_are_cpython312_linux_x64_without_unreviewed_extras(self):
        artifacts = json.loads(installer.LOCK.read_text())["artifacts"]
        wheels = [a for a in artifacts if a["path"].startswith("wheels/")]
        self.assertEqual(2, len(wheels))
        for wheel in wheels:
            self.assertIn("-cp312-cp312-manylinux", wheel["path"])
            self.assertIn("x86_64.whl", wheel["path"])
            self.assertTrue(wheel["url"].startswith("https://files.pythonhosted.org/packages/"))
            self.assertIn(wheel["sha256"], (generator.HERE / "requirements.lock.txt").read_text())

    def test_source_tamper_rejected_before_generation(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            shutil.copytree(generator.HERE / "sources", root / "sources")
            shutil.copyfile(generator.HERE / "sources.lock.json", root / "sources.lock.json")
            with (root / "sources/package_index.json").open("ab") as out:
                out.write(b" ")
            with self.assertRaisesRegex(policy.PolicyError, "metadata changed"):
                generator.generate(root)

    def test_wrong_expected_lock_digest_rejected(self):
        with self.assertRaisesRegex(policy.PolicyError, "SHA-256 mismatch"):
            installer.load_lock("0" * 64, "/tmp/lock-mismatch-test")

    def test_key_is_exact_and_run_scoped(self):
        lock = installer.load_lock(EXPECTED_LOCK, "/tmp/lock-key-test")
        key = installer.experiment_key(lock, "123", "2")
        self.assertEqual(policy.exact_cache_key(lock) + "-run-123-attempt-2", key)
        self.assertNotEqual(key, installer.experiment_key(lock, "124", "2"))
        for value in (None, "", "0", "01", "latest", "123\n"):
            with self.assertRaises(policy.PolicyError):
                installer.experiment_key(lock, value, "2")


class SyntheticIOTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.raw = b"SYNTHETIC DISTRIBUTION BYTES, DO NOT EXECUTE\n"
        self.item = policy.Artifact("wheels/test-1.0.0-py3-none-any.whl",
                                  "https://files.pythonhosted.org/packages/test/test-1.0.0-py3-none-any.whl",
                                  len(self.raw), hashlib.sha256(self.raw).hexdigest())

    def response(self, raw=None, url=None, headers=None, status=200):
        class Response(io.BytesIO):
            def geturl(response):
                return response.url
        response = Response(self.raw if raw is None else raw)
        response.url = url or self.item.url
        response.headers = headers or {}
        response.status = status
        return response

    def test_streamed_download_commits_only_verified_bytes(self):
        target = self.root / "download.whl"
        installer.download_one(self.item, target, opener=lambda *_a, **_k: self.response())
        self.assertEqual(self.raw, target.read_bytes())
        self.assertFalse(target.stat().st_mode & 0o111)
        self.assertFalse(target.with_name(target.name + ".partial").exists())

    def test_bad_download_bytes_never_commit(self):
        for raw in (b"wrong", self.raw[:-1], self.raw + b"extra", b"x" * len(self.raw)):
            with self.subTest(raw=raw):
                target = self.root / "bad.whl"
                with self.assertRaises(policy.PolicyError):
                    installer.download_one(self.item, target, opener=lambda *_a, **_k: self.response(raw))
                self.assertFalse(target.exists())
                self.assertFalse(target.with_name(target.name + ".partial").exists())

    def test_wrong_headers_status_and_redirect_are_rejected(self):
        responses = [dict(headers={"Content-Encoding": "gzip"}),
                     dict(headers={"Content-Length": "999999"}), dict(status=206),
                     dict(url="http://files.pythonhosted.org/packages/test"),
                     dict(url="https://evil.example/download.whl")]
        for options in responses:
            with self.subTest(options=options), self.assertRaises(policy.PolicyError):
                installer.download_one(self.item, self.root / "bad.whl",
                                       opener=lambda *_a, **_k: self.response(**options))

    def test_redirect_allowlist_is_narrow(self):
        installer.redirect_allowed("https://github.com/espressif/a", "https://release-assets.githubusercontent.com/a?sig=abc")
        for target in ("http://release-assets.githubusercontent.com/a", "https://evil.example/a",
                       "https://user:password@release-assets.githubusercontent.com/a",
                       "https://release-assets.githubusercontent.com:8443/a"):
            with self.subTest(target=target), self.assertRaises(policy.PolicyError):
                installer.redirect_allowed("https://github.com/espressif/a", target)
        with self.assertRaises(policy.PolicyError):
            installer.redirect_allowed(self.item.url, "https://release-assets.githubusercontent.com/a")

    def test_existing_download_target_is_not_overwritten(self):
        target = self.root / "exists.whl"
        target.write_bytes(b"already here")
        with self.assertRaises(policy.PolicyError):
            installer.download_one(self.item, target, opener=mock.Mock())
        self.assertEqual(b"already here", target.read_bytes())

    def test_copy_rehashes_and_refuses_unsafe_file_types(self):
        source = self.root / "source.whl"
        source.write_bytes(self.raw)
        target = self.root / "copy.whl"
        installer.copy_verified(self.item, source, target)
        self.assertEqual(self.raw, target.read_bytes())
        source.write_bytes(b"x" * len(self.raw))
        with self.assertRaises(policy.PolicyError):
            installer.copy_verified(self.item, source, self.root / "badcopy.whl")
        source.write_bytes(self.raw)
        source.chmod(0o700)
        with self.assertRaises(policy.PolicyError):
            installer.copy_verified(self.item, source, self.root / "exec.whl")
        source.chmod(0o600)
        link = self.root / "link.whl"
        link.symlink_to(source)
        with self.assertRaises((OSError, policy.PolicyError)):
            installer.copy_verified(self.item, link, self.root / "symlink.whl")
        os.link(source, self.root / "hardlink.whl")
        with self.assertRaises(policy.PolicyError):
            installer.copy_verified(self.item, source, self.root / "hardcopy.whl")

    def archive(self, members):
        path = self.root / "synthetic.tar.gz"
        with tarfile.open(path, "w:gz") as out:
            for name, kind, raw in members:
                info = tarfile.TarInfo(name)
                info.type = kind
                info.size = len(raw) if kind == tarfile.REGTYPE else 0
                info.linkname = "../../escape" if kind in (tarfile.SYMTYPE, tarfile.LNKTYPE) else ""
                out.addfile(info, io.BytesIO(raw) if info.size else None)
        return path

    def test_cli_extractor_extracts_only_named_binary(self):
        archive = self.archive([("arduino-cli", tarfile.REGTYPE, self.raw),
                                ("LICENSE.txt", tarfile.REGTYPE, b"license fixture")])
        binary = installer.extract_cli(archive, self.root / "bin")
        self.assertEqual(self.raw, binary.read_bytes())
        self.assertEqual({"arduino-cli"}, {p.name for p in binary.parent.iterdir()})
        self.assertTrue(binary.stat().st_mode & stat.S_IXUSR)

    def test_cli_extractor_rejects_links_traversal_and_duplicate_binary(self):
        cases = [[("arduino-cli", tarfile.SYMTYPE, b"")],
                 [("arduino-cli", tarfile.REGTYPE, self.raw), ("../escape", tarfile.REGTYPE, b"bad")],
                 [("arduino-cli", tarfile.REGTYPE, self.raw), ("arduino-cli", tarfile.REGTYPE, self.raw)],
                 [("different", tarfile.REGTYPE, self.raw)]]
        for i, members in enumerate(cases):
            with self.subTest(members=members), self.assertRaises(policy.PolicyError):
                installer.extract_cli(self.archive(members), self.root / ("bin" + str(i)))

    def test_safe_path_rejects_symlink_ancestor_and_newline(self):
        real = self.root / "real"
        real.mkdir()
        link = self.root / "link"
        link.symlink_to(real)
        for path in (link / "child", self.root / "name\ncommand"):
            with self.assertRaises(policy.PolicyError):
                installer.safe_path(path)


class WarmFailureTests(unittest.TestCase):
    def test_warm_missing_corrupt_and_unsuffixed_key_never_install(self):
        real_lock = installer.load_lock(EXPECTED_LOCK, "/tmp/warm-fixture")
        full_key = installer.experiment_key(real_lock, "123", "1")
        for restored in ("", policy.exact_cache_key(real_lock), full_key + "-wrong", full_key):
            with self.subTest(restored=restored), tempfile.TemporaryDirectory() as temp:
                base = Path(temp)
                args = argparse.Namespace(cache_root=str(base / "cache"), workspace=str(base / "work"),
                    report=str(base / "report.json"), expected_lock_sha256=EXPECTED_LOCK,
                    run_id="123", run_attempt="1", cache_key=full_key, phase="warm",
                    restored_key=restored, github_env=None, github_path=None)
                miss = policy.CacheDecision(False, "clean_download", full_key, "synthetic miss")
                with mock.patch.object(installer, "check_runtime"), \
                     mock.patch.object(installer, "load_lock", return_value=real_lock), \
                     mock.patch.object(policy, "evaluate_cache", return_value=miss) as evaluate, \
                     mock.patch.object(installer, "install_fresh") as install, \
                     mock.patch.object(installer, "download_all") as download:
                    with self.assertRaises(installer.WarmMiss):
                        installer.prepare(args)
                    install.assert_not_called()
                    download.assert_not_called()
                    forwarded = evaluate.call_args.args[-1]
                    self.assertEqual(policy.exact_cache_key(real_lock) if restored == full_key else None,
                                     forwarded)
                report = json.loads((base / "report.json").read_text())
                self.assertFalse(report["cache_hit"])
                self.assertEqual("failed", report["status"])
                self.assertEqual("clean_download", report["action"])


if __name__ == "__main__":
    unittest.main()
