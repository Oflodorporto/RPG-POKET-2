#!/usr/bin/env python3
"""Replay independent Save29/30 codec, journal and production-NVS fault review.

Only tracked sources and exact pinned Git history are inputs. Host adapters do
not establish physical NVS/brownout behavior. No network or device writes.
"""
from pathlib import Path
import argparse, os, re, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
BASE='dd626414b62bf6f88638ffe5d542196230c51061'
TREE='d1d9972abe0e792bd2adbf51f39cd720d354a074'
SOURCE=ROOT/'tests/package6_save_review'

def stage(out, revision=None):
    pending=['Persistence.h','NvsPersistence.h','ResetEpoch.h'];done=set()
    while pending:
        name=pending.pop()
        if name in done: continue
        assert not Path(name).is_absolute() and '..' not in Path(name).parts
        path='firmware/RPG_POKET_2/'+name
        data=subprocess.check_output(['git','show',revision+':'+path],cwd=ROOT) if revision else (ROOT/path).read_bytes()
        (out/name).write_bytes(data);done.add(name)
        pending+=re.findall(r'^#include\s+"([^"]+)"',data.decode(),re.M)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--sanitizers',action='store_true');args=parser.parse_args()
    assert subprocess.check_output(['git','rev-parse',BASE+'^{tree}'],cwd=ROOT,text=True).strip()==TREE
    flags=['-std=c++17','-O1','-g','-UNDEBUG']
    if args.sanitizers: flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie']
    env=dict(os.environ,ASAN_OPTIONS='detect_leaks='+os.environ.get('RPG_DETECT_LEAKS','1'),UBSAN_OPTIONS='halt_on_error=1')
    with tempfile.TemporaryDirectory(prefix='package6-save-review-') as d:
        d=Path(d);old=d/'old';new=d/'new';old.mkdir();new.mkdir();stage(old,BASE);stage(new)
        for name in ('generate29','migrate30','nvs_migrate30','rollback29','nvs_rollback29'):
            header=old if name in ('generate29','rollback29','nvs_rollback29') else new
            exe=d/name
            subprocess.run([os.environ.get('CXX','g++'),*flags,'-I',str(header),str(SOURCE/(name+'.cpp')),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],cwd=d,env=env,check=True)
    print('PASS: tracked independent Save29/30 migration and journal/NVS fault review; no device I/O.')
if __name__=='__main__':main()
