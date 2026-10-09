#include "LegacySave.h"
#include "../firmware/RPG_POKET_2/Bestiary.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
using namespace rpg;
struct Storage{std::map<std::string,std::vector<uint8_t>> files;bool fail=false;Read read(const char* key,uint8_t* out){auto i=files.find(key);if(i==files.end())return Read::Missing;memcpy(out,i->second.data(),SAVE_SIZE);return Read::Ok;}bool write(const char* key,const uint8_t* b){if(fail)return false;files[key]=std::vector<uint8_t>(b,b+SAVE_SIZE);return true;}};
int main(){uint8_t b[SAVE_SIZE],c[SAVE_SIZE];uint32_t seq;Game h;auto g=create(0,31);assert(!g.seenEnemies&&!bestiaryCount(g));for(unsigned id=0;id<18;++id){g.seenEnemies=1u<<id;encode(g,7,b);assert(get16(b,4)==22&&get16(b,6)==128&&decode(b,h,seq)==Decode::Ok&&h.seenEnemies==g.seenEnemies);encode(h,7,c);assert(!memcmp(b,c,SAVE_SIZE));assert(bestiaryCount(h)==1&&bestiaryFirst(h)==id&&bestiaryStep(h,id,1)==id&&bestiaryStep(h,id,-1)==id);}
 g.seenEnemies=(1u<<18)-1;encode(g,7,b);assert(decode(b,h,seq)==Decode::Ok&&bestiaryCount(h)==18);for(unsigned id=0;id<18;++id){assert(bestiaryStep(g,id,1)==(id+1)%18);assert(bestiaryStep(g,id,-1)==(id+17)%18);}
 // All old formats remain supported; no speculative catalogue unlocked at home.
 for(unsigned v=12;v<=19;++v){g=create(0,31);encode(g,7,b);legacyFormat(b,v);put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Ok&&!h.seenEnemies);}
 g=create(0,31);assert(begin(g,0)&&bestiaryKnown(g,0));encode(g,7,b);legacyFormat(b,19);put32(b,124,crc(b,124));assert(decode(b,h,seq)==Decode::Ok&&h.seenEnemies==1);g=h;g.phase=Phase::Fled;home(g);assert(g.seenEnemies==1);auto rng=g.randomState;assert(!begin(g,3)&&g.seenEnemies==1&&g.randomState==rng);
 // Sparse catalogue skips unknown foes, wraps, and is read-only.
 g.seenEnemies=(1u<<2)|(1u<<17);encode(g,7,b);assert(bestiaryStep(g,2,1)==17&&bestiaryStep(g,17,1)==2&&bestiaryStep(g,2,-1)==17);encode(g,7,c);assert(!memcmp(b,c,SAVE_SIZE));g.seenEnemies=1u<<18;assert(!valid(g));
 Storage store;Journal<Storage> journal(store);g=create(0,31);assert(journal.load(h)==Load::Empty);assert(journal.save(g));assert(begin(g,0));store.fail=true;assert(!journal.save(g));Journal<Storage> reboot(store);assert(reboot.load(h)==Load::Ok&&!h.seenEnemies);store.fail=false;assert(journal.save(g));assert(reboot.load(h)==Load::Ok&&h.seenEnemies==1&&h.phase==Phase::Hero);auto seed=h.randomState;assert(journal.save(h));assert(reboot.load(g)==Load::Ok&&g.seenEnemies==1&&g.randomState==seed);
 puts("PASS: save20 discovery18 bits,128bytes,legacy12..19/resume, encounter unlock, sparse navigation, atomic failure/retry/read-only.");}
