#!/usr/bin/env python3
"""Record exact CI source identity only after aggregate, target build and byte gates."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
from verify_package7_release import verify
from test_package7_release import published_lineage
from check_package7_scope import assert_generated_inventory


def git(*args):
    return subprocess.check_output(['git',*args],text=True).strip()


def assert_generated_only(status):
    allowed = {'release/firmware.bin','release/artes.pak','release/manifest.json','release/SHA256SUMS'}
    assert all(line.startswith('?? ') and line[3:] in allowed for line in status.splitlines()), 'Unexpected source change: '+status


def main():
    assert git('rev-parse','HEAD') == os.environ['GITHUB_SHA']
    assert os.environ['GITHUB_REF'] == 'refs/heads/feat/package7-release'
    assert os.environ['GITHUB_EVENT_NAME'] == 'push' and os.environ['RPG_DETECT_LEAKS'] == '1'
    assert_generated_inventory(Path('.'), git('ls-files','--cached','-z').split('\0'))
    assert_generated_only(git('status','--porcelain','--untracked-files=all'))
    verify()
    lineage = published_lineage()
    summary=json.loads(Path('build/package7-validation/ci-'+os.environ['GITHUB_SHA']+'/summary.json').read_text())
    assert summary['all_passed'] and not summary['failed'] and summary['lsan'] == 'detect_leaks=1' and summary['tasks'] == 127
    names=['firmware.bin','artes.pak','manifest.json']
    files={n:dict(bytes=(Path('release')/n).stat().st_size,sha256=hashlib.sha256((Path('release')/n).read_bytes()).hexdigest()) for n in names}
    report=dict(source_commit=git('rev-parse','HEAD'),source_tree=git('rev-parse','HEAD^{tree}'),branch=os.environ['GITHUB_REF'],artifacts=files,esp32_core='3.3.12',leak_detection=True,aggregate=summary,published_baselines=lineage,save_writes=30,save_reads=[29,30],reset_epoch=1,new_reset=False,physical_validation=False,published=False)
    Path('build/package7-source-identity.json').write_text(json.dumps(report,indent=2)+'\n')
    Path('release/SHA256SUMS').write_text(''.join(f"{files[n]['sha256']}  {n}\n" for n in names))
    p=Path('build/package7-scope-budget.json');budget=json.loads(p.read_text());budget.update(candidate_firmware_bytes=files['firmware.bin']['bytes'],candidate_ota_margin_bytes=0x300000-files['firmware.bin']['bytes'],target_compile_verified=True);p.write_text(json.dumps(budget,indent=2)+'\n')
    assert_generated_inventory(Path('.'), git('ls-files','--cached','-z').split('\0'))
    assert_generated_only(git('status','--porcelain','--untracked-files=all'))


if __name__=='__main__':
    main()
