#!/usr/bin/env python3
"""Install the reviewed benchmark dependencies, using verified distributions only.

Never run this helper against a restored installation. Cache roots contain only
the 27 archives/wheels. Every installation and its Arduino indexes are fresh.
"""
from __future__ import annotations

import argparse
from dataclasses import asdict
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import stat
import subprocess
import sys
import tarfile
import time
from urllib.error import HTTPError, URLError
from urllib.parse import urlsplit
from urllib.request import HTTPRedirectHandler, Request, build_opener

import generate_lock as generator
from generate_lock import CONTEXT, HERE, CLI_COMMIT, require, encode, sha
import cache_policy as policy

LOCK = HERE / "downloads.lock.json"
CHUNK = 1024 * 1024


class WarmMiss(policy.CacheRejected):
    pass


def check_runtime():
    require(sys.flags.optimize == 0, "Optimized Python is not allowed")
    require(platform.system() == "Linux" and platform.machine() == "x86_64",
            "Only Linux x86-64 is supported")
    require(platform.python_version() == CONTEXT["python_version"],
            "Exact benchmark Python 3.12.3 is required; use a new reviewed baseline otherwise")
    release = platform.freedesktop_os_release()
    require(release.get("ID") == "ubuntu" and release.get("VERSION_ID") == "24.04",
            "Only Ubuntu 24.04 is supported")
    for field, expected in (("RUNNER_OS", "Linux"), ("RUNNER_ARCH", "X64")):
        require(os.environ.get(field, expected) == expected, "Runner context mismatch: " + field)


def load_lock(expected, root):
    generator.verify_generated()
    return policy.load_trusted_lock(LOCK, expected, root, CONTEXT)


def experiment_key(lock, run_id, attempt):
    for value in (run_id, attempt):
        require(type(value) is str and re.fullmatch(r"[1-9][0-9]*", value),
                "Exact positive run ID and attempt are required")
    return policy.exact_cache_key(lock) + "-run-" + run_id + "-attempt-" + attempt


def verify_tree(root, expected):
    lock = load_lock(expected, root)
    decision = policy.evaluate_cache(root, LOCK, expected, CONTEXT, policy.exact_cache_key(lock))
    require(decision.hit, "Distribution tree failed verification: " + decision.reason)
    return lock


def safe_path(raw):
    path = policy._absolute(raw)
    require(all("\n" not in part and "\r" not in part for part in path.parts), "Unsafe output path")
    # Existing ancestors must not be symlinks, including dangling symlinks.
    for parent in reversed((path, *path.parents)):
        if parent.is_symlink():
            raise policy.PolicyError("Symlink in workspace/cache path")
    return path


def redirect_allowed(original, destination):
    old, new = urlsplit(original), urlsplit(destination)
    require(new.scheme == "https" and new.port in (None, 443) and
            not new.username and not new.password and not new.fragment,
            "Unsafe download redirect")
    require(new.hostname == old.hostname or
            old.hostname == "github.com" and new.hostname == "release-assets.githubusercontent.com",
            "Download redirected outside the reviewed origin/CDN")


class ReviewedRedirects(HTTPRedirectHandler):
    def __init__(self, original):
        self.original = original

    def redirect_request(self, req, fp, code, msg, headers, newurl):
        redirect_allowed(self.original, newurl)
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def download_one(item, target, *, opener=None):
    """One bounded streaming download; only exact size and SHA-256 are committed."""
    policy._validate_artifact(asdict(item))
    require(not target.exists() and not target.is_symlink(), "Download target must be new")
    partial = target.with_name(target.name + ".partial")
    require(not partial.exists() and not partial.is_symlink(), "Partial target must be new")
    open_url = opener or build_opener(ReviewedRedirects(item.url)).open
    request = Request(item.url, headers={"Accept-Encoding": "identity", "User-Agent": "RPG-POKET-reviewed-dependency-lock/1"})
    try:
        with open_url(request, timeout=120) as response, partial.open("xb") as out:
            redirect_allowed(item.url, response.geturl())
            require(response.status == 200, "Unexpected HTTP status")
            require(response.headers.get("Content-Encoding", "identity") == "identity",
                    "Unexpected HTTP content encoding")
            declared = response.headers.get("Content-Length")
            if declared is not None:
                require(declared.isdecimal() and int(declared) == item.bytes, "HTTP length differs from lock")
            digest, total = hashlib.sha256(), 0
            while True:
                block = response.read(min(CHUNK, item.bytes - total + 1))
                if not block:
                    break
                total += len(block)
                require(total <= item.bytes, "Download exceeds locked size")
                digest.update(block)
                out.write(block)
            require(total == item.bytes and digest.hexdigest() == item.sha256,
                    "Download size/SHA-256 mismatch")
            out.flush()
            os.fsync(out.fileno())
        partial.chmod(0o600)
        partial.rename(target)
    finally:
        if partial.exists():
            partial.unlink()


