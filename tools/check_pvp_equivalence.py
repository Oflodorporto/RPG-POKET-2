#!/usr/bin/env python3
"""Recheck the deterministic 245760-step trace against the independently reviewed reliability-v2 trace.

The original two-version comparison was performed on 2026-10-14. It compared
reliability-v2 (patch 155d6699...) and combined (patch ac9e3725...) with precisely
this tests/pvp_review_equivalence.cpp. SHA256 below binds all printed commands
and non-cosmetic combat states, and excludes only newly added visual metadata.
This sampled equivalence does not prove universal correctness or physical RF.
"""
import hashlib, subprocess, sys
EXPECTED='931eb3ea7f882d465c6a6f04518140a78a90fc7450d2e01a811a08a12e2acfb0'
assert len(sys.argv)==2
with subprocess.Popen([sys.argv[1]],stdout=subprocess.PIPE) as process:
    digest=hashlib.sha256()
    for chunk in iter(lambda:process.stdout.read(1024*1024),b''):digest.update(chunk)
    assert process.wait()==0, 'Equivalence trace process failed'
assert digest.hexdigest()==EXPECTED, 'Reviewed combat/command equivalence trace changed: '+digest.hexdigest()
print('PASS: 245760 deterministic paired transitions match reviewed reliability-v2 trace '+EXPECTED)
