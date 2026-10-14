#!/usr/bin/env python3
"""Record CI source/artifact identity after all real-byte qualification gates."""
from pathlib import Path
import hashlib,json,os,subprocess

def main():
    git=lambda *args:subprocess.check_output(['git',*args],text=True).strip()
    assert git('rev-parse','HEAD')==os.environ['GITHUB_SHA']
    assert os.environ['GITHUB_REF']=='refs/heads/feat/package6-release'
    assert os.environ['RPG_DETECT_LEAKS']=='1'
    allowed={'release/firmware.bin','release/artes.pak','release/manifest.json','release/SHA256SUMS'}
    status=git('status','--porcelain','--untracked-files=all')
    assert all(line.startswith('?? ') and line[3:] in allowed for line in status.splitlines()),'Unexpected source change: '+status
    names=['firmware.bin','artes.pak','manifest.json']
    files={n:dict(bytes=(Path('release')/n).stat().st_size,sha256=hashlib.sha256((Path('release')/n).read_bytes()).hexdigest()) for n in names}
    report=dict(source_commit=git('rev-parse','HEAD'),source_tree=git('rev-parse','HEAD^{tree}'),branch=os.environ['GITHUB_REF'],artifacts=files,esp32_core='3.3.12',leak_detection=True,save_writes=30,save_reads=[29,30],reset_epoch=1,new_reset=False,physical_validation=False,published=False)
    Path('build/package6-source-identity.json').write_text(json.dumps(report,indent=2)+'\n')
    Path('release/SHA256SUMS').write_text(''.join(f"{files[n]['sha256']}  {n}\n" for n in names))
    p=Path('build/package6-scope-budget.json');budget=json.loads(p.read_text());budget.update(candidate_firmware_bytes=files['firmware.bin']['bytes'],candidate_ota_margin_bytes=0x300000-files['firmware.bin']['bytes'],target_compile_verified=True);p.write_text(json.dumps(budget,indent=2)+'\n')
if __name__=='__main__':main()
