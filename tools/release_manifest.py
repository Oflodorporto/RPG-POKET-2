from pathlib import Path
import os,re,struct,hashlib,json,shutil,zlib
header=Path('firmware/RPG_POKET_2/ReleaseVersion.h').read_text()
version=os.environ.get('RELEASE_VERSION') or re.search(r'FW_VERSION="([^"]+)"',header).group(1)
assert re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]{0,63}',version), 'Versao invalida'
header=Path('firmware/RPG_POKET_2/ReleaseVersion.h').read_text()
compiled_version=re.search(r'FW_VERSION="([^"]+)"',header).group(1)
build=int(re.search(r'FW_BUILD=(\d+)u',header).group(1))
assert version==compiled_version, 'A versao da Release deve ser igual a ReleaseVersion.h'
firmware=Path('build/RPG_POKET_2.ino.bin').read_bytes()
assert 0<len(firmware)<=0x300000, 'Firmware nao cabe em uma particao OTA'
artpath=Path('cartao/RPGPOKET/artes.pak')
if not artpath.exists():
 import gzip
 artpath.write_bytes(gzip.decompress(Path(str(artpath)+'.gz').read_bytes()))
art=artpath.read_bytes()
magic,size,crc,count=struct.unpack('<4sIII',art[:16]);assert magic==b'PKA1' and size==len(art)-16 and zlib.crc32(art[16:])==crc
catalog=Path('firmware/RPG_POKET_2/AssetCatalog.h').read_text()
assert int(re.search(r'ART_BYTES=(\d+)',catalog).group(1))==size
assert int(re.search(r'ART_CRC=0x([a-f0-9]+)',catalog).group(1),16)==crc
assert int(re.search(r'ART_COUNT=(\d+)',catalog).group(1))==count
out=Path('release');out.mkdir(exist_ok=True)
(out/'firmware.bin').write_bytes(firmware);(out/'artes.pak').write_bytes(art)
base=f'https://github.com/Oflodorporto/RPG-POKET-2/releases/download/v{version}/'
manifest={'schema':2,'version':version,'build':build,'board':'Waveshare-29667','partition':'app3M_fat9M_16MB','firmware':{'url':base+'firmware.bin','bytes':len(firmware),'sha256':hashlib.sha256(firmware).hexdigest()},'art':{'url':base+'artes.pak','bytes':len(art),'sha256':hashlib.sha256(art).hexdigest(),'payload_crc32':f'{crc:08x}','assets':count},'minimum_psram_bytes':size+153600,'installation':'confirmed-device-ota','save_format':7,'writes_save_format':8,'reads_save_formats':[1,2,3,4,5,6,7,8]}
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('PASS: firmware fits OTA; art header, size and CRC checked; release hashes generated')
