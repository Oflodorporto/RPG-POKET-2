#!/usr/bin/env python3
"""Offline-only allowlist validator. Never downloads, installs, extracts or saves.

A lock and its expected SHA-256 must come from reviewed, independent inputs.
No cache content is evidence of its own trustworthiness. Callers must consume a
hit only in an isolated workspace without concurrent writers; see CACHE.md.
"""
from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import sys
from urllib.parse import urlsplit

SCHEMA = 1
MAX_LOCK_BYTES = 2 * 1024 * 1024
MAX_ARTIFACTS = 256
MAX_ARTIFACT_BYTES = 8 * 1024**3
MAX_TOTAL_BYTES = 32 * 1024**3
CONTEXT_FIELDS = frozenset({
    "runner_os", "runner_arch", "python_version", "arduino_cli_version",
    "esp32_core_version", "pillow_version", "pyyaml_version", "fqbn",
    "benchmark_salt",
})
VERSION_FIELDS = CONTEXT_FIELDS - {
    "runner_os", "runner_arch", "fqbn", "benchmark_salt",
}
ARCHIVE_SUFFIXES = (".tar.gz", ".tar.xz", ".tar.bz2", ".tgz", ".zip")
FROZEN_CORE_DEPENDENCY_URLS = {
    # Exact upstream distributions already declared by official ESP32 3.3.12.
    "https://github.com/igrr/mkspiffs/releases/download/0.2.3/mkspiffs-0.2.3-arduino-esp32-linux64.tar.gz":
        (50646, "5e1a4ff41385e842f389f6b5254102a547e566a06b49babeffa93ef37115cb5d"),
    "https://github.com/earlephilhower/mklittlefs/releases/download/4.0.2/x86_64-linux-gnu-mklittlefs-db0513a.tar.gz":
        (54000, "7a70428b7089bf1c9d481b0e070e99cd8a430d37e197b7d3db64f24ba8891508"),
}


class PolicyError(ValueError):
    """Invalid trusted input: stop, rather than repair it from cache contents."""


class CacheRejected(ValueError):
    """Untrusted cache must be treated as a miss, then downloaded cleanly."""


def _require(condition: bool, message: str, error=PolicyError) -> None:
    # Deliberately not assert: policy must remain effective under python -O.
    if not condition:
        raise error(message)


def _exact_keys(value, expected, label):
    _require(type(value) is dict and set(value) == set(expected),
             f"{label}: exact fields required")


def _sha(value, label):
    _require(type(value) is str and re.fullmatch(r"[0-9a-f]{64}", value),
             f"{label}: lowercase SHA-256 required")


def _strict_json(raw):
    def pairs(items):
        result = {}
        for key, value in items:
            _require(key not in result, "duplicate JSON key")
            result[key] = value
        return result

    def reject_constant(value):
        raise PolicyError(f"non-JSON numeric constant: {value}")

    try:
        return json.loads(raw, object_pairs_hook=pairs,
                          parse_constant=reject_constant)
    except (ValueError, UnicodeError, RecursionError) as exc:
        raise PolicyError(f"invalid JSON: {exc}") from exc


def validate_context(context):
    """Require every dimension, including exact observed/pinned versions."""
    _exact_keys(context, CONTEXT_FIELDS, "context")
    for field, value in context.items():
        _require(type(value) is str and 0 < len(value) <= 512,
                 f"{field}: nonempty bounded string required")
        if field in VERSION_FIELDS:
            _require(re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+(?:[-+][A-Za-z0-9.-]+)?", value),
                     f"{field}: exact three-component version required")
        elif field == "fqbn":
            _require(re.fullmatch(r"[A-Za-z0-9_-]+:[A-Za-z0-9_-]+:[A-Za-z0-9_-]+(?::[A-Za-z0-9_=,.-]+)?", value),
                     "fqbn: exact board and options required")
        else:
            _require(re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]{0,63}", value),
                     f"{field}: exact identifier required")
            _require(value.lower() not in {"latest", "any", "unknown", "default"},
                     f"{field}: moving or unknown identifier forbidden")
    _require(context["benchmark_salt"].startswith("benchmark-"),
             "benchmark_salt must start with benchmark-")
    return tuple(sorted(context.items()))


