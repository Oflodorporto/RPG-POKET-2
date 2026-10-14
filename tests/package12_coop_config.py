#!/usr/bin/env python3
"""Compile the package12 default/override contract without an ESP32 SDK."""
from pathlib import Path
import os
import importlib.util
import shlex
import subprocess
import tempfile
import sys
from unittest import mock

sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parents[1]


def cadence_probes(compiler):
    # Keep the frozen package7 extractor unambiguous. It intentionally selects
    # the first occurrence, so a training-only predicate must not shadow it.
    sketch = (ROOT/'firmware/RPG_POKET_2/RPG_POKET_2.ino').read_text()
    legacy_marker = '  if(lcdReady&&(dirty||'
    assert sketch.count(legacy_marker) == 1
    campaign = 'lcdReady&&(dirty||'+sketch.split(legacy_marker, 1)[1].split('{drawAt=now;', 1)[0][:-1]
    training_block = sketch.split('  if(coopIsOpen()){\n    sampleTouch();tickCoopTraining(now);\n', 1)[1].split('    delay(1);return;', 1)[0]
    assert training_block.count('    if(') == 1
    training = training_block.split('    if(', 1)[1].split('){drawAt=now;', 1)[0]
    source = f'''#define RPG_TEST_SAVE29
#define RPG_CONTROLLER_NO_MAIN
#define main priorPackage12CadenceMain
#include "{ROOT}/tests/title_controller.cpp"
#undef main
#if RPG_ENABLE_COOP_TRAINING
#include "{ROOT}/firmware/RPG_POKET_2/Coop/CoopRadioDevice.h"
#include "{ROOT}/firmware/RPG_POKET_2/CoopDevice.h"
#endif
static bool campaignDue(uint32_t now){{return {campaign};}}
static bool trainingDue(uint32_t now){{return {training};}}
int main(){{
 lcdReady=true;dirty=false;drawAt=100;combatFx=CombatFx{{}};menu=MenuState{{}};view=ViewState{{}};game=rpg::create(0,42);
 for(Page page:{{Page::CampSetup,Page::CampRest,Page::Ruins,Page::Home,Page::Battle}}){{view.page=page;assert(!campaignDue(219)&&campaignDue(220));}}
 view.page=Page::Clock;menu.clockValid=true;assert(!campaignDue(299)&&campaignDue(300));
 menu.clockValid=false;assert(!campaignDue(100000));menu.clockValid=true;menu.sessionStarted=true;game.eventStage=1;assert(!campaignDue(299)&&campaignDue(300));
 menu.clockValid=false;assert(!campaignDue(579)&&campaignDue(580));menu.clockValid=true;
 game.eventStage=0;view.page=Page::Settings;assert(!campaignDue(100000));dirty=true;assert(campaignDue(101));dirty=false;
 // Training is 120 ms regardless of the suspended campaign/clock state.
 for(Page page:{{Page::CoopTraining,Page::Clock,Page::Settings}}){{view.page=page;assert(!trainingDue(219)&&trainingDue(220));}}
 dirty=true;assert(trainingDue(101));dirty=false;
 view.page=Page::CampSetup;drawAt=0xfffffff0u;assert(!campaignDue(103)&&campaignDue(104));assert(!trainingDue(103)&&trainingDue(104));
 view.page=Page::Clock;assert(!campaignDue(183)&&campaignDue(184));
 lcdReady=false;dirty=true;assert(!campaignDue(100000)&&!trainingDue(100000));
}}
'''
    with tempfile.TemporaryDirectory(prefix='package12-cadence-') as folder:
        path = Path(folder)/'cadence.cpp'
        path.write_text(source)
        for flag in (0, 1):
            output = Path(folder)/('cadence-'+str(flag))
            subprocess.run([*compiler, '-std=c++17', '-O2', '-UNDEBUG',
                            '-DRPG_ENABLE_COOP_TRAINING='+str(flag), '-Itests/coop_fake',
                            str(path), 'firmware/RPG_POKET_2/CoopRuntime.cpp', '-o', str(output)],
                           cwd=ROOT, check=True, timeout=120)
            subprocess.run([str(output)], cwd=ROOT, check=True, timeout=60)
    print('PASS: exact production redraw predicates compiled OFF/ON; training 120 ms, campaign 120 ms, clock 200 ms, letter 480 ms, dirty/LCD guards and wrap; frozen extractor unambiguous')