def download_all(lock, destination):
    destination.mkdir(mode=0o700)
    for item in lock.artifacts:
        target = destination / item.path
        target.parent.mkdir(mode=0o700, exist_ok=True)
        for attempt in range(3):
            try:
                download_one(item, target)
                break
            except (URLError, TimeoutError) as exc:
                if isinstance(exc, HTTPError) and exc.code not in {408, 429, 500, 502, 503, 504}:
                    raise
                if attempt == 2:
                    raise
                time.sleep(2 ** attempt)


def copy_verified(item, source, target):
    """Copy through O_NOFOLLOW and hash again; cache bytes never become executable."""
    fd = policy._open_nofollow(source)
    try:
        before = os.fstat(fd)
        require(stat.S_ISREG(before.st_mode) and before.st_nlink == 1 and
                not before.st_mode & 0o111 and before.st_size == item.bytes,
                "Unsafe distribution during staging")
        digest, total = hashlib.sha256(), 0
        with os.fdopen(fd, "rb", closefd=False) as src, target.open("xb") as dst:
            while True:
                block = src.read(min(CHUNK, item.bytes - total + 1))
                if not block:
                    break
                total += len(block)
                require(total <= item.bytes, "Staged distribution exceeds locked size")
                dst.write(block)
                digest.update(block)
        require(total == item.bytes and digest.hexdigest() == item.sha256 and
                policy._signature(before) == policy._signature(os.fstat(fd)),
                "Distribution changed while staging")
        target.chmod(0o600)
    finally:
        os.close(fd)


def extract_cli(archive, bin_dir):
    """Extract only the regular CLI binary, never tar paths or links."""
    bin_dir.mkdir(mode=0o700)
    target = bin_dir / "arduino-cli"
    with tarfile.open(archive, "r:gz") as package:
        members = package.getmembers()
        require(len(members) <= 32, "Unexpected CLI archive member count")
        binary = [member for member in members if member.name == "arduino-cli"]
        require(len(binary) == 1 and binary[0].isfile() and
                0 < binary[0].size <= 200 * 1024 * 1024, "Invalid CLI archive binary")
        for member in members:
            require(not member.name.startswith("/") and ".." not in Path(member.name).parts and
                    not member.issym() and not member.islnk(), "Unsafe CLI archive entry")
        with package.extractfile(binary[0]) as src, target.open("xb") as dst:
            shutil.copyfileobj(src, dst, CHUNK)
        require(target.stat().st_size == binary[0].size, "Truncated CLI binary")
    target.chmod(0o700)
    return target


def run(command, env, log, *, capture=False):
    with log.open("ab") as out:
        out.write(("$ " + " ".join(map(str, command)) + "\n").encode())
        if capture:
            result = subprocess.run(list(map(str, command)), check=True, env=env,
                                    stdout=subprocess.PIPE, stderr=out, timeout=1800)
            out.write(result.stdout)
            return policy._strict_json(result.stdout)
        subprocess.run(list(map(str, command)), check=True, env=env,
                       stdout=out, stderr=subprocess.STDOUT, timeout=1800)


