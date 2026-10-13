#!/usr/bin/env python3
"""Deterministic public synthetic data; never derived from user files."""
from pathlib import Path
import hashlib
SIZE=6661456
PATTERN=b'RPG-POKET-SD-DIAGNOSTIC-0123456789-ABCDEFGHIJKLMNOPQRSTUVWXYZ-abc\n'
def data(): return (PATTERN*((SIZE+len(PATTERN)-1)//len(PATTERN)))[:SIZE]
if __name__=='__main__':
 p=Path(__file__).parent/'fixtures'/'transfer_fixture.bin';p.parent.mkdir(exist_ok=True);p.write_bytes(data());print(len(data()),hashlib.sha256(data()).hexdigest());print('git',hashlib.sha1(b'blob '+str(SIZE).encode()+b'\0'+data()).hexdigest())
