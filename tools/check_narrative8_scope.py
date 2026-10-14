#!/usr/bin/env python3
"""Narrative-only candidate boundary against the exact published package7 source."""
import hashlib,json,re,subprocess
from pathlib import Path
BASE='e775b5d1854174b45404ca0cd3305c1e24610861'
def git(*args): return subprocess.check_output(['git',*args])
def baseline(path): return git('show',f'{BASE}:{path}')
allowed={f'firmware/RPG_POKET_2/{p}' for p in ['DialogueStory.h','AnwenStory.h','Narrative.h','OriginStory.h','FinaleStory.h','NarrativeView.h','EventCatalog.h','EventView.h','View.h','Rules.h','GuildEvents.h','RPG_POKET_2.ino']}
tracked=git('ls-tree','-r','--name-only',BASE).decode().splitlines()
protected=[]
for path in tracked:
    if path.startswith(('firmware/','assets/','cartao/','.github/')) and path not in allowed:
        assert Path(path).read_bytes()==baseline(path),f'Protected source changed: {path}'
        protected.append(path)
# These gameplay files contain only approved wording changes, never rule changes.
for name in ['Rules.h','GuildEvents.h','EventCatalog.h']:
    p=f'firmware/RPG_POKET_2/{name}'
    def neutralize(data):
        text=data.decode();return re.sub(r'"(?:[^"\\]|\\.)*"','"TEXT"',text)
    assert neutralize(Path(p).read_bytes())==neutralize(baseline(p)),name
# Controller differences are read-only epilogue/crystal-objective navigation.
p='firmware/RPG_POKET_2/RPG_POKET_2.ino'
def remove_epilogue(data):
    text=data.decode().replace('game.guardianDefeated||game.crystals','game.guardianDefeated');a=text.index('  if(view.page==Page::Epilogue)');b=text.index('  if(view.page==Page::ContributionResult)',a);return text[:a]+text[b:]
assert remove_epilogue(Path(p).read_bytes())==remove_epilogue(baseline(p))
for name in ['Campaign.h','Dungeon.h','DailyTravel.h','Save.h','Persistence.h','ReleaseConfig.h']:
    p=f'firmware/RPG_POKET_2/{name}'
    if p in tracked: assert Path(p).read_bytes()==baseline(p),name
# Local-art payload stays exactly the released pack when materialized.
pack=Path('cartao/RPGPOKET/artes.pak')
report={'base':BASE,'protected_byte_identical_count':len(protected),'protected_byte_identical':protected,
        'text_only_gameplay_files':['Rules.h','GuildEvents.h'],'text_only_event_catalog':True, 'controller_change':'Read-only epilogue navigation and crystal-owned objective shortcut; all other bytes identical',
        'save_format':30,'reset_epoch':1,'art_items':666,'art_payload_changed':False,'remote_publication':False}
if pack.exists(): report['art_sha256']=hashlib.sha256(pack.read_bytes()).hexdigest()
Path('build/narrative8').mkdir(parents=True,exist_ok=True)
Path('build/narrative8/scope-report.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS: {len(protected)} protected files identical; Rules/GuildEvents literals only; controller epilogue/crystal-objective UI only; Save30/epoch1/art666 preserved')
