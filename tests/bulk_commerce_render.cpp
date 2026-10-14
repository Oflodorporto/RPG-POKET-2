#define main originalRenderTests
#include "render.cpp"
#undef main
#include <new>
static bool forbidAlloc=false;
#ifdef RPG_TEST_COMMERCE_ALLOCATIONS
extern "C" void* __real_malloc(size_t);extern "C" void* __real_calloc(size_t,size_t);extern "C" void* __real_realloc(void*,size_t);
extern "C" void* __wrap_malloc(size_t n){assert(!forbidAlloc);return __real_malloc(n);}
extern "C" void* __wrap_calloc(size_t n,size_t s){assert(!forbidAlloc);return __real_calloc(n,s);}
extern "C" void* __wrap_realloc(void* p,size_t n){assert(!forbidAlloc);return __real_realloc(p,n);}
#endif
void* operator new(std::size_t n){assert(!forbidAlloc);if(void* p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}
int main(int argc,char** argv){using namespace rpg;assert(argc==2);artMemory=static_cast<uint8_t*>(malloc(ART_BYTES));assert(artMemory);makeFallback();std::ifstream pack("cartao/RPGPOKET/artes.pak",std::ios::binary);assert(pack);pack.seekg(16);pack.read(reinterpret_cast<char*>(artMemory),ART_BYTES);assert(pack.gcount()==ART_BYTES&&crc(artMemory,ART_BYTES)==ART_CRC);Canvas c;unsigned frames=0;
 auto draw=[&](const Game& g,const ViewState& v){uint8_t before[SAVE_SIZE],after[SAVE_SIZE];encode(g,17,before);forbidAlloc=true;render(c,g,v,10);forbidAlloc=false;static uint16_t shown[240*320];memcpy(shown,c.pixels,sizeof(shown));auto opening=v;opening.paperAnimate=true;opening.paperStartFrame=0;for(unsigned frame=0;frame<7;++frame){forbidAlloc=true;render(c,g,opening,frame);forbidAlloc=false;assert(!memcmp(shown,c.pixels,sizeof(shown)));}encode(g,17,after);assert(!memcmp(before,after,sizeof(before)));++frames;};
 for(unsigned city=0;city<4;++city)for(unsigned cls=0;cls<4;++cls)for(unsigned item=0;item<38;++item)for(unsigned state=0;state<4;++state){auto g=create(cls,17);g.p.xp=dndXp[19];levelUp(g);g.city=city;g.p.gold=state==0?0:state==1?31:999999;g.rations=state==3?10:0;g.p.life=g.p.mana=state==3?99:0;g.charts=g.charms=g.gazuas=g.crystals=state==3?9:0;g.sleepKit=state==3;ViewState v;v.page=Page::Buy;
  if(quotePurchase(g,v.purchase,item))continue;v.purchase.quantity=purchaseMaximum(g,item);const char* err=purchaseError(g,item,v.purchase.quantity);v.message=err?err:"";draw(g,v);v.purchase.quantity=1;draw(g,v);if(purchaseMaximum(g,item)){assert(!confirmPurchase(g,v.purchase));v.page=Page::PurchaseReceipt;v.message="";draw(g,v);}
 }
 auto save=[&](const char* name,const Game& g,const ViewState& v){draw(g,v);c.save(std::string(argv[1])+"/"+name+".ppm");};
 Game g=create(2,71);g.p.gold=240;g.rations=0;ViewState v;v.page=Page::GoodsBuy;assert(!quotePurchase(g,v.purchase,2));v.purchase.quantity=10;save("comprar-10-racoes",g,v);assert(!confirmPurchase(g,v.purchase));v.page=Page::PurchaseReceipt;save("recibo-10-racoes",g,v);
 v=ViewState{};v.page=Page::TradeConfirm;assert(!quoteSale(g,v.sale,2));v.sale.quantity=7;save("vender-7-racoes",g,v);assert(!confirmSale(g,v.sale));v.page=Page::TradeReceipt;save("recibo-venda",g,v);
 g=create(0,71);g.p.gold=35;v=ViewState{};v.page=Page::Buy;assert(!quotePurchase(g,v.purchase,0));v.purchase.quantity=purchaseMaximum(g,0);save("pocoes-limite-ouro",g,v);
 g.p.gold=9999;g.p.life=98;assert(!quotePurchase(g,v.purchase,0));v.purchase.quantity=1;save("pocoes-limite-bolsa",g,v);
 g.rations=10;v.page=Page::GoodsBuy;assert(!quotePurchase(g,v.purchase,2));v.purchase.quantity=0;v.message=purchaseError(g,2,1);save("racoes-bolsa-cheia",g,v);
 g=create(0,71);g.p.gold=999990;g.p.life=99;v=ViewState{};v.page=Page::TradeConfirm;assert(!quoteSale(g,v.sale,0));v.sale.quantity=saleMaximum(g,0);save("venda-limite-ouro",g,v);
 g=create(2,71);g.p.gold=1000;v=ViewState{};v.page=Page::GearBuy;v.itemId=7;assert(!quotePurchase(g,v.purchase,26));save("equipamento-unico",g,v);
 v.page=Page::CampKit;assert(!quotePurchase(g,v.purchase,7));save("kit-unico",g,v);
 g.city=1;v.page=Page::CrystalBuy;assert(!quotePurchase(g,v.purchase,8));v.purchase.quantity=3;save("comprar-cristais",g,v);
 v.page=Page::Buy;assert(!quotePurchase(g,v.purchase,6));v.purchase.quantity=9;save("comprar-gazuas",g,v);
 g.city=2;v.page=Page::GoodsBuy;assert(!quotePurchase(g,v.purchase,3));v.purchase.quantity=9;save("comprar-mapas",g,v);
 g.city=3;assert(!quotePurchase(g,v.purchase,4));v.purchase.quantity=9;save("comprar-talismas",g,v);
 free(artMemory);artMemory=nullptr;printf("PASS: bulk commerce %u frames, all city/class/item/zero/full states; bounded text, read-only saves, no new allocations; commerce controls visible on every opening frame.\n",frames);
}
