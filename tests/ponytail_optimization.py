#!/usr/bin/env python3
"""Optimization and wrapper-bypass rejection probes; no qualification is run."""
if not __debug__:
    raise RuntimeError("Qualification requires assertions; optimized Python is forbidden")
from pathlib import Path
import os
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]
GATES=(
 'tools/test_wifi_radio_test.py',
 'tools/check_ponytail_scope.py','tools/check_ponytail_release_gate.py',
 'tools/check_ponytail_qualified_artifact.py',
 'tools/record_ponytail_identity.py','tools/release_ponytail_manifest.py',
 'tools/verify_ponytail_release.py','tools/test_wifitest77_local.py',
 'tools/test_ponytail_release.py','tests/ponytail_qualification_config.py',
 'tests/ponytail_scope.py','tests/ponytail_build_evidence.py',
 'tests/ponytail_release_gate.py','tests/ponytail_qualified_artifact.py',
 'tests/ponytail_release_verifier.py',
)
checks=0
for path in GATES:
 for optimization in ('-O','-OO'):
  result=subprocess.run([sys.executable,optimization,path,'--help'],cwd=ROOT,capture_output=True,text=True,env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'})
  assert result.returncode!=0 and 'optimized Python is forbidden' in result.stderr,(path,optimization,result.stdout,result.stderr)
  checks+=1
 result=subprocess.run([sys.executable,path,'--help'],cwd=ROOT,capture_output=True,text=True,env={**os.environ,'PYTHONOPTIMIZE':'1','PYTHONDONTWRITEBYTECODE':'1'})
 assert result.returncode!=0 and 'optimized Python is forbidden' in result.stderr,path
 checks+=1
for args,env in ((['--focused'],{}),(['--skip'],{}),([] ,{'RPG_DETECT_LEAKS':'0'}),([] ,{'PYTHONOPTIMIZE':'1'})):
 result=subprocess.run(['bash','tools/test_ponytail.sh',*args],cwd=ROOT,capture_output=True,text=True,env={**os.environ,**env,'PYTHONDONTWRITEBYTECODE':'1'})
 assert result.returncode!=0 and not result.stdout and not result.stderr,(args,env,result.stdout,result.stderr)
 checks+=1
print(f'PASS: {checks} optimized-Python, focused/skip wrapper and disabled-LSan refusal probes. No tests/builds were qualified.')
