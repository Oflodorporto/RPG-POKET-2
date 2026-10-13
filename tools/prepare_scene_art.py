#!/usr/bin/env python3
"""Append typed backgrounds to the immutable published636 package; no artwork generation."""
from dataclasses import dataclass
from pathlib import Path
import argparse,base64,gzip,hashlib,json,struct,sys,zlib
sys.dont_write_bytecode=True
import prepare_enemy_art as old
ROOT=old.ROOT;FOLDER=ROOT/'assets/scenes1'
CATALOG=ROOT/'firmware/RPG_POKET_2/AssetCatalog.h'
DESCRIPTORS=ROOT/'firmware/RPG_POKET_2/SceneArtCatalog.h'
FALLEN=ROOT/'firmware/RPG_POKET_2/FallenArtCatalog.h'
REPORT=FOLDER/'integration_manifest.json'
def sha(data):return hashlib.sha256(data).hexdigest()
@dataclass
class Result:
    pack:bytes
    metadata:list
    fallback:bytes
    catalog:str
    descriptors:str
    fallen_descriptors:str
    report:dict
def build(base=None):
    base=base or old.build();old.verify_generated(base)
    provenance=json.loads((FOLDER/'provenance.json').read_text())
    payload=bytearray(base.pack[16:]);metadata=list(base.metadata);fallback=bytearray(base.fallback);records=[]
    old.require(len(metadata)==636 and sha(base.pack)=='5416f6d507601bc131751e89106332f291f8d1d09642faa264a9b61215dd43f1','Published636 identity changed')
    for entry in provenance['scenes']:
        source=(FOLDER/entry['source_file']).read_bytes();old.require(sha(source)==entry['source_sha256'],'Scene source mismatch')
        encoded=(FOLDER/entry['export_file']).read_bytes();old.require(sha(encoded)==entry['export_sha256'],'Scene export mismatch')
        data=gzip.decompress(base64.b64decode(encoded));old.require(len(data)==77312==entry['bytes'] and sha(data)==entry['decoded_sha256'],'Scene codec length/hash mismatch')
        palette,plane=data[:512],data[512:];reference=b''.join(palette[v*2:v*2+2] for v in plane)
        old.require(sha(reference)==entry['reference_rgb565_sha256'],'Scene pixel reference mismatch')
        plane_id=len(metadata);metadata.append((len(payload),240,320,len(fallback),16,16,1));payload.extend(plane)
        fallback.extend(plane[(y*320//16)*240+x*240//16] for y in range(16) for x in range(16))
        palette_id=len(metadata);metadata.append((len(payload),128,2,len(fallback),128,2,2));payload.extend(palette);fallback.extend(palette)
        records.append(dict(name=entry['name'],plane=plane_id,palette=palette_id,reference_rgb565_sha256=entry['reference_rgb565_sha256']))
    old.require(payload[:len(base.pack)-16]==base.pack[16:] and metadata[:636]==base.metadata and fallback[:len(base.fallback)]==base.fallback,'Published assets changed')
    layers=[]
    fallen=provenance['fallen'];old.require(fallen['width']==160 and fallen['height']==80 and fallen['transparency_rgb565']=='f81f','Fallen geometry/key mismatch')
    old.require([r['name'] for r in fallen['layers']]==['base','human','elf','dwarf','orc','mage','paladin','fighter','barbarian'],'Fallen layer order changed')
    for entry in fallen['layers']:
        encoded=(FOLDER/entry['export_file']).read_bytes();old.require(sha(encoded)==entry['export_sha256'],'Fallen export mismatch')
        data=gzip.decompress(base64.b64decode(encoded));old.require(len(data)==38400==entry['bytes'] and sha(data)==entry['decoded_sha256'],'Fallen length/hash mismatch')
        color,mask=data[:25600],data[25600:];old.require(max(mask)<=3 and sha(color)==entry['color_sha256'] and sha(mask)==entry['mask_sha256'],'Fallen mask/reference mismatch')
        color_id=len(metadata);metadata.append((len(payload),160,80,len(fallback),1,1,2));payload.extend(color);fallback.extend(b'\x1f\xf8')
        mask_id=len(metadata);metadata.append((len(payload),160,80,len(fallback),1,1,1));payload.extend(mask);fallback.append(0)
        layers.append(dict(name=entry['name'],color=color_id,mask=mask_id,color_sha256=entry['color_sha256'],mask_sha256=entry['mask_sha256']))
    old.require(len(payload)+153600+13760<=8*1024*1024-512*1024,'Known PSRAM reserve below512KiB; physical budget still unmeasured')
    crc=zlib.crc32(payload);pack=struct.pack('<4sIII',b'PKA1',len(payload),crc,len(metadata))+payload
    catalog=old.catalog_text(metadata,fallback,crc).replace('tools/prepare_enemy_art.py','tools/prepare_scene_art.py')
    descriptors='#pragma once\n#include "AssetCatalog.h"\nnamespace sceneArt {\nstruct Descriptor {unsigned plane,palette;};\nenum class Id:unsigned {'+','.join(r['name'].replace('-','_') for r in records)+',Count};\nconstexpr Descriptor catalog[]={'+','.join('{%d,%d}'%(r['plane'],r['palette']) for r in records)+'};\nconstexpr bool validCatalog(){for(auto d:catalog){if(d.plane>=ART_COUNT||d.palette>=ART_COUNT)return false;auto a=assetMeta[d.plane],p=assetMeta[d.palette];if(a.w!=240||a.h!=320||a.elem!=1||p.w!=128||p.h!=2||p.elem!=2)return false;}return true;}\nstatic_assert(validCatalog(),"Typed scene geometry");\n}\n'
    fallen_descriptors='#pragma once\n#include "AssetCatalog.h"\nnamespace fallenArt {\nconstexpr unsigned W=160,H=80,PIXELS=W*H;\nstruct Descriptor{unsigned color,mask;};\nconstexpr Descriptor layers[]={'+','.join('{%d,%d}'%(r['color'],r['mask']) for r in layers)+'};\nconstexpr bool validCatalog(){if(sizeof(layers)/sizeof(layers[0])!=9)return false;for(auto d:layers){if(d.color>=ART_COUNT||d.mask>=ART_COUNT)return false;auto c=assetMeta[d.color],m=assetMeta[d.mask];if(c.w!=W||c.h!=H||c.elem!=2||m.w!=W||m.h!=H||m.elem!=1)return false;}return true;}\nstatic_assert(validCatalog(),"Typed fallen hero layers");\n}\n'
    report=dict(schema=1,baseline_sha256=sha(base.pack),unchanged_assets=636,assets=len(metadata),payload_bytes=len(payload),file_bytes=len(pack),crc32=f'{crc:08x}',pack_sha256=sha(pack),added_bytes=len(payload)-len(base.pack)+16,known_psram_bytes=len(payload)+153600+13760,hardware_measured=False,scenes=records,fallen=layers)
    return Result(bytes(pack),metadata,bytes(fallback),catalog,descriptors,fallen_descriptors,report)
def files(result):return {CATALOG:result.catalog,DESCRIPTORS:result.descriptors,FALLEN:result.fallen_descriptors,REPORT:old.json_text(result.report)}
def verify_generated(result):
    for path,value in files(result).items():old.require(path.exists() and path.read_text()==value,'Stale scene output: '+str(path.relative_to(ROOT)))
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');parser.add_argument('--output',type=Path);args=parser.parse_args();result=build()
    if args.check:verify_generated(result)
    else:
        for path,value in files(result).items():path.write_text(value)
    if args.output:args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(result.pack)
    print(json.dumps(result.report,indent=2))
if __name__=='__main__':main()
