#!/usr/bin/env python3
"""Bounded, text-only failed-link diagnostics; never compile or qualify firmware."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import resource
import shlex
import stat
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
from record_coop_training_build import CORE, FQBN, check_options

ROOT = Path(__file__).absolute().parents[1]
NAMES = {'link-diagnostics.txt', 'diagnostic-identity.json'}
TEXT_LIMIT = 1024 * 1024
JSON_LIMIT = 64 * 1024
READ_LIMIT = 32 * 1024 * 1024
COMMAND_LIMIT = 2 * 1024 * 1024
TARGET = re.compile(r'^\.(?:text|literal)\.[\w.$]*?(?:installUpdate|updateInstallTask)[\w.$]*$')
TOOL_SUFFIXES = {'size', 'nm', 'objdump', 'readelf'}
PREFIXES = ('xtensa-esp32s3-elf-', 'xtensa-esp-elf-')
OBJECTS = ('RPG_POKET_2.ino.cpp.o', 'CoopRuntime.cpp.o', 'UpdateInstall.cpp.o')
SECRET = re.compile(r'password|passwd|secret|token|api[_-]?key|credential|authorization', re.I)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def safe_path(path, roots=None):
    """Reject symlinks at every component, lexical escapes and special files."""
    path = Path(path).absolute()
    require('..' not in path.parts, 'Parent traversal rejected: ' + str(path))
    for item in (path, *path.parents):
        if item.exists() or item.is_symlink():
            mode = item.lstat().st_mode
            require(not stat.S_ISLNK(mode), 'Symlink rejected: ' + str(item))
            require(stat.S_ISDIR(mode) or stat.S_ISREG(mode), 'Special file rejected: ' + str(item))
    if roots is not None:
        require(any(path.is_relative_to(root) for root in roots), 'Path outside approved roots: ' + str(path))
    return path


def read_text(path, roots, limit=READ_LIMIT):
    path = safe_path(path, roots)
    require(path.is_file(), 'File unavailable: ' + str(path))
    require(path.stat().st_size <= limit, 'Input exceeds byte cap: ' + str(path))
    with path.open('rb') as handle:
        data = handle.read(limit + 1)
    require(len(data) <= limit and b'\0' not in data, 'Non-text or over-limit input: ' + str(path))
    return data.decode('utf-8', errors='replace')


def digest(path, roots):
    path = safe_path(path, roots)
    require(path.is_file() and path.stat().st_size <= 128 * 1024 * 1024, 'Invalid/over-limit object: ' + str(path))
    h = hashlib.sha256()
    count = 0
    with path.open('rb') as handle:
        for chunk in iter(lambda: handle.read(65536), b''):
            count += len(chunk)
            require(count <= 128 * 1024 * 1024, 'Object grew beyond cap')
            h.update(chunk)
    return {'bytes': count, 'sha256': h.hexdigest()}


def run(argv, cwd=ROOT, limit=COMMAND_LIMIT, timeout=20):
    """No shell or inherited credentials; bounded temporary text, deleted on close."""
    def cap_output():
        resource.setrlimit(resource.RLIMIT_FSIZE, (limit, limit))
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    with tempfile.TemporaryFile() as output:
        reason = None
        try:
            result = subprocess.run([str(a) for a in argv], cwd=cwd, stdin=subprocess.DEVNULL,
                                    stdout=output, stderr=subprocess.STDOUT, timeout=timeout,
                                    env={'PATH': os.defpath, 'LC_ALL': 'C'}, preexec_fn=cap_output)
            returncode = result.returncode
        except subprocess.TimeoutExpired:
            returncode, reason = -1, 'timeout'
        output.seek(0)
        data = output.read(limit)
    if len(data) == limit:
        reason = reason or 'output cap'
    return {'text': data.decode('utf-8', errors='replace').replace('\0', '[NUL]'),
            'returncode': returncode, 'limited': reason}


def git(*args):
    result = run(['git', '--no-optional-locks', *args])
    require(result['returncode'] == 0 and not result['limited'], 'Git identity check failed')
    return result['text'].strip()


def validate_identity(sha, tree, status, environment):
    require(bool(re.fullmatch('[0-9a-f]{40}', sha)), 'Invalid source SHA')
    require(bool(re.fullmatch('[0-9a-f]{40}', tree)), 'Invalid source tree')
    require(environment.get('GITHUB_SHA') == sha, 'GITHUB_SHA differs from exact HEAD')
    require(environment.get('GITHUB_REF') == 'refs/heads/feat/coop-training-check', 'Unexpected branch ref')
    require(environment.get('GITHUB_EVENT_NAME') == 'push', 'Only the isolated push event is allowed')
    require(not status, 'Exact clean source required')
    return {'source_commit': sha, 'source_tree': tree, 'source_status': 'clean',
            'event': 'push', 'ref': 'refs/heads/feat/coop-training-check'}


def validate_options(options, flag):
    result = check_options(options, flag)
    # Version-bearing paths, when present in Arduino's options, must agree.
    versions = re.findall(r'/hardware/esp32/([^/"\\\s,]+)', json.dumps(options))
    require(all(version == CORE for version in versions), 'Options reference another ESP32 core')
    result['core_versions_in_options'] = sorted(set(versions))
    return result


def validate_output(folder, complete=False):
    safe_path(folder)
    if not folder.exists():
        require(not complete, 'Diagnostic output missing')
        return
    require(folder.is_dir(), 'Diagnostic output must be a directory')
    found = set()
    for path in folder.iterdir():
        safe_path(path, [folder])
        require(path.name in NAMES and path.is_file(), 'Non-diagnostic artifact rejected: ' + path.name)
        read_text(path, [folder], JSON_LIMIT if path.suffix == '.json' else TEXT_LIMIT)
        found.add(path.name)
    require(not complete or found == NAMES, 'Diagnostic file missing')


class Report:
    def __init__(self):
        self.text = ''
        self.truncated = False
        self.incomplete = False

    def add(self, title, text):
        self.incomplete |= 'UNAVAILABLE:' in str(text) or 'capped at' in str(text)
        text = re.sub(r'[\x00-\x08\x0b\x0c\x0e-\x1f\x7f]', '?', str(text))
        block = '\n=== ' + title + ' ===\n' + text.rstrip() + '\n'
        room = TEXT_LIMIT - len(self.text.encode()) - 100
        if len(block.encode()) > max(0, room):
            block = block.encode()[:max(0, room)].decode('utf-8', errors='ignore')
            if not self.truncated:
                block += '\n[Report byte cap reached; remaining detail omitted.]\n'
            self.truncated = True
        self.text += block


def section_names(headers):
    names = []
    for line in headers.splitlines():
        match = re.match(r'^\s*\d+\s+(\S+)\s+[0-9a-fA-F]+\s', line)
        if match and TARGET.fullmatch(match[1]):
            names.append(match[1])
    require(len(names) <= 32 and len(names) == len(set(names)), 'Too many/duplicate target sections')
    return names


def section_summary(text):
    rows = []
    for line in text.splitlines():
        match = re.match(r'^\s*(\S+)\s+(\d+)\s+\d+\s*$', line)
        if match and match[1].startswith('.'):
            rows.append((int(match[2]), match[1]))
    top = sorted(rows, reverse=True)[:30]
    return ('Parsed sections: %d; sum of reported section sizes: %d bytes (includes non-alloc/debug).\n'
            'Largest 30 sections, bytes name:\n' % (len(rows), sum(x[0] for x in rows))) + '\n'.join('%d %s' % row for row in top)


def symbol_summary(text):
    rows = []
    for line in text.splitlines():
        match = re.match(r'^\s*[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+\S\s+(.+)$', line)
        if match:
            rows.append((int(match[1], 16), line))
    return '\n'.join(row[1] for row in sorted(rows, reverse=True)[:30]) or '[No defined sized symbols reported.]'


def map_context(text):
    lines = text.splitlines()
    selected = set()
    for i, line in enumerate(lines):
        if re.search(r'\.(?:text|literal)\.\S*(?:installUpdate|updateInstallTask)', line):
            selected.update(range(max(0, i - 3), min(len(lines), i + 5)))
    if not selected:
        return '[No matching target section names in this map.]'
    return '\n'.join('%d: %s' % (i + 1, lines[i]) for i in sorted(selected)[:500]) + ('\n[Map context capped at 500 lines.]' if len(selected) > 500 else '')


def find_tools(folder):
    # Arduino packages install tools at tools/<package>/<version>/bin, not PATH.
    available = {}
    for suffix in sorted(TOOL_SUFFIXES):
        for prefix in PREFIXES:
            matches = sorted(folder.glob('*/*/bin/' + prefix + suffix))
            require(len(matches) <= 1, 'Ambiguous installed Xtensa ' + suffix)
            if matches:
                tool = safe_path(matches[0], [folder])
                require(tool.is_file() and os.access(tool, os.X_OK), 'Target tool is not executable')
                available[suffix] = tool
                break
    return available


def redact(text):
    # Do not read or serialize environment values; mask sensitive-looking options.
    return '\n'.join('[Sensitive-looking command/response line omitted.]' if SECRET.search(line) else line
                     for line in text.splitlines())


def compile_commands(text, sdk, build):
    """Select original verbose compiler/link lines, never execute logged argv."""
    commands = []
    for line in text.splitlines():
        if not any(name in line for name in (*OBJECTS, '.elf')) or len(line) > 65536:
            continue
        try:
            argv = shlex.split(line)
        except ValueError:
            continue
        if not argv or Path(argv[0]).name not in {p + c for p in PREFIXES for c in ('g++', 'gcc')}:
            continue
        safe_path(argv[0], [sdk])
        if '-o' not in argv or argv.index('-o') + 1 == len(argv):
            continue
        output = safe_path(argv[argv.index('-o') + 1], [build])
        if (output.name in OBJECTS and '-c' in argv) or (output.name == 'RPG_POKET_2.ino.elf' and '-c' not in argv):
            commands.append((line, argv))
    require(len(commands) <= 16, 'Too many target compile/link commands')
    return commands


def response_paths(argv, sdk, build):
    paths = []
    for arg in argv:
        if not arg.startswith('@'):
            continue
        raw = Path(arg[1:])
        require(raw.is_absolute(), 'Relative response file rejected; verbose command cwd is unverified')
        path = safe_path(raw, [sdk, build])
        if path.is_relative_to(sdk):
            require(path.parent.name == 'flags' and path.name in {'c_flags', 'cpp_flags', 'defines', 'includes', 'ld_flags', 'ld_scripts', 'ld_libs'}, 'Non-SDK-response file rejected')
        else:
            require(path.name in {'build_opt.h', 'build.opt', 'file_opts'} or path.suffix in {'.rsp', '.response'}, 'Non-build-response file rejected')
        paths.append(path)
    require(len(paths) <= 16, 'Response file count cap exceeded')
    return paths


def capture_objects(flag, folder, tools, report):
    records = {}
    for name in OBJECTS:
        path = safe_path(folder / 'sketch' / name, [folder])
        label = 'flag-%d %s' % (flag, name)
        if not path.exists():
            records[name] = {'available': False}
            report.add(label, 'UNAVAILABLE: object not produced/preserved by this failed build.')
            continue
        records[name] = {'available': True, **digest(path, [folder])}
        report.add(label + ' identity', json.dumps(records[name], sort_keys=True))
        def invoke(suffix, args, title, transform=None):
            if suffix not in tools:
                report.add(label + ' ' + title, 'UNAVAILABLE: installed Xtensa ' + suffix + ' not found.')
                return ''
            result = run([tools[suffix], *args, path])
            body = result['text']
            report.incomplete |= bool(result['limited'] or result['returncode'])
            if transform:
                body = transform(body)
            report.add(label + ' ' + title, 'argv: ' + shlex.join([str(tools[suffix]), *args, str(path)]) +
                       '\nreturncode=%s; limited=%s\n' % (result['returncode'], result['limited']) + body)
            return result['text'] if not result['limited'] and result['returncode'] == 0 else ''
        invoke('size', ['-A', '-d'], 'section summary', section_summary)
        invoke('nm', ['-S', '-C', '--defined-only', '--size-sort', '--reverse-sort'], 'largest 30 symbols', symbol_summary)
        invoke('readelf', ['-h'], 'ELF ABI header')
        invoke('objdump', ['-s', '-j', '.xtensa.info'], 'Xtensa ABI metadata')
        if 'objdump' not in tools:
            report.add(label + ' target sections', 'UNAVAILABLE: objdump missing.')
            continue
        headers = run([tools['objdump'], '-h', path])
        if headers['limited'] or headers['returncode']:
            report.add(label + ' target section discovery', 'UNAVAILABLE: objdump headers incomplete; returncode=%s; limited=%s' % (headers['returncode'], headers['limited']))
            continue
        names = section_names(headers['text'])
        records[name]['target_sections'] = names
        if not names:
            report.incomplete |= name in ('RPG_POKET_2.ino.cpp.o', 'UpdateInstall.cpp.o')
            report.add(label + ' target sections', 'No installUpdate/updateInstallTask text/literal sections found.')
        else:
            selected = [item for section in names for item in ('-j', section)]
            invoke('objdump', ['-h', *selected], 'target section headers')
            invoke('objdump', ['-r', *selected], 'target relocations')
            code = [item for section in names if section.startswith('.text.') for item in ('-j', section)]
            if code:
                invoke('objdump', ['-d', *code], 'target disassembly')
    return records


def collect_maps(folder, report, flag):
    # ESP32 core 3.3.12 recipe emits --Map={build.path}/{build.project_name}.map.
    path = safe_path(folder / 'RPG_POKET_2.ino.map', [folder])
    if not path.exists():
        report.add('flag-%d linker map' % flag, 'UNAVAILABLE: no core-generated .map exists; final section placement and literal reach cannot be established.')
        return []
    context = map_context(read_text(path, [folder]))
    report.incomplete |= context == '[No matching target section names in this map.]'
    report.add('flag-%d map %s' % (flag, path.name), context)
    return [path.name]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-root', type=Path, required=True)
    parser.add_argument('--compile-logs', type=Path, required=True)
    parser.add_argument('--out-dir', type=Path, required=True)
    parser.add_argument('--tools-dir', type=Path, default=Path.home() / '.arduino15/packages/esp32/tools')
    args = parser.parse_args()
    root = safe_path(ROOT)
    build = safe_path(args.build_root)
    logs = safe_path(args.compile_logs)
    out = safe_path(args.out_dir)
    sdktools = safe_path(args.tools_dir)
    require(not build.is_relative_to(root) and not root.is_relative_to(build), 'Build root must be external to checkout')
    require(logs == root / 'build/coop-training-evidence', 'Use dedicated compile log directory')
    require(out == root / 'build/coop-training-link-diagnostics', 'Use dedicated diagnostic output directory')
    require(sdktools.name == 'tools' and sdktools.parent.name == 'esp32', 'Use installed ESP32 SDK tools directory')
    require(git('rev-parse', '--show-toplevel') == str(root), 'Unexpected repository root')
    identity = validate_identity(git('rev-parse', 'HEAD'), git('rev-parse', 'HEAD^{tree}'), git('status', '--porcelain', '--untracked-files=all'), os.environ)
    validate_output(out)
    report = Report()
    report.add('Diagnostic scope', 'EXPERIMENTAL FAILED-LINK TEXT ONLY. No compilation, flags, source, firmware artifacts, or release qualification changed.\nObject addresses are section-relative; actual placement requires a linker map. Partial maps may describe an unsuccessful link.')
    report.add('Source identity', json.dumps(identity, indent=2, sort_keys=True))
    identity.update(schema=1, kind='EXPERIMENTAL_FAILED_LINK_DIAGNOSTICS_ONLY', expected_core=CORE,
                    expected_fqbn=FQBN, target_compile_verified=False, release_qualified=False,
                    binary_uploaded=False, install_authorized=False, builds={})
    toolchain = safe_path(logs / 'toolchain.txt', [logs])
    if toolchain.exists():
        text = read_text(toolchain, [logs], 65536)
        versions = re.findall(r'^esp32:esp32\s+(\S+)', text, re.M)
        require(versions == [CORE] and 'arduino-cli' in text.lower(), 'Installed core/toolchain evidence mismatch')
        identity['installed_core_evidence'] = CORE
        report.add('Installed core', 'toolchain.txt reports esp32:esp32 ' + CORE)
    else:
        identity['installed_core_evidence'] = None
        report.add('Installed core', 'UNAVAILABLE: toolchain.txt missing; installed core not verified.')
    tools = find_tools(sdktools)
    identity['tools'] = {name: str(path.relative_to(sdktools)) for name, path in tools.items()}
    sdk = sdktools.parent
    for flag in (0, 1):
        folder = safe_path(build / ('flag-%d' % flag), [build])
        record = {}
        options = safe_path(folder / 'build.options.json', [folder])
        if options.exists():
            record['effective_options'] = validate_options(json.loads(read_text(options, [folder], 256 * 1024)), flag)
            report.add('flag-%d actual build.options.json' % flag, json.dumps(record['effective_options'], indent=2, sort_keys=True))
        else:
            record['effective_options'] = None
            report.add('flag-%d actual options' % flag, 'UNAVAILABLE: build.options.json not produced/preserved; effective flag/FQBN cannot be verified.')
        log = safe_path(logs / ('esp32-flag-%d.log' % flag), [logs])
        if log.exists():
            text = read_text(log, [logs])
            commands = compile_commands(text, sdk, build)
            report.add('flag-%d actual verbose compile/link lines' % flag, '\n'.join(redact(line) for line, _ in commands) or 'UNAVAILABLE: no matching verbose target compile/link command lines. Compile must use --verbose to preserve these.')
            errors = [line for line in text.splitlines() if 'dangerous relocation' in line or 'literal placed after use' in line or 'collect2: error:' in line or 'ld returned' in line]
            report.add('flag-%d observed linker errors' % flag, redact('\n'.join(errors[:250])) or '[No matching linker-error lines in captured log.]')
            seen = set()
            for _, argv in commands:
                for response in response_paths(argv, sdk, build):
                    if response in seen:
                        continue
                    seen.add(response)
                    require(len(seen) <= 24, 'Unique response file cap exceeded')
                    if response.exists():
                        content = read_text(response, [sdk, build], 32768)
                        require(not any(a.startswith('@') for a in shlex.split(content)), 'Nested response files are not captured')
                        report.add('flag-%d response %s' % (flag, response), redact(content))
                    else:
                        report.add('flag-%d response %s' % (flag, response), 'UNAVAILABLE: referenced response file missing.')
            record['captured_command_count'] = len(commands)
        else:
            record['captured_command_count'] = 0
            report.add('flag-%d compile log' % flag, 'UNAVAILABLE: compile log absent; this flag may not have run.')
        record['objects'] = capture_objects(flag, folder, tools, report)
        record['maps'] = collect_maps(folder, report, flag)
        identity['builds'][str(flag)] = record
    # Recheck immutable source before emitting the only two allowed artifacts.
    require(git('rev-parse', 'HEAD') == identity['source_commit'] and not git('status', '--porcelain', '--untracked-files=all'), 'Source changed while collecting diagnostics')
    identity['diagnostics_incomplete'] = report.incomplete or report.truncated
    identity['report_truncated'] = report.truncated
    identity['report_sha256'] = hashlib.sha256(report.text.encode()).hexdigest()
    serialized = json.dumps(identity, indent=2, sort_keys=True) + '\n'
    require(len(serialized.encode()) <= JSON_LIMIT and len(report.text.encode()) <= TEXT_LIMIT, 'Output byte cap exceeded')
    out.mkdir(parents=True, exist_ok=True)
    validate_output(out)
    (out / 'link-diagnostics.txt').write_text(report.text, encoding='utf-8')
    (out / 'diagnostic-identity.json').write_text(serialized, encoding='utf-8')
    validate_output(out, complete=True)
    print('Collected bounded text-only diagnostics. This does not change or qualify the failed compile.')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, AssertionError, OSError, KeyError, TypeError) as error:
        print('Diagnostic collection rejected: ' + str(error), file=sys.stderr)
        sys.exit(1)
