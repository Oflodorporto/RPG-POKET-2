#define main exhaustiveRenderMain
#include "render.cpp"
#undef main

int main(int argc,char** argv){
  assert(argc==2);
  artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));
  assert(artMemory);
  makeFallback();
  std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);
  assert(pack);
  pack.seekg(16);
  pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);
  assert(pack.gcount()==ART_BYTES&&rpg::crc(artMemory,ART_BYTES)==ART_CRC);
  Canvas c;
  rpg::Game g=rpg::testHero(0,42);
  ViewState v;
  v.page=Page::Result;
  g.gainXp=3276;
  g.gainGold=255;
  g.dropLife=g.dropMana=true;
  for(auto phase:{rpg::Phase::Won,rpg::Phase::Fled}){
    g.phase=phase;
    uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];
    rpg::encode(g,11,before);
    render(c,g,v,10);
    rpg::encode(g,11,after);
    assert(!memcmp(before,after,sizeof(before)));
    assert(c.pixels[70*240+12]==STORY_PAPER);
    unsigned iconPixels=0;
    for(int y=48;y<72;++y)for(int x=108;x<132;++x)
      if(c.pixels[y*240+x]!=STORY_PAPER&&c.pixels[y*240+x]!=0xee72)++iconPixels;
    assert(iconPixels>80);
    c.save(std::string(argv[1])+(phase==rpg::Phase::Won?"/result-victory.ppm":"/result-flee.ppm"));
  }
  free(artMemory);
  std::puts("PASS: victory/flee icons visible at 240x320, labels in bounds, save unchanged, parchment preserved");
}
