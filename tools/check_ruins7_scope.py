"""Package7 local Ruins scope check; deliberately not a release qualification."""
from pathlib import Path
import subprocess,re,hashlib,gzip,struct
root=Path(__file__).resolve().parents[1];base='3f9d3221b99e6b4ba55dc270a2a1d0a9155ea27b'
def baseline(path):return subprocess.check_output(['git','show',base+':'+path],cwd=root)
for name in ['Rules.h','Save.h','Persistence.h','SaveVersion.h','FatigueJournal.h','Camp.h','Dungeon.h','DungeonGeneration.h','AssetCatalog.h','AssetStore.h','ReleaseVersion.h','UpdateDevice.h','DeviceSettings.h']:
 path='firmware/RPG_POKET_2/'+name;assert (root/path).read_bytes()==baseline(path),name
for path in (root/'cartao/RPGPOKET').glob('*'):
 if path.name=='artes.pak':continue
 rel=str(path.relative_to(root));assert path.read_bytes()==baseline(rel),rel
before=baseline('firmware/RPG_POKET_2/ScenicArt.h').decode();after=(root/'firmware/RPG_POKET_2/ScenicArt.h').read_text()
for name in ['shelter','menu','map']:
 for suffix in ['Palette','Pixels']:
  p=r'inline constexpr [^\n]+ '+name+suffix+r'\[[^\]]+\]=\{[^}]+\};';assert re.search(p,before)[0]==re.search(p,after)[0]
for name,expected in [('ruinsPalette',256),('ruinsPixels',240*320)]:
 p=r' '+name+r'\[[^\]]+\]=\{([^}]+)\};';assert len(re.search(p,after)[1].replace('\n','').split(','))==expected
header=(root/'firmware/RPG_POKET_2/Ruins7Art.h').read_text();flash=0
for typ,name,values in re.findall(r'inline constexpr (uint\d+_t) (\w+)\[[^\]]+\]=\{([^}]+)\};',header):flash+=len(values.replace('\n','').split(','))*(2 if typ=='uint16_t' else 1)
assert flash==18144,flash
sketch=(root/'firmware/RPG_POKET_2/RPG_POKET_2.ino').read_text();old=baseline('firmware/RPG_POKET_2/RPG_POKET_2.ino').decode()
normalized=sketch.replace('(view.page==Page::Ruins&&scenicUi::ruinsMission.contains(x,y))','(view.page==Page::Ruins&&hit(x,y,10,94,220,30))').replace('if(view.page==Page::Ruins){if(scenicUi::ruinsPager.contains(x,y)){view.cityPage=1-scenicUi::ruinsPage(view.cityPage);say("");return;}int choice=scenicUi::ruinsChoice(view.cityPage,x,y);','if(view.page==Page::Ruins){int choice=scenicUi::ruinsChoice(x,y);')
assert normalized==old,'Only the Ruins target geometry/paging dispatch may change'
assert hashlib.sha256((root/'cartao/RPGPOKET/artes.pak').read_bytes()).hexdigest()=='14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56'
print('PASS: exact package6 rules/Save30/epoch/costs/OTA/666-asset pack; only Ruins input geometry and transient cityPage dispatch change; background replaces same flash slot, authored UI +18144 flash bytes, +0 PSRAM allocation')