def capture_wrapper_probes():
    spec = importlib.util.spec_from_file_location('package12_capture_probe', ROOT/'tools/capture_package12_coop.py')
    capture = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(capture)
    feature = capture.FEATURE_DEFINE
    checks = 0
    successful = [
        ['g++', '-O2', 'render.cpp', '-o', 'render'],
        ['g++', '-O1', '-fsanitize=address,undefined', 'render.cpp', '-o', 'render-san'],
        ['/usr/bin/g++', '-O2', 'render.cpp'],
        ['g++-14', feature, feature, '-O2', 'render.cpp'],
        ['g++', '-D', 'RPG_ENABLE_COOP_TRAINING=1', 'render.cpp'],
    ]
    for command in successful:
        with mock.patch.object(capture.subprocess, 'run') as intercepted:
            capture.strict_run(command, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0',
                                                UBSAN_OPTIONS='halt_on_error=0', LSAN_OPTIONS='exitcode=0'))
            intercepted.assert_called_once()
            args, kwargs = intercepted.call_args
            assert args[0].count(feature) == 1 and args[0][1] == feature
            assert kwargs['check'] is True
            for key, expected in capture.SANITIZER_ENV.items():
                assert kwargs['env'][key] == expected
            checks += 1
    with mock.patch.object(capture.subprocess, 'run') as intercepted:
        command = ['/tmp/coop-render-san', '/tmp/art.pak', '/tmp/screens']
        capture.strict_run(command)
        assert intercepted.call_args.args[0] == command
        checks += 1
    bad_flags = [
        ['-DRPG_ENABLE_COOP_TRAINING='+value] for value in ('0', '-1', '2', 'invalid', '')
    ] + [
        ['-DRPG_ENABLE_COOP_TRAINING'], ['-URPG_ENABLE_COOP_TRAINING'],
        ['-D', 'RPG_ENABLE_COOP_TRAINING=0'], ['-U', 'RPG_ENABLE_COOP_TRAINING'],
        ['-Wp,-DRPG_ENABLE_COOP_TRAINING=0'], [feature, '-DRPG_ENABLE_COOP_TRAINING=0'],
        ['@unexamined-compiler-flags'],
    ]
    for flags in bad_flags:
        with mock.patch.object(capture.subprocess, 'run') as intercepted:
            try:
                capture.strict_run(['g++', *flags, 'render.cpp'])
            except ValueError:
                pass
            else:
                raise AssertionError('Capture accepted conflicting/hidden feature flags: '+repr(flags))
            intercepted.assert_not_called()
            checks += 1
    print(f'PASS: {checks} offline capture-wrapper flag/env probes; normal and sanitizer compiles explicitly ON; OFF/invalid/hidden overrides rejected; no sanitizer runtime executed')


def main():
    compiler = shlex.split(os.environ.get('CXX', 'g++'))
    cases = [
        ('host-default-off', [], 0),
        ('arduino-default-on', ['-DARDUINO=10819'], 1),
        ('host-explicit-off', ['-DRPG_ENABLE_COOP_TRAINING=0'], 0),
        ('host-explicit-on', ['-DRPG_ENABLE_COOP_TRAINING=1'], 1),
        ('arduino-explicit-off', ['-DARDUINO=10819', '-DRPG_ENABLE_COOP_TRAINING=0'], 0),
        ('arduino-explicit-on', ['-DARDUINO=10819', '-DRPG_ENABLE_COOP_TRAINING=1'], 1),
    ]
    with tempfile.TemporaryDirectory(prefix='package12-config-') as folder:
        output = Path(folder)/'probe.o'
        for name, definitions, expected in cases:
            source = '#include "CoopConfig.h"\nstatic_assert(RPG_ENABLE_COOP_TRAINING == '+str(expected)+', "wrong coop default/override");\n'
            subprocess.run([*compiler, '-std=c++17', '-Werror', '-Ifirmware/RPG_POKET_2',
                            *definitions, '-x', 'c++', '-c', '-', '-o', str(output)],
                           input=source, text=True, cwd=ROOT, check=True, timeout=60)
            print('PASS: '+name)
        for platform in ([], ['-DARDUINO=10819']):
            for invalid in ('-1', '2', '99'):
                result = subprocess.run([*compiler, '-std=c++17', '-Ifirmware/RPG_POKET_2',
                                         *platform, '-DRPG_ENABLE_COOP_TRAINING='+invalid,
                                         '-x', 'c++', '-c', '-', '-o', str(output)],
                                        input='#include "CoopConfig.h"\n', text=True,
                                        capture_output=True, cwd=ROOT, timeout=60)
                if result.returncode == 0 or 'RPG_ENABLE_COOP_TRAINING must be 0 or 1' not in result.stderr:
                    raise AssertionError('Invalid flag was not rejected by the production header: '+invalid)
    print('PASS: 12 compiled config probes; production ARDUINO default ON, host default OFF, explicit 0/1 and invalid values')
    cadence_probes(compiler)
    capture_wrapper_probes()


if __name__ == '__main__':
    main()
