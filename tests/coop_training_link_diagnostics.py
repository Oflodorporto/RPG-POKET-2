#!/usr/bin/env python3
"""Offline synthetic diagnostics fixtures. These are not ESP32 compilation proof."""
from pathlib import Path
import contextlib
import io
import json
import os
import sys
import tempfile
import unittest
from unittest import mock

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import diagnose_coop_link as d


class DiagnosticsTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.base = Path(self.temp.name)
        self.sdk = self.base / 'sdk/packages/esp32'
        self.tools = self.sdk / 'tools'
        self.tools.mkdir(parents=True)
        self.build = self.base / 'target'
        self.build.mkdir()
        self.out = self.base / 'output'
        self.out.mkdir()

    def tearDown(self):
        self.temp.cleanup()

    def reject(self, fn):
        with self.assertRaises((ValueError, AssertionError, OSError, KeyError, TypeError)):
            fn()

    def options(self, flag):
        return {'fqbn': d.FQBN, 'customBuildProperties': 'compiler.cpp.extra_flags=-DRPG_ENABLE_COOP_TRAINING=' + str(flag)}

    def test_target_selection(self):
        headers = '''  0 .literal._Z17updateInstallTaskPv 00000044 00000000 00000000 00000034 2**2
  1 .text._Z13installUpdateb$isra$0 00000e90 00000000 00000000 00000078 2**2
  2 .text._Z17updateInstallTaskPv 00000080 00000000 00000000 00000f08 2**2
  3 .rodata.assetMeta 00008000 00000000 00000000 00000f88 2**2
  4 .debug_installUpdate 00000020 00000000 00000000 00008f88 2**0
'''
        selected = d.section_names(headers)
        self.assertEqual(selected, ['.literal._Z17updateInstallTaskPv', '.text._Z13installUpdateb$isra$0', '.text._Z17updateInstallTaskPv'])
        self.reject(lambda: d.section_names(headers + headers))
        self.assertEqual(d.section_names('garbage'), [])

    def test_maps_include_placement_and_absence(self):
        text = '''fixture map
.literal 0x42000000 0x100
 .literal._Z17updateInstallTaskPv
                0x42000100 0x44 sketch/RPG_POKET_2.ino.cpp.o
                0x42000200 unrelated
.text 0x42000400 0x2000
 .text._Z13installUpdateb$isra$0
                0x42000400 0xe90 sketch/RPG_POKET_2.ino.cpp.o
LAST
'''
        result = d.map_context(text)
        self.assertIn('0x42000100', result)
        self.assertIn('0x42000400', result)
        self.assertIn('No matching', d.map_context('ordinary map'))
        self.assertIn('capped', d.map_context('\n'.join('.text.installUpdate' for _ in range(1000))))
        report = d.Report()
        self.assertEqual(d.collect_maps(self.build, report, 1), [])
        self.assertIn('placement and literal reach cannot be established', report.text)
        (self.build / 'RPG_POKET_2.ino.map').write_text(text)
        self.assertEqual(d.collect_maps(self.build, report, 1), ['RPG_POKET_2.ino.map'])

    def test_paths_no_escape_symlink_or_special_files(self):
        file = self.build / 'file'
        file.write_text('ordinary')
        self.assertEqual(d.safe_path(file, [self.build]), file)
        self.reject(lambda: d.safe_path(self.base / 'outside', [self.build]))
        self.reject(lambda: d.safe_path(self.build / '../outside', [self.build]))
        (self.build / 'link').symlink_to(file)
        self.reject(lambda: d.safe_path(self.build / 'link', [self.build]))
        (self.build / 'dirlink').symlink_to(self.out, target_is_directory=True)
        self.reject(lambda: d.safe_path(self.build / 'dirlink/new.txt', [self.build]))
        (self.build / 'broken').symlink_to(self.base / 'missing')
        self.reject(lambda: d.safe_path(self.build / 'broken', [self.build]))
        os.mkfifo(self.build / 'fifo')
        self.reject(lambda: d.safe_path(self.build / 'fifo', [self.build]))

    def test_allowlist_and_text_bounds(self):
        d.validate_output(self.out)
        self.reject(lambda: d.validate_output(self.out, True))
        (self.out / 'link-diagnostics.txt').write_text('fixture')
        (self.out / 'diagnostic-identity.json').write_text('{}')
        d.validate_output(self.out, True)
        for name in ('firmware.o', 'firmware.elf', 'firmware.bin', 'manifest.json', 'extra.txt'):
            p = self.out / name
            p.write_text('fixture')
            self.reject(lambda: d.validate_output(self.out))
            p.unlink()
        p = self.out / 'diagnostic-identity.json'
        p.write_text('x' * (d.JSON_LIMIT + 1))
        self.reject(lambda: d.validate_output(self.out))
        p.write_bytes(b'{}\0')
        self.reject(lambda: d.validate_output(self.out))
        p.unlink()
        p.symlink_to(self.out / 'link-diagnostics.txt')
        self.reject(lambda: d.validate_output(self.out))
        p.unlink()
        p.mkdir()
        self.reject(lambda: d.validate_output(self.out))
        report = d.Report()
        for _ in range(10):
            report.add('large', 'é' * d.TEXT_LIMIT)
        self.assertTrue(report.truncated)
        self.assertLessEqual(len(report.text.encode()), d.TEXT_LIMIT)
        self.assertIn('Report byte cap reached', report.text)

    def test_identity_and_effective_options(self):
        env = {'GITHUB_SHA': 'a' * 40, 'GITHUB_REF': 'refs/heads/feat/coop-training-check', 'GITHUB_EVENT_NAME': 'push'}
        result = d.validate_identity('a' * 40, 'b' * 40, '', env)
        self.assertEqual(result['source_status'], 'clean')
        for key, value in (('GITHUB_SHA', 'c' * 40), ('GITHUB_REF', 'refs/heads/main'), ('GITHUB_EVENT_NAME', 'pull_request')):
            bad = dict(env, **{key: value})
            self.reject(lambda: d.validate_identity('a' * 40, 'b' * 40, '', bad))
        self.reject(lambda: d.validate_identity('a' * 40, 'b' * 40, ' M source.cpp', env))
        self.reject(lambda: d.validate_identity('invalid', 'b' * 40, '', env))
        for flag in (0, 1):
            self.assertEqual(d.validate_options(self.options(flag), flag)['training_define'], flag)
            self.reject(lambda: d.validate_options(self.options(1 - flag), flag))
            bad = self.options(flag)
            bad['fqbn'] = bad['fqbn'].replace('FlashSize=16M', 'FlashSize=8M')
            self.reject(lambda: d.validate_options(bad, flag))
            bad = self.options(flag)
            bad['runtime.platform.path'] = '/sdk/hardware/esp32/3.3.11'
            self.reject(lambda: d.validate_options(bad, flag))
            bad['runtime.platform.path'] = '/sdk/hardware/esp32/3.3.12'
            self.assertEqual(d.validate_options(bad, flag)['core_versions_in_options'], ['3.3.12'])

    def test_verbose_commands_and_responses(self):
        compiler = self.tools / 'xtensa/123/bin/xtensa-esp-elf-g++'
        response = self.tools / 'esp32-arduino-libs/idf/esp32s3/flags/cpp_flags'
        response.parent.mkdir(parents=True)
        response.write_text('-Os -mlongcalls\n-DPASSWORD=not-exported')
        obj = self.build / 'flag-1/sketch/RPG_POKET_2.ino.cpp.o'
        elf = self.build / 'flag-1/RPG_POKET_2.ino.elf'
        compile_line = f'"{compiler}" @"{response}" -c source.cpp -o "{obj}"'
        link_line = f'"{compiler}" "{obj}" -o "{elf}"'
        log = '\n'.join(['unrelated diagnostic', 'echo SECRET=hidden', compile_line, link_line])
        found = d.compile_commands(log, self.sdk, self.build)
        self.assertEqual([v[0] for v in found], [compile_line, link_line])
        self.assertEqual(d.response_paths(found[0][1], self.sdk, self.build), [response])
        ld_libs = response.with_name('ld_libs')
        ld_libs.write_text('-lstdc++ -lm -lc')
        self.assertEqual(d.response_paths(['@' + str(ld_libs)], self.sdk, self.build), [ld_libs])
        build_opt = self.build / 'flag-1/build.opt'
        self.assertEqual(d.response_paths(['@' + str(build_opt)], self.sdk, self.build), [build_opt])
        self.assertNotIn('not-exported', d.redact(d.read_text(response, [self.sdk])))
        self.reject(lambda: d.response_paths(['@' + str(self.base / 'secret.rsp')], self.sdk, self.build))
        self.reject(lambda: d.response_paths(['@relative.rsp'], self.sdk, self.build))
        self.reject(lambda: d.response_paths(['@' + str(self.sdk / 'credentials')], self.sdk, self.build))
        self.reject(lambda: d.compile_commands(compile_line.replace(str(obj), str(self.out / obj.name)), self.sdk, self.build))
        self.reject(lambda: d.compile_commands(compile_line.replace(str(compiler), '/unapproved/bin/' + compiler.name), self.sdk, self.build))
        self.assertEqual(d.compile_commands('echo ' + compile_line, self.sdk, self.build), [])
        response.unlink()
        response.symlink_to(self.base / 'missing')
        self.reject(lambda: d.response_paths(found[0][1], self.sdk, self.build))

    def test_tool_selection_and_bounded_invocation(self):
        self.assertEqual(d.find_tools(self.tools), {})
        binary = self.tools / 'xtensa/123/bin/xtensa-esp-elf-objdump'
        binary.parent.mkdir(parents=True)
        binary.write_text('offline fixture; never executed')
        binary.chmod(0o700)
        self.assertEqual(d.find_tools(self.tools), {'objdump': binary})
        other = self.tools / 'xtensa/456/bin/xtensa-esp-elf-objdump'
        other.parent.mkdir(parents=True)
        other.write_text('offline fixture; never executed')
        other.chmod(0o700)
        self.reject(lambda: d.find_tools(self.tools))
        result = d.run([sys.executable, '-c', 'print("x" * 100000)'], cwd=self.base, limit=128)
        self.assertEqual(result['limited'], 'output cap')
        self.assertLessEqual(len(result['text']), 128)
        result = d.run([sys.executable, '-c', 'import time; time.sleep(5)'], cwd=self.base, timeout=0.1)
        self.assertEqual(result['limited'], 'timeout')
        os.environ['DIAGNOSTIC_FIXTURE_SECRET'] = 'must-not-leak'
        try:
            result = d.run([sys.executable, '-c', 'import os; print(os.getenv("DIAGNOSTIC_FIXTURE_SECRET"))'], cwd=self.base)
        finally:
            del os.environ['DIAGNOSTIC_FIXTURE_SECRET']
        self.assertEqual(result['text'].strip(), 'None')

    def test_targeted_object_commands_and_missing_objects(self):
        folder = self.build / 'flag-1'
        report = d.Report()
        missing = d.capture_objects(1, folder, {}, report)
        self.assertFalse(missing[d.OBJECTS[0]]['available'])
        self.assertIn('UNAVAILABLE', report.text)
        (folder / 'sketch').mkdir(parents=True)
        (folder / 'sketch' / d.OBJECTS[0]).write_bytes(b'OFFLINE SYNTHETIC OBJECT')
        called = []
        def fake_run(argv):
            called.append([str(x) for x in argv])
            text = '  0 .text._Z13installUpdateb$isra$0 00000e90 00000000 00000000 00000034 2**2\n'
            return {'text': text, 'returncode': 0, 'limited': None}
        tools = {name: self.tools / ('xtensa-esp-elf-' + name) for name in d.TOOL_SUFFIXES}
        with mock.patch.object(d, 'run', fake_run):
            record = d.capture_objects(1, folder, tools, report)
        self.assertTrue(record[d.OBJECTS[0]]['available'])
        for argv in called:
            self.assertTrue(Path(argv[0]).name.endswith(tuple(d.TOOL_SUFFIXES)))
            if argv[0].endswith('objdump') and ('-d' in argv or '-r' in argv):
                self.assertIn('-j', argv)
                self.assertIn('.text._Z13installUpdateb$isra$0', argv)
        self.assertFalse(any('-s' in argv and '.xtensa.info' not in argv for argv in called))
        self.assertNotIn('OFFLINE SYNTHETIC OBJECT', report.text)

    def test_map_without_targets_is_incomplete(self):
        path = self.build / 'RPG_POKET_2.ino.map'
        for text, incomplete in [('ordinary map\n', True), ('.text._Z13installUpdateb$isra$0\n 0x42000400 0xe90 object.o\n', False)]:
            with self.subTest(incomplete=incomplete):
                path.write_text(text)
                report = d.Report()
                self.assertEqual(d.collect_maps(self.build, report, 1), [path.name])
                self.assertEqual(report.incomplete, incomplete)

    def test_only_sketch_requires_target_sections(self):
        folder = self.build / 'flag-1'
        (folder / 'sketch').mkdir(parents=True)
        for name in d.OBJECTS:
            (folder / 'sketch' / name).write_bytes(b'OFFLINE SYNTHETIC OBJECT')
        tools = {name: self.tools / ('xtensa-esp-elf-' + name) for name in d.TOOL_SUFFIXES}
        for sketch_has_target in (False, True):
            with self.subTest(sketch_has_target=sketch_has_target):
                def fake_run(argv):
                    # Runtime has no updater section in either case, as expected.
                    target = sketch_has_target and Path(argv[-1]).name == d.OBJECTS[0]
                    text = ' 0 .text._Z13installUpdateb$isra$0 00000e90 00000000 00000000 00000034 2**2\n' if target else ''
                    return {'text': text, 'returncode': 0, 'limited': None}
                report = d.Report()
                with mock.patch.object(d, 'run', fake_run):
                    records = d.capture_objects(1, folder, tools, report)
                self.assertEqual(report.incomplete, not sketch_has_target)
                self.assertEqual(records[d.OBJECTS[1]]['target_sections'], [])

    def test_end_to_end_missing_failed_build_outputs(self):
        root = self.base / 'checkout'
        logs = root / 'build/coop-training-evidence'
        out = root / 'build/coop-training-link-diagnostics'
        logs.mkdir(parents=True)
        (logs / 'toolchain.txt').write_text('arduino-cli Version: fixture\nesp32:esp32 3.3.12 3.3.12 ESP32\n')
        env = {'GITHUB_SHA': 'a' * 40, 'GITHUB_REF': 'refs/heads/feat/coop-training-check', 'GITHUB_EVENT_NAME': 'push'}
        def fake_git(*args):
            return {('rev-parse', '--show-toplevel'): str(root), ('rev-parse', 'HEAD'): 'a' * 40,
                    ('rev-parse', 'HEAD^{tree}'): 'b' * 40, ('status', '--porcelain', '--untracked-files=all'): ''}[args]
        argv = ['diagnose_coop_link.py', '--build-root', str(self.build), '--compile-logs', str(logs), '--out-dir', str(out), '--tools-dir', str(self.tools)]
        with mock.patch.object(d, 'ROOT', root), mock.patch.object(d, 'git', fake_git), mock.patch.dict(os.environ, env), mock.patch.object(sys, 'argv', argv), contextlib.redirect_stdout(io.StringIO()):
            d.main()
        self.assertEqual({p.name for p in out.iterdir()}, d.NAMES)
        identity = json.loads((out / 'diagnostic-identity.json').read_text())
        self.assertFalse(identity['target_compile_verified'])
        self.assertFalse(identity['binary_uploaded'])
        self.assertTrue(identity['diagnostics_incomplete'])
        self.assertIsNone(identity['builds']['1']['effective_options'])
        text = (out / 'link-diagnostics.txt').read_text()
        self.assertIn('build.options.json not produced', text)
        self.assertIn('compile log absent', text)
        self.assertIn('no core-generated .map', text)
        d.validate_output(out, True)
        # A dangling link is unsafe, not an ordinary missing failed-build output.
        option = self.build / 'flag-1/build.options.json'
        option.parent.mkdir()
        option.symlink_to(self.base / 'missing-options')
        with mock.patch.object(d, 'ROOT', root), mock.patch.object(d, 'git', fake_git), mock.patch.dict(os.environ, env), mock.patch.object(sys, 'argv', argv):
            self.reject(d.main)

    def test_summaries_never_dump_art_contents(self):
        summary = d.section_summary('.text 100 0\n.rodata.assetMeta 200 0\n.debug_info 300 0\n')
        self.assertIn('600 bytes', summary)
        symbols = '\n'.join('00000000 %08x R symbol%d' % (n, n) for n in range(50))
        top = d.symbol_summary(symbols)
        self.assertEqual(len(top.splitlines()), 30)
        self.assertIn('symbol49', top)
        self.assertNotIn(' symbol0\n', top)


if __name__ == '__main__':
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(DiagnosticsTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful():
        sys.exit(1)
    print('PASS: offline synthetic link-diagnostic fixtures; no ESP32 compile/link or hardware claim.')