@dataclass(frozen=True)
class Artifact:
    path: str
    url: str
    bytes: int
    sha256: str


@dataclass(frozen=True)
class TrustedLock:
    sha256: str
    context: tuple[tuple[str, str], ...]
    artifacts: tuple[Artifact, ...]


@dataclass(frozen=True)
class CacheDecision:
    hit: bool
    action: str
    key: str
    reason: str
    verified_files: int = 0


def _validate_artifact(item):
    _exact_keys(item, {"path", "url", "bytes", "sha256"}, "artifact")
    path = item["path"]
    _require(type(path) is str and len(path) <= 255, "invalid artifact path")
    parts = path.split("/")
    _require(len(parts) == 2 and parts[0] in {"wheels", "archives"},
             "only wheels/<file> and archives/<file> allowed")
    category, name = parts
    _require(re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._+-]*", name),
             "noncanonical artifact filename")
    _require(name.split(".")[0].upper() not in {
        "CON", "PRN", "AUX", "NUL", *[f"COM{i}" for i in range(1, 10)],
        *[f"LPT{i}" for i in range(1, 10)],
    }, "reserved filename")
    _require(name.endswith(".whl") if category == "wheels" else
             name.endswith(ARCHIVE_SUFFIXES), "only distribution wheels/archives allowed")
    _require(type(item["bytes"]) is int and 0 < item["bytes"] <= MAX_ARTIFACT_BYTES,
             "artifact bytes must be a positive bounded integer")
    _sha(item["sha256"], "artifact sha256")
    url = item["url"]
    _require(type(url) is str and 0 < len(url) <= 2048,
             "artifact URL required")
    _require(all(32 < ord(char) < 127 for char in url) and
             not any(char in url for char in "\\%?#"), "noncanonical URL")
    try:
        parsed = urlsplit(url)
    except ValueError as exc:
        raise PolicyError("invalid URL") from exc
    _require(parsed.scheme == "https" and parsed.netloc in {
        "download.arduino.cc", "downloads.arduino.cc", "github.com", "files.pythonhosted.org",
    }, "URL must use an allowlisted HTTPS host, without credentials or port")
    segments = parsed.path.split("/")
    _require(segments[0] == "" and all(p not in {"", ".", ".."} for p in segments[1:]),
             "noncanonical URL path")
    _require(segments[-1] == name, "URL filename must match artifact filename")
    if category == "wheels":
        _require(parsed.netloc == "files.pythonhosted.org" and
                 parsed.path.startswith("/packages/"), "wheels require files.pythonhosted.org/packages/")
    elif parsed.netloc == "github.com":
        allowed = (parsed.path.startswith("/arduino/arduino-cli/releases/download/") or
                   parsed.path.startswith("/espressif/") or
                   FROZEN_CORE_DEPENDENCY_URLS.get(url) == (item["bytes"], item["sha256"]))
        _require(allowed, "GitHub URL outside reviewed Arduino/Espressif namespaces")
    else:
        _require(parsed.netloc in {"download.arduino.cc", "downloads.arduino.cc"},
                 "archives require Arduino or approved GitHub namespace")
    return Artifact(**item)


def _absolute(path):
    raw = os.fspath(path)
    _require(type(raw) is str and raw and "\x00" not in raw, "invalid filesystem path")
    _require(".." not in Path(raw).parts, "parent traversal in filesystem path")
    return Path(os.path.abspath(raw))


def _open_nofollow(path, *, directory=False):
    """Anchor every component with descriptors, never following symlink dirs."""
    _require(os.name == "posix" and hasattr(os, "O_NOFOLLOW") and
             os.open in os.supports_dir_fd and os.stat in os.supports_dir_fd and
             os.listdir in os.supports_fd, "safe descriptor operations unavailable")
    path = _absolute(path)
    flags = os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK
    fd = os.open(path.anchor, flags | os.O_DIRECTORY)
    try:
        parts = path.parts[1:]
        for index, part in enumerate(parts):
            is_dir = index < len(parts) - 1 or directory
            child = os.open(part, flags | (os.O_DIRECTORY if is_dir else 0), dir_fd=fd)
            os.close(fd)
            fd = child
        return fd
    except BaseException:
        os.close(fd)
        raise


