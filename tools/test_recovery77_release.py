#!/usr/bin/env python3
"""Bounded final SD-release qualification: reuse unchanged7.6/final guards,
then test the exact published ota77 installer with real SHA-256. No device I/O.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
sys.dont_write_bytecode=True
import test_release77_ota as transition

transition.FINAL_VERSION='2026.10.13-recovery77'
transition.FINAL_BUILD=2026101303
# The historical driver stays intact; this wrapper pins a different final identity.
transition.main()
root=Path(__file__).resolve().parents[1]
# Local reviewed equivalent is permitted only with the exact published root tree.
expected_tree='29f34d992151208855fbc92b406ec34b85b7a4cf'
revision=None
for candidate in ('93b1b4ed75beb96bdf5ebad0642d39a2048a4326','af06ae548d1c0ce92f5f889028283f8e41669155'):
    p=subprocess.run(['git','rev-parse',candidate+'^{tree}'],cwd=root,capture_output=True,text=True)
    if p.returncode==0 and p.stdout.strip()==expected_tree:
        revision=candidate;break
assert revision, 'Exact published ota77 source tree unavailable'
args=sys.argv[1:]
folder=next((Path(a).resolve() for a in args if not a.startswith('--')),root/'release')
sanitizers='--sanitizers' in args
with tempfile.TemporaryDirectory(prefix='ota77-published-') as temp:
    temp=Path(temp);staged=temp/'published';transition.stage_headers(staged,revision)
    if '--synthetic-fixture' in args:
        folder=temp/'synthetic';folder.mkdir()
        transition.fixture(folder,transition.FINAL_VERSION,transition.FINAL_BUILD,transition.source_identity(transition.FINAL_VERSION,transition.FINAL_BUILD))
    transition.verify_files(folder,transition.FINAL_VERSION,transition.FINAL_BUILD,transition.source_identity(transition.FINAL_VERSION,transition.FINAL_BUILD))
    flags=['-std=c++17','-O1','-g','-UNDEBUG','-Wno-deprecated-declarations']
    if sanitizers:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie']
    executable=temp/'published77_upgrade'
    subprocess.run([os.environ.get('CXX','g++'),*flags,'-I',str(staged),'-I',str(staged/'fake'),str(root/'tests/recovery77_upgrade.cpp'),str(staged/'fake/cJSON.c'),'-lcrypto','-o',str(executable)],check=True)
    env=dict(os.environ)
    if sanitizers:env.update(ASAN_OPTIONS='detect_leaks='+os.environ.get('RPG_DETECT_LEAKS','0'),UBSAN_OPTIONS='halt_on_error=1')
    subprocess.run([str(executable),str(folder)],check=True,env=env)
    print('PASS: old ota77 headers unchanged, exact tree '+expected_tree+'; OpenSSL real SHA-256, simulated HTTP/SD/OTA, no physical claim.',flush=True)
