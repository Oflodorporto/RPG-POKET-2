#!/usr/bin/env python3
"""Verify new nested invitation captures without replacing the frozen 58-frame gate."""
from pathlib import Path
import argparse,hashlib,json,os
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1]
COUNTDOWN={f'invite-{side}-{seconds}s': ('ACEITE EM ' if side=='host' else 'PRAZO APROX.: ')+str(seconds)+'s'
           for side in ('host','guest') for seconds in (30,10,1,0)}
SPECIAL={
 'invite-guest-initial':['CONECTANDO','AGUARDE RESPOSTA'],
 'invite-starting-host':['SINCRONIZANDO','AGUARDE AS PLACAS'],
 'invite-starting-guest':['SINCRONIZANDO','AGUARDE AS PLACAS'],
 'invite-host-expired':['CONVITE EXPIRADO / AGUARDE NOVO PEDIDO'],
 'invite-guest-expired':['CONVITE EXPIRADO','VOLTE A GUILDA E ENTRE NOVAMENTE','VOLTAR A GUILDA','TOQUE AQUI PARA SAIR'],
 'invite-guest-declined':['PEDIDO RECUSADO'],
 'invite-guest-busy':['SALA OCUPADA'],
 'invite-guest-technical-timeout':['TEMPO DE CONEXAO ESGOTADO'],
}
EXPECTED=set(COUNTDOWN)|set(SPECIAL)
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def verify(folder,sanitized=True):
 expected={name+'.ppm' for name in EXPECTED}
 normal=folder/'screens/invites';san=folder/'sanitized/invites'
 assert {p.name for p in normal.glob('*.ppm')}==expected,'Missing/unexpected invitation snapshot'
 assert {p.name for p in normal.glob('*-labels.tsv')}=={name+'-labels.tsv' for name in EXPECTED},'Missing invitation labels'
 if sanitized:
  assert {p.name for p in san.glob('*.ppm')}==expected,'Missing/unexpected sanitized invitation snapshot'
  assert {p.name for p in san.glob('*-labels.tsv')}=={name+'-labels.tsv' for name in EXPECTED},'Missing sanitized labels'
 captures={}
 for name in sorted(EXPECTED):
  ppm=normal/(name+'.ppm');labels=normal/(name+'-labels.tsv')
  im=Image.open(ppm);im.load();assert im.size==(320,240),'Invitation geometry changed'
  rows=[]
  for line in labels.read_text().splitlines():
   x,y,text=line.split('\t',2);x=int(x);y=int(y)
   assert 0<=x<320 and 0<=y and y+8<=240 and x+len(text)*6<=320,(name,line)
   rows.append((x,y,text))
  texts=[text for _,_,text in rows]
  if name in COUNTDOWN:
   assert any(y==172 and text==COUNTDOWN[name] for _,y,text in rows),(name,COUNTDOWN[name])
  else:
   for required in SPECIAL[name]:assert required in texts,(name,required)
  if name.startswith('invite-starting-') or name=='invite-guest-initial':
   assert 'AGUARDE ACEITE' not in texts
   assert not any('ACEITE EM ' in text or 'PRAZO APROX.:' in text for text in texts)
  if name=='invite-guest-expired':
   for exact in [(64,99,'VOLTE A GUILDA E ENTRE NOVAMENTE'),(115,187,'VOLTAR A GUILDA'),(100,205,'TOQUE AQUI PARA SAIR')]:assert exact in rows
  if sanitized:
   assert ppm.read_bytes()==(san/ppm.name).read_bytes(),'Invitation normal/sanitizer pixel mismatch: '+name
   assert labels.read_bytes()==(san/labels.name).read_bytes(),'Invitation normal/sanitizer label mismatch: '+name
  captures[name]={'ppm_sha256':digest(ppm),'labels_sha256':digest(labels),'labels':len(rows),'size':list(im.size)}
 return captures

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--root',type=Path,default=ROOT/'build/coop-training-render')
 parser.add_argument('--normal-only',action='store_true')
 args=parser.parse_args()
 assert not (args.normal_only and os.environ.get('GITHUB_ACTIONS')=='true'),'CI requires strict sanitized render parity'
 if not args.normal_only:assert os.environ.get('RPG_DETECT_LEAKS','1')=='1','LSan1 is mandatory'
 output=args.root/'invites';output.mkdir(parents=True,exist_ok=True)
 report={'passed':False,'sanitized':not args.normal_only,'physical_validation':False}
 result=output/'manifest.json';result.write_text(json.dumps(report,indent=2)+'\n')
 captures=verify(args.root,not args.normal_only)
 sheet=Image.new('RGB',(4*640,4*504),(10,23,26));draw=ImageDraw.Draw(sheet)
 for i,name in enumerate(sorted(captures)):
  im=Image.open(args.root/'screens/invites'/(name+'.ppm'));im.load();png=output/(name+'.png');im.save(png)
  captures[name]['png_sha256']=digest(png)
  x=i%4*640;y=i//4*504;sheet.paste(im.resize((640,480),Image.Resampling.NEAREST),(x,y+24));draw.text((x+8,y+6),name,fill='white')
 sheet.save(output/'contactsheet.png')
 sources=['firmware/RPG_POKET_2/Coop/CoopView.h','firmware/RPG_POKET_2/Coop/CoopViewModel.h','firmware/RPG_POKET_2/Coop/CoopPortraitBridge.h','tests/coop_training_render.cpp','tools/check_coopfix77_invite_render.py']
 report.update(passed=True,captures=captures,pixel_and_label_parity=not args.normal_only,
               sources_sha256={name:digest(ROOT/name) for name in sources},contactsheet_sha256=digest(output/'contactsheet.png'))
 result.write_text(json.dumps(report,indent=2)+'\n')
 print('PASS: 16 additional invitation captures, geometry/labels and '+('normal-only previews; sanitizer parity NOT RUN' if args.normal_only else 'normal/sanitizer pixel+label parity')+'. No physical validation implied.')
if __name__=='__main__':main()
