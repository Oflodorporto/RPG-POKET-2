#!/usr/bin/env python3
"""Synthetic rejection probes for extra capture evidence, never renderer proof."""
from pathlib import Path
import shutil,sys,tempfile
from PIL import Image
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from check_coopfix77_invite_render import COUNTDOWN,SPECIAL,EXPECTED,verify
checks=0
def reject(fn):
 global checks
 try:fn()
 except (AssertionError,ValueError,FileNotFoundError):checks+=1
 else:raise AssertionError('Accepted invalid invitation capture evidence')
with tempfile.TemporaryDirectory(prefix='coopfix77-render-probe-') as d:
 root=Path(d);normal=root/'screens/invites';normal.mkdir(parents=True);san=root/'sanitized/invites';san.mkdir(parents=True)
 for name in EXPECTED:
  Image.new('RGB',(320,240)).save(normal/(name+'.ppm'))
  rows=[(0,172,COUNTDOWN[name])] if name in COUNTDOWN else [(0,20+i*10,text) for i,text in enumerate(SPECIAL[name])]
  if name=='invite-guest-expired':rows=[(112,81,'CONVITE EXPIRADO'),(64,99,'VOLTE A GUILDA E ENTRE NOVAMENTE'),(115,187,'VOLTAR A GUILDA'),(100,205,'TOQUE AQUI PARA SAIR')]
  (normal/(name+'-labels.tsv')).write_text(''.join(f'{x}\t{y}\t{text}\n' for x,y,text in rows))
 for path in normal.iterdir():shutil.copyfile(path,san/path.name)
 assert len(verify(root))==16;checks+=1
 for name in EXPECTED:
  file=san/(name+'.ppm');saved=file.read_bytes();file.unlink();reject(lambda:verify(root));file.write_bytes(saved)
 name='invite-host-30s';file=normal/(name+'.ppm');saved=file.read_bytes()
 Image.new('RGB',(320,240),'red').save(file);reject(lambda:verify(root));file.write_bytes(saved)
 Image.new('RGB',(320,241)).save(file);reject(lambda:verify(root));file.write_bytes(saved)
 label=normal/(name+'-labels.tsv');saved=label.read_bytes()
 for payload in ('0\t172\tACEITE EM 29s\n','0\t171\tACEITE EM 30s\n','319\t172\tACEITE EM 30s\n'):
  label.write_text(payload);reject(lambda:verify(root))
 label.write_bytes(saved)
 # Disable cross-mode parity only for this synthetic mutation so rejection
 # specifically proves full glyph height, not an unrelated label mismatch.
 label.write_bytes(saved+b'0\t239\tX\n');reject(lambda:verify(root,False));label.write_bytes(saved)
 file=san/'invite-guest-expired-labels.tsv';saved=file.read_bytes();file.write_bytes(saved+b'0\t0\textra\n');reject(lambda:verify(root));file.write_bytes(saved)
 file=normal/'invite-starting-host-labels.tsv';saved=file.read_bytes();file.write_bytes(saved+b'0\t0\tAGUARDE ACEITE\n');reject(lambda:verify(root));file.write_bytes(saved)
 extra=normal/'unapproved.ppm';Image.new('RGB',(320,240)).save(extra);reject(lambda:verify(root));extra.unlink()
print(f'PASS: {checks} synthetic capture-count/geometry/label/parity negative probes; not actual renderer evidence.')
