from pathlib import Path
import hashlib,re,subprocess,zipfile,json,os,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'tools/sd_diagnostic/SD_Diagnostico/SD_Diagnostico.ino').read_text()
for forbidden in ['Preferences','nvs_set','nvs_flash_erase','esp_ota','SD.remove','SD.rename','FILE_WRITE','FILE_APPEND','SD.format']:
 assert forbidden not in s,forbidden
assert 'O_WRONLY|O_CREAT|O_EXCL' in s and 'O_WRONLY|O_APPEND' in s
assert 'SD.begin(41,SPI,hz,"/sd",1,false)' in s
assert 'SPI.begin(39,40,38)' in s and 'Arduino_HWSPI bus(42,45,39,38,40)' in s
assert '||!ssid[0]){privateInputFailed=true;' in s
assert 'WIFI_STORAGE_RAM' in s and 'WiFi.persistent(false)' in s
assert 'tls.useBuiltinCACertBundle()' in s and 'setInsecure' not in s
# Verify exclusive creation protects existing content using the same OS flags contract.
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'report.txt';p.write_bytes(b'old')
 try: fd=os.open(p,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
 except FileExistsError: pass
 else: os.close(fd);raise AssertionError('exclusive open overwrote file')
 assert p.read_bytes()==b'old'
probe=s.split('void existingReadbackRun() {',1)[1].split('bool uniqueDirectory()',1)[0]
for forbidden in ['http.','exclusiveFile(','::write','SD.mkdir','screen(','lcd.begin','configTime(','networkRun(']:assert forbidden not in probe,forbidden
assert probe.count('verifyPath(')==4 and 'connectPrivate(false)' in probe
assert 'prior_errors=' in probe and 'P REMOUNT cycle2' in probe
assert 'if(!secureTime)return true;configTime' in s
fixture=ROOT/'tools/sd_diagnostic/fixtures/transfer_fixture.bin'
assert fixture.stat().st_size==6661456
assert hashlib.sha256(fixture.read_bytes()).hexdigest()=='a59c72f71490c5d10a68e5e48c49821712916c9eef671bf7f6bb907b5e7638f0'
subprocess.run(['python3','tools/sd_diagnostic/package.py'],cwd=ROOT,check=True)
with zipfile.ZipFile(ROOT/'build/sd_diagnostic/RPG_POKET_Diagnostico_SD_v2.zip') as z:
 prefix='RPG_POKET_Diagnostico_SD/'
 for p,h in json.loads(z.read(prefix+'SHA256SUMS.json')).items():assert hashlib.sha256(z.read(prefix+p)).hexdigest()==h
 for p in (ROOT/'firmware/RPG_POKET_2/src/GFX').rglob('*'):
  if p.is_file():assert z.read(prefix+'SD_Diagnostico/src/GFX/'+str(p.relative_to(ROOT/'firmware/RPG_POKET_2/src/GFX')))==p.read_bytes()
print('PASS: diagnostic file-isolation guards, exclusive-create OS contract, exact fixture and ZIP hashes, unchanged bundled LCD dependency.')
