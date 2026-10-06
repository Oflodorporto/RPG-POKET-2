from pathlib import Path
import gzip
p=Path('cartao/RPGPOKET/artes.pak')
if not p.exists():p.write_bytes(gzip.decompress(Path(str(p)+'.gz').read_bytes()))
