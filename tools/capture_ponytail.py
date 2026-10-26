from pathlib import Path
import struct, zlib

import sys
ROOT = Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path("build/ponytail").resolve()

def read(path):
    magic, dims, maximum, pixels = path.read_bytes().split(b'\n', 3)
    assert magic == b'P6' and maximum == b'255'
    w, h = map(int, dims.split())
    assert len(pixels) == w * h * 3
    return w, h, pixels

def png(path, w, h, pixels):
    assert len(pixels) == w * h * 3
    def chunk(kind, body):
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))
    rows = b''.join(b'\0' + pixels[y*w*3:(y+1)*w*3] for y in range(h))
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB',w,h,8,2,0,0,0)) + chunk(b'IDAT',zlib.compress(rows,9)) + chunk(b'IEND',b''))

for folder in ('render-before','render-after'):
    for source in (ROOT/folder).glob('*.ppm'):
        w,h,p = read(source)
        png(source.with_suffix('.png'),w,h,p)

sheet = bytearray(480 * 960 * 3)
for row in range(3):
    for col, folder in enumerate(('render-before','render-after')):
        w,h,p = read(ROOT/folder/f'equipment-page-{row+1}.ppm')
        assert (w,h)==(240,320)
        for y in range(h):
            start=((row*320+y)*480+col*240)*3
            sheet[start:start+720]=p[y*720:(y+1)*720]
png(ROOT/'equipment-before-after.png',480,960,sheet)
w,h,p = read(ROOT/'render-after/cleanup-portrait.ppm')
assert (w,h)==(240,320)
landscape = bytearray(320*240*3)
for y in range(240):
    for x in range(320):
        src=(x*240+239-y)*3
        dst=(y*320+x)*3
        landscape[dst:dst+3]=p[src:src+3]
png(ROOT/'cleanup-retry.png',320,240,landscape)
pw,ph,pp = read(ROOT/'render-after/pvp-cleanup.ppm')
assert (pw,ph)==(240,320)
png(ROOT/'pvp-cleanup.png',pw,ph,pp)
print('PASS: lossless PNGs of actual host-rendered frames; left before/right after, no artwork regeneration')
