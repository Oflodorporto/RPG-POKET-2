#!/usr/bin/env python3
from pathlib import Path
import shutil,zipfile,hashlib,json
ROOT=Path(__file__).resolve().parents[2]; OUT=ROOT/'build/sd_diagnostic'; OUT.mkdir(parents=True,exist_ok=True)
pkg=OUT/'RPG_POKET_Diagnostico_SD';sketch=pkg/'SD_Diagnostico'
shutil.copytree(ROOT/'tools/sd_diagnostic/SD_Diagnostico',sketch,dirs_exist_ok=True)
shutil.copytree(ROOT/'firmware/RPG_POKET_2/src/GFX',sketch/'src/GFX',dirs_exist_ok=True)
for name in ['README_PTBR.txt','DEPENDENCIAS.txt']: shutil.copyfile(ROOT/'tools/sd_diagnostic'/name,pkg/name)
files={str(p.relative_to(pkg)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(pkg.rglob('*')) if p.is_file() and p.name!='SHA256SUMS.json'}
(pkg/'SHA256SUMS.json').write_text(json.dumps(files,indent=2)+'\n')
with zipfile.ZipFile(OUT/'RPG_POKET_Diagnostico_SD_v2.zip','w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(pkg.rglob('*')):
  if p.is_file(): z.write(p,str(p.relative_to(OUT)))
print(sketch);print(OUT/'RPG_POKET_Diagnostico_SD_v2.zip')
