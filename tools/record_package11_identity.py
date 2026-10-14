#!/usr/bin/env python3
"""Record exact CI source identity only after aggregate, target build and byte gates."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
from verify_package11_release import verify
from test_package11_release import published_lineage
from check_package11_scope import assert_generated_inventory


def git(*args):
    return subprocess.check_output(['git',*args],text=True).strip()


def assert_generated_only(status):
    allowed = {'release/firmware.bin','release/artes.pak','release/manifest.json','release/SHA256SUMS'}
    assert all(line.startswith('?? ') and line[3:] in allowed for line in status.splitlines()), 'Unexpected source change: '+status


def main():
    assert git('rev-parse','HEAD') == os.environ['GITHUB_SHA']
    assert os.environ['GITHUB_REF'] == 'refs/heads/feat/package11-release'
    assert os.environ['GITHUB_EVENT_NAME'] == 'push' and os.environ['RPG_DETECT_LEAKS'] == '1'
    assert_generated_inventory(Path('.'), git('ls-files','--cached','-z').split('\0'))
    assert_generated_only(git('status','--porcelain','--untracked-files=all'))
    verify()
    lineage = published_lineage()
    summary=json.loads(Path('build/package11-validation/ci-'+os.environ['GITHUB_SHA']+'/summary.json').read_text())
    assert summary['all_passed'] and not summary['failed'] and summary['lsan'] == 'detect_leaks=1' and summary['tasks'] == 149
    aggregate_root=Path('build/package11-validation/ci-'+os.environ['GITHUB_SHA'])
    source_manifest=json.loads((aggregate_root/'source-manifest.json').read_text())
    assert source_manifest['sha256'] == summary['source_sha256']
    manifest_body={key:value for key,value in source_manifest.items() if key != 'sha256'}
    assert hashlib.sha256(json.dumps(manifest_body,sort_keys=True).encode()).hexdigest() == source_manifest['sha256']
    assert set(source_manifest['files']) == set(git('ls-files','--cached','-z').split('\0')) - {''}
    for name,digest in source_manifest['files'].items():
        assert hashlib.sha256(Path(name).read_bytes()).hexdigest() == digest, 'Aggregate input changed: '+name
    checkpoints=json.loads((aggregate_root/'checkpoints.json').read_text())
    assert len(checkpoints) == 149
    for name,entry in checkpoints.items():
        assert entry['status'] == 'passed' and hashlib.sha256((aggregate_root/(name+'.log')).read_bytes()).hexdigest() == entry['log_sha256'], 'Aggregate log changed: '+name
    names=['firmware.bin','artes.pak','manifest.json']
    files={n:dict(bytes=(Path('release')/n).stat().st_size,sha256=hashlib.sha256((Path('release')/n).read_bytes()).hexdigest()) for n in names}
    narrative=json.loads(Path('build/narrative8-render/summary.json').read_text())
    assert narrative['rendered_frames'] == 3481 and narrative['unique_previews'] == 671
    clock_visibility=json.loads(Path('build/clock-cycle7/visibility-preview/capture-manifest.json').read_text())
    assert clock_visibility['Relogio-pulso-manha-velocidade-real-12s.gif']['duration_ms'] == 12000
    pvp=json.loads(Path('build/pvp-presentation/capture-manifest.json').read_text())
    assert sum(name.endswith('.gif') for name in pvp) == 12
    for directory in ('screens','sanitized'):
        report_text=Path('build/pvp-presentation',directory,'render-results.txt').read_text()
        assert '15030 total renders' in report_text and '2048 independent identity matrix cases' in report_text and '5376 action phase cases' in report_text
    assert len(list(Path('build/pvp-presentation/screens').glob('*.ppm'))) == 596
    assert len(list(Path('build/pvp-presentation/sanitized').glob('*.ppm'))) == 596
    bulk_capture=Path('build/bulk-shop/screens/compras-vendas-em-lote.png')
    assert bulk_capture.is_file() and len(list(Path('build/bulk-shop/screens').glob('*.ppm'))) == 14
    bulk_frames={path.name:hashlib.sha256(path.read_bytes()).hexdigest() for path in Path('build/bulk-shop/screens').glob('*.ppm')}
    sanitized_frames={path.name:hashlib.sha256(path.read_bytes()).hexdigest() for path in Path('build/bulk-shop/sanitized').glob('*.ppm')}
    assert bulk_frames == sanitized_frames, 'Optimized and sanitizer commerce pixels differ'
    bulk={'frames_sha256':bulk_frames,'optimized_sanitized_identical':True,'preview_sha256':hashlib.sha256(bulk_capture.read_bytes()).hexdigest(),'frames':14,'ration_limit':10,'historical_reader_limit':9,'firmware_downgrade_supported':False}
    report=dict(bulk=bulk,pvp=pvp,pvp_protocol_version=5,pvp_packet_bytes=52,current_save30_journal=True,clock_visibility=clock_visibility,narrative=narrative,source_commit=git('rev-parse','HEAD'),source_tree=git('rev-parse','HEAD^{tree}'),branch=os.environ['GITHUB_REF'],artifacts=files,esp32_core='3.3.12',leak_detection=True,aggregate=summary,published_baselines=lineage,save_writes=30,save_reads=[29,30],save_bytes=160,ration_limit=10,historical_reader_ration_limit=9,firmware_downgrade_supported=False,reset_epoch=1,new_reset=False,physical_validation=False,published=False)
    Path('build/package11-source-identity.json').write_text(json.dumps(report,indent=2)+'\n')
    Path('release/SHA256SUMS').write_text(''.join(f"{files[n]['sha256']}  {n}\n" for n in names))
    p=Path('build/package11-scope-budget.json');budget=json.loads(p.read_text());budget.update(candidate_firmware_bytes=files['firmware.bin']['bytes'],candidate_ota_margin_bytes=0x300000-files['firmware.bin']['bytes'],target_compile_verified=True);p.write_text(json.dumps(budget,indent=2)+'\n')
    assert_generated_inventory(Path('.'), git('ls-files','--cached','-z').split('\0'))
    assert_generated_only(git('status','--porcelain','--untracked-files=all'))


if __name__=='__main__':
    main()