def install_fresh(lock, cache_root, work):
    begin = time.perf_counter_ns()
    stage = work / "staging"
    archives, wheels = stage / "packages", stage / "wheels"
    archives.mkdir(parents=True, mode=0o700)
    wheels.mkdir(mode=0o700)
    for item in lock.artifacts:
        target = (wheels if item.path.startswith("wheels/") else archives) / Path(item.path).name
        copy_verified(item, cache_root / item.path, target)
    copy_ns = time.perf_counter_ns() - begin
    cli_archive = archives / "arduino-cli_1.5.2-rc.1_Linux_64bit.tar.gz"
    cli = extract_cli(cli_archive, work / "bin")
    # The CLI archive itself is not an Arduino package download.
    cli_archive.unlink()
    data, user, home = work / "arduino-data", work / "arduino-user", work / "home"
    for path in (data, user, home):
        path.mkdir(mode=0o700)
    for name in ("package_index.json", "library_index.json"):
        (data / name).write_bytes((HERE / "frozen" / name).read_bytes())
    config = work / "arduino-cli.yaml"
    # JSON is valid YAML. No additional URL, update-index, or mutable library feed.
    config.write_bytes(encode({"board_manager": {"additional_urls": []},
                              "directories": {"data": str(data), "downloads": str(stage), "user": str(user)},
                              "updater": {"enable_notification": False},
                              "network": {"proxy": "http://127.0.0.1:9", "connection_timeout": "2s"}}))
    env = {k: v for k, v in os.environ.items() if not k.startswith(("ARDUINO_", "PYTHON", "PIP_"))}
    env.update({"HOME": str(home), "ARDUINO_CONFIG_FILE": str(config),
                "ARDUINO_UPDATER_ENABLE_NOTIFICATION": "false", "PYTHONDONTWRITEBYTECODE": "1",
                "HTTP_PROXY": "http://127.0.0.1:9", "HTTPS_PROXY": "http://127.0.0.1:9",
                "ALL_PROXY": "http://127.0.0.1:9", "NO_PROXY": ""})
    venv, log = work / "venv", work / "install.log"
    pip_begin = time.perf_counter_ns()
    run([sys.executable, "-m", "venv", "--without-pip", venv], env, log)
    python = venv / "bin" / "python3"
    # Use the runner's existing pip only as installer. No pip/setuptools download,
    # and no bundled bootstrap wheel becomes part of the benchmark cache.
    run([sys.executable, "-m", "pip", "--isolated", "--python", python, "install",
         "--no-index", "--no-deps", "--require-hashes", "--only-binary=:all:",
         "--no-compile", "--find-links", wheels, "-r", HERE / "requirements.lock.txt"], env, log)
    versions = run([python, "-I", "-c", "import json,platform,PIL,yaml; print(json.dumps({'python':platform.python_version(),'pillow':PIL.__version__,'pyyaml':yaml.__version__}))"], env, log, capture=True)
    require(versions == {"python": CONTEXT["python_version"], "pillow": CONTEXT["pillow_version"],
                         "pyyaml": CONTEXT["pyyaml_version"]}, "Installed Python package identity changed")
    pip_ns = time.perf_counter_ns() - pip_begin
    arduino_begin = time.perf_counter_ns()
    cli_args = [cli, "--config-file", config]
    cli_version = run([*cli_args, "version", "--format", "json"], env, log, capture=True)
    require(cli_version.get("VersionString") == CONTEXT["arduino_cli_version"] and
            cli_version.get("Commit") in {CLI_COMMIT, CLI_COMMIT[:9]}, "Installed CLI identity changed")
    run([*cli_args, "core", "install", "esp32:esp32@3.3.12"], env, log)
    cores = run([*cli_args, "core", "list", "--format", "json"], env, log, capture=True)
    installed = cores.get("platforms", [])
    require(len(installed) == 1 and installed[0].get("id") == "esp32:esp32" and
            installed[0].get("installed_version") == "3.3.12", "Installed core identity changed")
    closure = policy._strict_json((HERE / "closure.json").read_bytes())
    for tool in closure["tools"]:
        require((data / "packages" / tool["packager"] / "tools" / tool["name"] / tool["version"]).is_dir(),
                "Missing installed locked tool: " + tool["name"])
    for name in ("package_index.json", "library_index.json"):
        require((data / name).read_bytes() == (HERE / "frozen" / name).read_bytes(),
                "Arduino changed a frozen index")
    expected_archives = {Path(item.path).name: item for item in lock.artifacts
                         if item.path.startswith("archives/") and "arduino-cli_" not in item.path}
    require({p.name for p in archives.iterdir()} == set(expected_archives),
            "Arduino staging contains unreviewed downloads")
    # A post-install check catches unexpected changes before saving the cache.
    for name, item in expected_archives.items():
        path = archives / name
        require(path.stat().st_size == item.bytes, "Arduino staging archive changed")
    arduino_ns = time.perf_counter_ns() - arduino_begin
    return {"python": str(python), "cli": str(cli), "config": str(config),
            "tools_dir": str(data / "packages" / "esp32" / "tools"),
            "venv": str(venv), "dependency_copy_ns": copy_ns, "pip_install_ns": pip_ns,
            "arduino_install_ns": arduino_ns,
            "tool_identity": {**versions, "arduino_cli_version": cli_version["VersionString"],
                              "arduino_cli_commit": CLI_COMMIT, "esp32_core_version": "3.3.12"}}


def append_environment(report, env_file, path_file):
    if env_file:
        with open(env_file, "a", encoding="utf-8") as out:
            out.write("ARDUINO_CONFIG_FILE=" + report["config"] + "\n")
            out.write("ARDUINO_UPDATER_ENABLE_NOTIFICATION=false\n")
            out.write("VIRTUAL_ENV=" + report["venv"] + "\n")
    if path_file:
        with open(path_file, "a", encoding="utf-8") as out:
            out.write(str(Path(report["python"]).parent) + "\n")
            out.write(str(Path(report["cli"]).parent) + "\n")


