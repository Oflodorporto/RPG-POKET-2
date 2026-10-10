from pathlib import Path
import gzip,struct,zlib
p=Path('cartao/RPGPOKET/artes.pak')
base=gzip.decompress(Path(str(p)+'.gz').read_bytes())
assert base[:4]==b'PKA1'
delta=Path('cartao/RPGPOKET/paineis1.bin.gz')
if delta.exists():
 payload=base[16:16+6158144]+gzip.decompress(delta.read_bytes())
 assert len(payload)==6772544
 assert zlib.crc32(payload)==0x1a8982c3
 p.write_bytes(struct.pack('<4sIII',b'PKA1',len(payload),zlib.crc32(payload),425)+payload)
else:p.write_bytes(base)

# Pacote 1 - Origem e rostos: append portraits after the validated panels payload.
portraits=Path('cartao/RPGPOKET/origens1.bin.gz')
if portraits.exists():
 previous=p.read_bytes();assert struct.unpack('<4sIII',previous[:16])==(b'PKA1',6772544,0x1a8982c3,425)
 payload=previous[16:]+gzip.decompress(portraits.read_bytes())
 assert len(payload)==6846528 and zlib.crc32(payload)==0x19aa0797
 p.write_bytes(struct.pack('<4sIII',b'PKA1',len(payload),zlib.crc32(payload),459)+payload)

# Post-campaign island: append only; all459 previous asset offsets remain stable.
island=Path('cartao/RPGPOKET/ilha1.bin.gz.b64')
if island.exists():
 import base64
 previous=p.read_bytes();assert struct.unpack('<4sIII',previous[:16])==(b'PKA1',6846528,0x19aa0797,459)
 payload=previous[16:]+gzip.decompress(base64.b64decode(island.read_text()))
 assert len(payload)==6910016 and zlib.crc32(payload)==0x415cb17d
 p.write_bytes(struct.pack('<4sIII',b'PKA1',len(payload),zlib.crc32(payload),480)+payload)

# REGIONAL_WORLD1: preserve all480 previous entries, append8 poses.
regional=Path('cartao/RPGPOKET/mundovivo1.bin.gz.b64')
if regional.exists():
 import base64
 previous=p.read_bytes();assert struct.unpack('<4sIII',previous[:16])==(b'PKA1',6910016,0x415cb17d,480)
 payload=previous[16:]+gzip.decompress(base64.b64decode(regional.read_text()))
 assert len(payload)==6991936 and zlib.crc32(payload)==0x53dcc3a5
 p.write_bytes(struct.pack('<4sIII',b'PKA1',len(payload),zlib.crc32(payload),488)+payload)
