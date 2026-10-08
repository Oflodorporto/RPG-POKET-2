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
