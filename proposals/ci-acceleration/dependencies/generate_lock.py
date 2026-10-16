#!/usr/bin/env python3
"""Reproduce the reviewed distribution lock from frozen official metadata.

Offline only. No package resolution, downloads, installation, or execution.
Refreshing sources is an explicit review operation, never a cache-miss action.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import cache_policy as policy

CLI_COMMIT = "fef6e48df6311752b7bfc0498e53e35f37869560"
CONTEXT = {
    "runner_os": "Linux", "runner_arch": "X64", "python_version": "3.12.3",
    "arduino_cli_version": "1.5.2-rc.1", "esp32_core_version": "3.3.12",
    "pillow_version": "11.3.0", "pyyaml_version": "6.0.3",
    "fqbn": "esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,EraseFlash=none",
    "benchmark_salt": "benchmark-coopdiag77-downloads-v1",
}
BUILTINS = {"ctags": "5.8-arduino11", "dfu-discovery": "0.1.2",
            "mdns-discovery": "1.1.0", "serial-discovery": "1.5.2",
            "serial-monitor": "0.15.0"}
WHEELS = {
    "pillow": "pillow-11.3.0-cp312-cp312-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl",
    "pyyaml": "pyyaml-6.0.3-cp312-cp312-manylinux2014_x86_64.manylinux_2_17_x86_64.manylinux_2_28_x86_64.whl",
}


def require(value, message):
    if not value:
        raise policy.PolicyError(message)


def encode(data):
    return (json.dumps(data, indent=2, sort_keys=True) + "\n").encode()


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def unique(items, label):
    result = list(items)
    require(len(result) == 1, "Expected exactly one " + label)
    return result[0]


def sources(root=HERE):
    manifest = policy._strict_json((root / "sources.lock.json").read_bytes())
    require(manifest["schema"] == 1, "Wrong source manifest schema")
    result = {}
    for entry in manifest["sources"]:
        path = entry["path"]
        require(re.fullmatch(r"sources/[A-Za-z0-9._-]+", path), "Invalid source path")
        require(path not in result, "Duplicate source")
        raw = (root / path).read_bytes()
        require(len(raw) == entry["bytes"] and sha(raw) == entry["sha256"],
                "Frozen official metadata changed: " + path)
        result[path] = raw
    return result


def archive(record):
    checksum = record["checksum"]
    require(checksum.startswith("SHA-256:"), "Only published SHA-256 is accepted")
    return {"path": "archives/" + record["archiveFileName"], "url": record["url"],
            "bytes": int(record["size"]), "sha256": checksum.removeprefix("SHA-256:")}


def generate(root=HERE):
    raw_sources = sources(root)
    def read(name):
        return policy._strict_json(raw_sources["sources/" + name])

    cli = read("arduino-cli-release.json")
    tag = read("arduino-cli-tag.json")
    require(cli["tag_name"] == "v" + CONTEXT["arduino_cli_version"] and
            not cli["draft"] and cli["prerelease"] is True, "Wrong official CLI release")
    require(tag["object"] == {"sha": CLI_COMMIT, "type": "commit",
            "url": "https://api.github.com/repos/arduino/arduino-cli/git/commits/" + CLI_COMMIT},
            "CLI tag no longer identifies observed commit")
    require(read("arduino-cli-commit.json")["sha"] == CLI_COMMIT, "Wrong CLI commit")
    cli_asset = unique((a for a in cli["assets"] if a["name"] ==
                       "arduino-cli_1.5.2-rc.1_Linux_64bit.tar.gz"), "CLI Linux64 asset")
    checksums = raw_sources["sources/arduino-cli-checksums.txt"]
    checksum_asset = unique((a for a in cli["assets"] if a["name"] ==
                            "1.5.2-rc.1-checksums.txt"), "CLI checksum asset")
    require(checksum_asset["digest"] == "sha256:" + sha(checksums) and
            checksum_asset["size"] == len(checksums), "CLI checksum asset digest mismatch")
    checks = {line.split()[1].lstrip("*"): line.split()[0]
              for line in checksums.decode().splitlines() if line.strip()}
    cli_sha = checks[cli_asset["name"]]
    require(cli_asset["digest"] == "sha256:" + cli_sha, "CLI release/checksum disagreement")
    artifacts = [{"path": "archives/" + cli_asset["name"],
                  "url": cli_asset["browser_download_url"],
                  "bytes": cli_asset["size"], "sha256": cli_sha}]

    requirements = []
    for project, filename in WHEELS.items():
        version = CONTEXT[project + "_version"]
        metadata = read(project + "-" + version + ".pypi.json")
        require(metadata["info"]["name"].lower() == project and
                metadata["info"]["version"] == version, "Wrong PyPI release")
        # No extras are requested. Reject unreviewed mandatory/OR dependencies.
        for dep in metadata["info"].get("requires_dist") or []:
            require(";" in dep and re.search(r'extra == "[A-Za-z0-9_-]+"', dep) and
                    " or " not in dep, "Unresolved Python dependency: " + dep)
        wheel = unique((u for u in metadata["urls"] if u["filename"] == filename),
                       "reviewed CPython 3.12 x86-64 wheel")
        require(wheel["packagetype"] == "bdist_wheel" and
                wheel["python_version"] == "cp312" and wheel["yanked"] is False,
                "Wheel is yanked or wrong ABI")
        artifacts.append({"path": "wheels/" + filename, "url": wheel["url"],
                          "bytes": wheel["size"], "sha256": wheel["digests"]["sha256"]})
        requirements.append(project + "==" + version + " --hash=sha256:" + wheel["digests"]["sha256"])

    esp_release = read("esp32-release.json")
    require(esp_release["tag_name"] == CONTEXT["esp32_core_version"] and
            esp_release["draft"] is False, "Wrong ESP32 release")
    index_asset = unique((a for a in esp_release["assets"] if a["name"] ==
                         "package_esp32_index.json"), "versioned ESP32 index")
    esp_raw = raw_sources["sources/package_esp32_index.json"]
    require(index_asset["digest"] == "sha256:" + sha(esp_raw) and
            index_asset["size"] == len(esp_raw), "ESP32 release/index digest disagreement")
    esp = unique((p for p in read("package_esp32_index.json")["packages"]
                  if p["name"] == "esp32"), "ESP32 package")
    platform = unique((p for p in esp["platforms"] if p["version"] ==
                       CONTEXT["esp32_core_version"]), "ESP32 platform")
    require(platform["architecture"] == "esp32" and not platform.get("libraryDependencies"),
            "Unresolved platform library dependencies")
    core_archive = archive(platform)
    core_asset = unique((a for a in esp_release["assets"] if a["name"] ==
                        platform["archiveFileName"]), "ESP32 core archive")
    require(core_asset["digest"] == "sha256:" + core_archive["sha256"] and
            core_asset["size"] == core_archive["bytes"], "ESP32 core metadata disagreement")
    artifacts.append(core_archive)

    official = {p["name"]: p for p in read("package_index.json")["packages"]}
    official["esp32"] = esp
    dependencies = list(platform["toolsDependencies"])
    dependencies += [{"packager": "builtin", "name": name, "version": version}
                     for name, version in sorted(BUILTINS.items())]
    selected = {}
    closure = []
    for dep in dependencies:
        package_name, name, version = dep["packager"], dep["name"], dep["version"]
        package = official[package_name]
        tool = copy.deepcopy(unique((t for t in package["tools"] if t["name"] == name and
                                     t["version"] == version), "tool " + str(dep)))
        system = unique((s for s in tool["systems"] if s["host"] in
                         {"x86_64-pc-linux-gnu", "x86_64-linux-gnu"}), "Linux x64 tool system")
        tool["systems"] = [system]
        if package_name not in selected:
            selected[package_name] = {k: copy.deepcopy(v) for k, v in package.items()
                                      if k not in {"platforms", "tools"}}
            selected[package_name].update({"platforms": [], "tools": []})
        selected[package_name]["tools"].append(tool)
        item = archive(system)
        artifacts.append(item)
        closure.append({**dep, "host": system["host"], "artifact": item["path"]})
    selected["esp32"]["platforms"] = [copy.deepcopy(platform)]
    for kind in ("discoveryDependencies", "monitorDependencies"):
        for dep in platform.get(kind, []):
            require(dep["packager"] == "builtin" and dep["name"] in BUILTINS,
                    "Unresolved " + kind)
    artifacts.sort(key=lambda a: a["path"])
    names = [a["path"].casefold() for a in artifacts]
    require(len(names) == len(set(names)), "Distribution filename collision")
    for item in artifacts:
        policy._validate_artifact(item)
    policy.validate_context(CONTEXT)
    require(len(artifacts) == 27, "Reviewed dependency closure changed")
    lock = {"schema": 1, "context": CONTEXT, "artifacts": artifacts}
    products = {
        "downloads.lock.json": encode(lock),
        "frozen/package_index.json": encode({"packages": [selected[k] for k in sorted(selected)]}),
        # No external Arduino libraries are required by this pinned core/sketch.
        # Explicit empty local index prevents CLI first-use network initialization.
        "frozen/library_index.json": encode({"libraries": []}),
        "requirements.lock.txt": ("\n".join(requirements) + "\n").encode(),
        "closure.json": encode({"schema": 1, "arduino_cli_commit": CLI_COMMIT,
                                "platform": "esp32:esp32@3.3.12", "tools": closure,
                                "arduino_libraries": [], "python_extras": []}),
    }
    products["PIN.json"] = encode({"schema": 1, "context": CONTEXT,
                                   "lock_sha256": sha(products["downloads.lock.json"]),
                                   "arduino_cli_commit": CLI_COMMIT, "artifact_count": len(artifacts)})
    products["SHA256SUMS"] = "".join(sha(v) + "  " + k + "\n"
                                       for k, v in sorted(products.items())).encode()
    return products


def verify_generated(root=HERE):
    products = generate(root)
    for name, expected in products.items():
        require((root / name).read_bytes() == expected, "Generated file changed: " + name)
    return products


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="Explicitly regenerate reviewed local outputs")
    args = parser.parse_args()
    try:
        products = generate() if args.write else verify_generated()
        if args.write:
            for name, raw in products.items():
                path = HERE / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(raw)
        print(json.dumps({"status": "generated" if args.write else "verified",
                          "lock_sha256": sha(products["downloads.lock.json"]),
                          "artifacts": 27}))
        return 0
    except (OSError, KeyError, TypeError, ValueError) as exc:
        print(json.dumps({"action": "stop", "error": str(exc)}), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