def prepare(args):
    started = time.perf_counter_ns()
    check_runtime()
    root, work, report_path = map(safe_path, (args.cache_root, args.workspace, args.report))
    require(root != work and root not in work.parents and work not in root.parents,
            "Cache/workspace roots must be separate")
    require(root not in report_path.parents and root != report_path, "Report must be outside cache")
    require(not work.exists(), "Installation workspace must be new")
    lock = load_lock(args.expected_lock_sha256, root)
    run_id = args.run_id or os.environ.get("GITHUB_RUN_ID")
    attempt = args.run_attempt or os.environ.get("GITHUB_RUN_ATTEMPT")
    key = experiment_key(lock, run_id, attempt)
    require(not args.cache_key or args.cache_key == key, "Wrong requested experiment key")
    work.mkdir(parents=True, mode=0o700)
    report = {"schema": 1, "phase": args.phase, "context": CONTEXT,
              "lock_sha256": lock.sha256, "cache_key": key, "cache_hit": False,
              "benchmark_salt": CONTEXT["benchmark_salt"],
              "cache_bytes": sum(a.bytes for a in lock.artifacts),
              "dependency_install_ns": 0, "dependency_verify_ns": time.perf_counter_ns() - started}
    try:
        if args.phase == "cold":
            require(not root.exists() and not root.is_symlink(), "Cold cache-root must not exist")
            require(not args.restored_key, "Cold must not restore a cache")
            begin = time.perf_counter_ns()
            clean = work / "downloads-clean"
            download_all(lock, clean)
            report["download_ns"] = time.perf_counter_ns() - begin
            begin = time.perf_counter_ns()
            verify_tree(clean, lock.sha256)
            report["dependency_verify_ns"] += time.perf_counter_ns() - begin
            root.parent.mkdir(parents=True, exist_ok=True)
            clean.rename(root)
        else:
            begin = time.perf_counter_ns()
            decision = policy.evaluate_cache(root, LOCK, lock.sha256, CONTEXT,
                        policy.exact_cache_key(lock) if args.restored_key == key else None)
            report["dependency_verify_ns"] += time.perf_counter_ns() - begin
            report["cache_hit"] = decision.hit
            if not decision.hit:
                report.update({"action": "clean_download", "reason": decision.reason})
                raise WarmMiss("Warm cache failed verification. Start a new cold/warm experiment; restored data was not installed.")
            report["download_ns"] = 0
        report.update(install_fresh(lock, root, work))
        begin = time.perf_counter_ns()
        verify_tree(root, lock.sha256)
        report["dependency_verify_ns"] += time.perf_counter_ns() - begin
        report["dependency_verify_ns"] += report["dependency_copy_ns"]
        report["total_ns"] = time.perf_counter_ns() - started
        report["dependency_install_ns"] = report["total_ns"] - report["dependency_verify_ns"]
        report["cache"] = {"key": key, "hit": report["cache_hit"], "restored": args.phase == "warm",
                           "lock_sha256": lock.sha256, "files": len(lock.artifacts), "bytes": report["cache_bytes"]}
        report["paths"] = {"python": report["python"], "arduino_cli": report["cli"],
                           "arduino_config": report["config"], "downloads": str(root)}
        report["status"] = "verified_and_installed"
        append_environment(report, args.github_env, args.github_path)
        return report
    except Exception as exc:
        report.update({"status": "failed", "error": str(exc)})
        raise
    finally:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_bytes(encode(report))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("key", "verify", "prepare", "install"):
        cmd = sub.add_parser(name)
        cmd.add_argument("--expected-lock-sha256", required=True)
        cmd.add_argument("--cache-root", default="/tmp/rpg-benchmark-key-only")
        if name != "verify":
            cmd.add_argument("--run-id")
            cmd.add_argument("--run-attempt")
        if name in ("prepare", "install"):
            cmd.add_argument("--phase", choices=("cold", "warm"), required=True)
            cmd.add_argument("--workspace", required=True)
            cmd.add_argument("--report", required=True)
            cmd.add_argument("--restored-key", default="")
            cmd.add_argument("--cache-key")
            cmd.add_argument("--github-env")
            cmd.add_argument("--github-path")
    args = parser.parse_args()
    try:
        if args.command == "key":
            lock = load_lock(args.expected_lock_sha256, args.cache_root)
            print(experiment_key(lock, args.run_id or os.environ.get("GITHUB_RUN_ID"),
                                 args.run_attempt or os.environ.get("GITHUB_RUN_ATTEMPT")))
        elif args.command == "verify":
            lock = verify_tree(args.cache_root, args.expected_lock_sha256)
            print(json.dumps({"status": "verified", "files": len(lock.artifacts), "lock_sha256": lock.sha256}))
        else:
            print(json.dumps(prepare(args)))
        return 0
    except WarmMiss as exc:
        print(json.dumps({"action": "clean_download", "error": str(exc)}), file=sys.stderr)
        return 3
    except (OSError, ValueError, KeyError, subprocess.SubprocessError) as exc:
        print(json.dumps({"action": "stop", "error": str(exc)}), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
