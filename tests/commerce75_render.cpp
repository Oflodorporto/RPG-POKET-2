#define main originalRenderTests
#include "render.cpp"
#undef main
int main(int argc,char** argv){assert(argc==2);artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);makeFallback();std::ifstream pack("outputs/RPG_POKET_2_0_Waveshare/cartao_preparado/RPGPOKET/artes.pak",std::ios::binary);if(!pack)pack.open("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES);Canvas c;ViewState v;unsigned frames=0;
 for(unsigned city=0;city<4;++city)for(unsigned cls=0;cls<4;++cls)for(unsigned item=0;item<38;++item){if(!rpg::tradable(item))continue;auto g=rpg::create(cls,13);g.p.gold=998000;g.city=city;g.p.life=g.p.mana=99;g.rations=g.charts=g.charms=g.gazuas=g.scrap=9;g.sleepKit=true;g.owned=(1u<<18)-1;assert(!rpg::quoteSale(g,v.sale,item));v.sale.quantity=v.sale.stock;
  for(auto page:{Page::TradeList,Page::TradeConfirm,Page::TradeReceipt}){v.page=page;v.saleIndex=0;for(unsigned i=0;i<rpg::tradeCount(g);++i)if(rpg::tradeOffer(g,i)==item)v.saleIndex=i;v.message="Limite de ouro; venda menos";uint8_t before[rpg::SAVE_SIZE],after[rpg::SAVE_SIZE];rpg::encode(g,4,before);render(c,g,v,10);rpg::encode(g,4,after);assert(!memcmp(before,after,sizeof(before)));++frames;if(city==0&&cls==0&&(item==0||item==7||item==37))c.save(std::string(argv[1])+"/commerce75-"+std::to_string(item)+"-"+std::to_string(int(page))+".ppm");}
 }
 free(artMemory);artMemory=nullptr;printf("PASS:commerce75 %u sale frames/4cities/4classes/26items, text bounds and read-only saves.\n",frames);
}