def _signature(info):
    return (info.st_dev, info.st_ino, info.st_mode, info.st_nlink,
            info.st_size, info.st_mtime_ns, info.st_ctime_ns)


def load_trusted_lock(lock_path, expected_lock_sha256, cache_root, context):
    """Read a reviewed external lock, binding its raw bytes and full context.

    The expected digest must not originate from the restored cache. The API
    cannot authenticate the caller or independently prove a human review.
    """
    expected_context = validate_context(context)
    _sha(expected_lock_sha256, "expected lock sha256")
    lock_path, cache_root = _absolute(lock_path), _absolute(cache_root)
    _require(lock_path != cache_root and cache_root not in lock_path.parents,
             "trusted lock must be outside cache root")
    try:
        fd = _open_nofollow(lock_path)
        try:
            before = os.fstat(fd)
            _require(stat.S_ISREG(before.st_mode) and before.st_nlink == 1,
                     "lock must be a regular non-hardlinked file")
            _require(before.st_size <= MAX_LOCK_BYTES, "lock exceeds size limit")
            with os.fdopen(fd, "rb", closefd=False) as stream:
                raw = stream.read(MAX_LOCK_BYTES + 1)
            _require(len(raw) <= MAX_LOCK_BYTES and _signature(before) == _signature(os.fstat(fd)),
                     "lock changed while reading or exceeds size limit")
        finally:
            os.close(fd)
    except OSError as exc:
        raise PolicyError(f"cannot safely read external lock: {exc}") from exc
    digest = hashlib.sha256(raw).hexdigest()
    _require(digest == expected_lock_sha256, "external lock SHA-256 mismatch")
    data = _strict_json(raw)
    _exact_keys(data, {"schema", "context", "artifacts"}, "lock")
    _require(type(data["schema"]) is int and data["schema"] == SCHEMA, "unsupported lock schema")
    _require(validate_context(data["context"]) == expected_context, "lock context mismatch")
    items = data["artifacts"]
    _require(type(items) is list and 0 < len(items) <= MAX_ARTIFACTS,
             "nonempty bounded artifact list required")
    artifacts = tuple(_validate_artifact(item) for item in items)
    paths = [item.path.casefold() for item in artifacts]
    _require(len(paths) == len(set(paths)), "duplicate or case-colliding paths")
    _require(len({item.url for item in artifacts}) == len(artifacts), "duplicate URLs")
    _require(sum(item.bytes for item in artifacts) <= MAX_TOTAL_BYTES, "lock total exceeds limit")
    return TrustedLock(digest, expected_context, artifacts)


def exact_cache_key(lock):
    """All dimensions and the raw lock SHA are bound; no restore prefixes."""
    context = dict(lock.context)
    payload = json.dumps(context, sort_keys=True, separators=(",", ":")).encode("utf-8")
    context_sha = hashlib.sha256(payload).hexdigest()
    return (f"ci-downloads-v{SCHEMA}-{context['benchmark_salt']}-"
            f"{context['runner_os']}-{context['runner_arch']}-{context_sha}-{lock.sha256}")


def _verify_file(directory_fd, name, item):
    before = os.stat(name, dir_fd=directory_fd, follow_symlinks=False)
    _require(stat.S_ISREG(before.st_mode) and before.st_nlink == 1,
             f"not a regular single-link distribution: {item.path}", CacheRejected)
    _require(not before.st_mode & 0o111, f"executable cache file: {item.path}", CacheRejected)
    _require(before.st_size == item.bytes, f"size mismatch: {item.path}", CacheRejected)
    fd = os.open(name, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK, dir_fd=directory_fd)
    try:
        _require(_signature(before) == _signature(os.fstat(fd)),
                 f"file changed before read: {item.path}", CacheRejected)
        digest = hashlib.sha256()
        remaining = item.bytes
        while remaining:
            chunk = os.read(fd, min(1024 * 1024, remaining))
            _require(bool(chunk), f"truncated file: {item.path}", CacheRejected)
            digest.update(chunk)
            remaining -= len(chunk)
        _require(not os.read(fd, 1), f"growing file: {item.path}", CacheRejected)
        _require(_signature(before) == _signature(os.fstat(fd)),
                 f"file changed during read: {item.path}", CacheRejected)
        _require(digest.hexdigest() == item.sha256, f"digest mismatch: {item.path}", CacheRejected)
    finally:
        os.close(fd)
    return _signature(before)


