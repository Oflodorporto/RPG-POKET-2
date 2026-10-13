#!/usr/bin/env python3
"""Bounded final dungeon-release qualification: reuse unchanged7.6/final guards,
then test the exact published recovery77 installer with real SHA-256. No device I/O.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
sys.dont_write_bytecode=True
import test_release77_ota as transition

transition.FINAL_VERSION='2026.10.13-dungeon77'
transition.FINAL_BUILD=2026101304
# The historical driver stays intact; this wrapper pins a different final identity.
transition.main()
root=Path(__file__).resolve().parents[1]
# Local reviewed equivalent is permitted only with the exact published root tree.
expected_tree='98a735c86ca856b6250f32675b6d4b0753b9b9b3'
revision=None
for candidate in ('cf0639ebf7e1dc729a026d09a8ef27f1e7dbc9ae','6e42f8571d943c967738aae459f14a78344dd1a6'):
    p=subprocess.run(['git','rev-parse',candidate+'^{tree}'],cwd=root,capture_output=True,text=True)
    if p.returncode==0 and p.stdout.strip()==expected_tree:
        revision=candidate;break
assert revision, 'Exact published recovery77 source tree unavailable'
args=sys.argv[1:]
folder=next((Path(a).resolve() for a in args if not a.startswith('--')),root/'release')
sanitizers='--sanitizers' in args
with tempfile.TemporaryDirectory(prefix='recovery77-published-') as temp:
    temp=Path(temp);staged=temp/'published';transition.stage_headers(staged,revision)
    if '--synthetic-fixture' in args:
        folder=temp/'synthetic';folder.mkdir()
        transition.fixture(folder,transition.FINAL_VERSION,transition.FINAL_BUILD,transition.source_identity(transition.FINAL_VERSION,transition.FINAL_BUILD))
    transition.verify_files(folder,transition.FINAL_VERSION,transition.FINAL_BUILD,transition.source_identity(transition.FINAL_VERSION,transition.FINAL_BUILD))
    flags=['-std=c++17','-O1','-g','-UNDEBUG','-Wno-deprecated-declarations']
    if sanitizers:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie']
    executable=temp/'published77_upgrade'
    subprocess.run([os.environ.get('CXX','g++'),*flags,'-I',str(staged),'-I',str(staged/'fake'),str(root/'tests/dungeon77_upgrade.cpp'),str(staged/'fake/cJSON.c'),'-lcrypto','-o',str(executable)],check=True)
    env=dict(os.environ)
    if sanitizers:env.update(ASAN_OPTIONS='detect_leaks='+os.environ.get('RPG_DETECT_LEAKS','0'),UBSAN_OPTIONS='halt_on_error=1')
    subprocess.run([str(executable),str(folder)],check=True,env=env)
    print('PASS: old recovery77 headers unchanged, exact tree '+expected_tree+'; OpenSSL real SHA-256, simulated HTTP/SD/OTA, no physical claim.',flush=True)
