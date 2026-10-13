#!/usr/bin/env python3
"""Rebuild the approved compact flash-only bag texture using stdlib only."""
import base64,gzip,hashlib,json,struct,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];folder=root/'assets/bag-touch1'
m=json.loads((folder/'provenance.json').read_text());raw=gzip.decompress(base64.b64decode((folder/'skin.bin.gz.b64').read_text()))
assert len(raw)==38432 and hashlib.sha256(raw).hexdigest()==m['source_sha256']
palette=struct.unpack('<16H',raw[:32]);plane=raw[32:]
reference=b''.join(struct.pack('<H',palette[(plane[i//2]>>(0 if i&1 else 4))&15]) for i in range(240*320))
assert hashlib.sha256(reference).hexdigest()==m['reference_rgb565_sha256']
text='#pragma once\n#include <stdint.h>\n// Generated from assets/bag-touch1; opaque high-nibble-first4bpp, in flash.\nnamespace bagSkin {\ninline constexpr uint16_t palette[16]={'+','.join(map(str,palette))+'};\ninline constexpr uint8_t plane[38400]={\n'+',\n'.join(','.join(map(str,plane[i:i+64])) for i in range(0,len(plane),64))+'\n};\nstatic_assert(sizeof(plane)==240*320/2,"Packed skin geometry");\n}\n'
out=root/'firmware/RPG_POKET_2/BagSkinData.h'
if '--check' in sys.argv:assert out.read_text()==text
else:out.write_text(text)
print('PASS: approved bag skin38432 bytes, palette/plane/reference hashes; external art unchanged')