def _verify_tree(cache_root, lock):
    groups = {}
    for item in lock.artifacts:
        category, name = item.path.split("/")
        groups.setdefault(category, {})[name] = item
    root_fd = _open_nofollow(cache_root, directory=True)
    try:
        root_before = _signature(os.fstat(root_fd))
        _require(set(os.listdir(root_fd)) == set(groups),
                 "missing or unexpected root paths", CacheRejected)
        directory_signatures = {}
        for category, items in sorted(groups.items()):
            before = os.stat(category, dir_fd=root_fd, follow_symlinks=False)
            _require(stat.S_ISDIR(before.st_mode), f"not a real directory: {category}", CacheRejected)
            fd = os.open(category, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=root_fd)
            try:
                _require(_signature(before) == _signature(os.fstat(fd)),
                         "directory changed before read", CacheRejected)
                _require(set(os.listdir(fd)) == set(items),
                         f"missing or unexpected paths in {category}", CacheRejected)
                checked = {name: _verify_file(fd, name, item) for name, item in sorted(items.items())}
                for name, signature in checked.items():
                    _require(signature == _signature(os.stat(name, dir_fd=fd, follow_symlinks=False)),
                             "file replaced during tree verification", CacheRejected)
                _require(set(os.listdir(fd)) == set(items) and
                         _signature(before) == _signature(os.fstat(fd)),
                         "directory changed during verification", CacheRejected)
            finally:
                os.close(fd)
            directory_signatures[category] = _signature(before)
        for category, signature in directory_signatures.items():
            _require(signature == _signature(os.stat(category, dir_fd=root_fd, follow_symlinks=False)),
                     "directory replaced during tree verification", CacheRejected)
        _require(set(os.listdir(root_fd)) == set(groups) and
                 root_before == _signature(os.fstat(root_fd)),
                 "cache root changed during verification", CacheRejected)
        reopened = _open_nofollow(cache_root, directory=True)
        try:
            _require(root_before == _signature(os.fstat(reopened)),
                     "cache root path replaced during verification", CacheRejected)
        finally:
            os.close(reopened)
    finally:
        os.close(root_fd)


def evaluate_cache(cache_root, lock_path, expected_lock_sha256, context, restored_key):
    """Return a hit only after validating the exact key and every cache path.

    Invalid trust inputs raise PolicyError (stop). Untrusted/missing cache
    returns action=clean_download (exit 3 in CLI), never partial reuse.
    """
    lock = load_trusted_lock(lock_path, expected_lock_sha256, cache_root, context)
    key = exact_cache_key(lock)
    if restored_key != key:
        return CacheDecision(False, "clean_download", key, "missing or non-exact restored key")
    try:
        _verify_tree(cache_root, lock)
    except (CacheRejected, PolicyError, OSError) as exc:
        return CacheDecision(False, "clean_download", key, str(exc))
    return CacheDecision(True, "use_verified_downloads", key, "all paths, sizes and SHA-256 verified", len(lock.artifacts))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("key", "verify"))
    parser.add_argument("--lock", required=True)
    parser.add_argument("--expected-lock-sha256", required=True)
    parser.add_argument("--cache-root", required=True)
    parser.add_argument("--context-json", required=True)
    parser.add_argument("--restored-key")
    args = parser.parse_args(argv)
    try:
        context = _strict_json(args.context_json)
        if args.command == "key":
            lock = load_trusted_lock(args.lock, args.expected_lock_sha256, args.cache_root, context)
            print(exact_cache_key(lock))
            return 0
        result = evaluate_cache(args.cache_root, args.lock, args.expected_lock_sha256, context, args.restored_key)
        print(json.dumps(asdict(result), sort_keys=True))
        return 0 if result.hit else 3
    except (PolicyError, TypeError) as exc:
        print(json.dumps({"action": "stop", "error": str(exc)}, sort_keys=True), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
