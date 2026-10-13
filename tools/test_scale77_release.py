#!/usr/bin/env python3
"""Pinned touch77 rejection and scale77-to-scale77 Save29 OTA regression on exact prepared/downloaded files."""
from pathlib import Path
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
import test_release77_ota as headers
from verify_scale77_release import verify, VERSION, BUILD, PACKAGE, MINIMUM_PSRAM

ROOT = Path(__file__).resolve().parents[1]
OLD_COMMIT = '91c64a364a7c1c697b9cd91b7e72027fa03f2dee'
OLD_TREE = '1099a6cb0713f22ed805a8ca28f3978f77734727'
SCENES_COMMIT = 'dd626414b62bf6f88638ffe5d542196230c51061'
SCENES_TREE = 'd1d9972abe0e792bd2adbf51f39cd720d354a074'


def stage_persistence(destination, revision=None):
    pending = ['Persistence.h', 'ResetEpoch.h']
    copied = set()
    while pending:
        name = pending.pop()
        if name in copied:
            continue
        assert '..' not in Path(name).parts
        data = headers.source_bytes('firmware/RPG_POKET_2/' + name, revision)
        out = destination / name
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_bytes(data)
        assert out.read_bytes() == data
        copied.add(name)
        pending.extend(re.findall(r'^#include\s+"([^"]+)"', data.decode(), re.M))
    return copied


def synthetic_fixture(folder):
    """Explicit fixture only; never used silently when compilation is unavailable."""
    import hashlib
    import json
    import struct
    fw = bytearray(10000)
    fw[0] = 0xe9
    struct.pack_into('<H', fw, 12, 9)
    marker = VERSION.encode() + b'\0'
    fw[32:32+len(marker)] = marker
    art = (ROOT / 'cartao/RPGPOKET/artes.pak').read_bytes()
    m = dict(schema=2, version=VERSION, build=BUILD, package_name=PACKAGE,
             board='Waveshare-29667', partition='app3M_fat9M_16MB', minimum_psram_bytes=MINIMUM_PSRAM,
             installation='usb-first-then-confirmed-device-ota', usb_required_before_save_format=29,
             usb_erase_flash='none', save_format=7, writes_save_format=29, reads_save_formats=[29],
             save_reset_epoch=1, reset_game_slots_once=[1,2,3], preserves=['wifi-settings','sd-art-packages'])
    base = f'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v{VERSION}/'
    folder.mkdir(parents=True, exist_ok=True)
    for key, name, data in (('firmware','firmware.bin',fw),('art','artes.pak',art)):
        (folder/name).write_bytes(data)
        m[key] = dict(url=base+name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
    m['art'].update(assets=666, payload_crc32='9b4bcfc0')
    (folder/'manifest.json').write_text(json.dumps(m,indent=2)+'\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('release_dir', nargs='?', type=Path, default=ROOT/'release')
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--synthetic-fixture', action='store_true', help='Host fixture only, not compiled/released binary qualification')
    args = parser.parse_args()
    compiler = os.environ.get('CXX','g++')
    assert shutil.which(compiler), 'C++ compiler unavailable; no OTA gate ran'
    tree = subprocess.check_output(['git','rev-parse',OLD_COMMIT+'^{tree}'],cwd=ROOT,text=True).strip()
    assert tree == OLD_TREE, 'Published touch77 source identity unavailable'
    assert subprocess.check_output(['git','rev-parse',SCENES_COMMIT+'^{tree}'],cwd=ROOT,text=True).strip() == SCENES_TREE, 'Published scenes77 source identity unavailable'
    with tempfile.TemporaryDirectory(prefix='scale77-release-') as temp:
        temp = Path(temp)
        folder = args.release_dir.resolve()
        if args.synthetic_fixture:
            folder = temp/'synthetic'
            synthetic_fixture(folder)
            print('HOST FIXTURE MODE: synthetic firmware, real approved art; not firmware compilation or release validation.',flush=True)
        verify(folder, ROOT)
        preservation = temp/'scenes77-save29'
        preservation.mkdir()
        for label, revision in (('old-touch77',OLD_COMMIT),('published-scenes77',SCENES_COMMIT),('new-scale77',None)):
            staged = temp/label
            headers.stage_headers(staged, revision)
            if revision != OLD_COMMIT:
                closure = stage_persistence(staged, revision)
                # Scale/contrast is presentation-only: pin the whole persistence
                # and epoch include closure byte-for-byte to published scenes77.
                for name in closure:
                    assert (staged/name).read_bytes() == headers.source_bytes('firmware/RPG_POKET_2/'+name, SCENES_COMMIT), 'Unexpected persistence/reset change: '+name
            flags = ['-std=c++17','-O1','-g','-UNDEBUG','-Wno-deprecated-declarations']
            if revision == OLD_COMMIT:
                flags += ['-DRPG_TEST_OLD_TOUCH77']
            elif revision == SCENES_COMMIT:
                flags += ['-DRPG_TEST_OLD_SCENES77']
            if args.sanitizers:
                flags += ['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie']
            exe = temp/(label+'-adapter')
            subprocess.run([compiler,*flags,'-I',str(staged),'-I',str(staged/'fake'),str(ROOT/'tests/scale77_release.cpp'),str(staged/'fake/cJSON.c'),'-lcrypto','-o',str(exe)],check=True)
            env = dict(os.environ)
            if args.sanitizers:
                env.update(ASAN_OPTIONS='detect_leaks='+os.environ.get('RPG_DETECT_LEAKS','1'),UBSAN_OPTIONS='halt_on_error=1')
            subprocess.run([str(exe),str(folder),str(preservation)],check=True,env=env)
            print(f'PASS: {label} production header closure unchanged, source {revision or "working tree"}',flush=True)
    inputs = 'synthetic firmware/real-art fixture' if args.synthetic_fixture else 'exact prepared/downloaded release bytes'
    print(f'PASS: touch77 refusal; published scenes77-to-scale77 OTA; Save29/epoch1 bytes preserved; {inputs} hashed with real SHA256. No publication/device I/O.')


if __name__ == '__main__':
    main()
