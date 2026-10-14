#pragma once
#include "Camp.h"
#include "Exploration.h"
namespace rpg {
// Volatile quotes only. Gold and stock are committed together by the existing Save30 journal (no quote is serialized).
struct SaleQuote {bool open=false;uint8_t item=0,city=0,stock=0,quantity=1;uint16_t unit=0;};
inline bool tradeSafe(const Game& g){return g.city<4&&g.p.gold<=999999u&&g.phase==Phase::Home&&!g.campStage&&!g.tripStage&&!g.dungeonFlags&&!g.discovery&&!g.campaignStage&&g.eventStage!=2&&g.clubStage!=1&&g.clubStage!=2;}
inline bool tradeGear(unsigned id){return id>=20&&id<20+GEAR_COUNT;}
inline bool tradable(unsigned id){return id<8||tradeGear(id);}
inline const char* tradeName(unsigned id){const char* n[]={"POCAO DE VIDA","POCAO DE MANA","RACAO DE VIAGEM","MAPA DO VIAJANTE","TALISMA DA SORTE","SUCATA","GAZUA","KIT DE DORMIR"};return id<8?n[id]:tradeGear(id)?gearName(id-19):"ITEM PROTEGIDO";}
inline unsigned tradeStock(const Game& g,unsigned id){switch(id){case 0:return g.p.life;case 1:return g.p.mana;case 2:return g.rations;case 3:return g.charts;case 4:return g.charms;case 5:return g.scrap;case 6:return g.gazuas;case 7:return g.sleepKit;default:return tradeGear(id)&&gearOwns(g.owned,id-19);}}
inline unsigned tradeUnit(const Game& g,unsigned id){
 if(!tradable(id))return 0;if(tradeGear(id))return sellPrice(g,id-19);if(id==5)return 2;if(id==7)return 40;
 unsigned price=0,cheapest=65535;
 for(unsigned city=0;city<4;++city){auto local=g;local.city=city;unsigned value=id<2?potionPrice(local,id==1):id==6?gazuaPrice(local):localGood(city)==id-2?goodPrice(city):(id==2?6:id==3?12:25);cheapest=std::min(cheapest,value);if(city==g.city)price=value;}
 return std::min(price/2,cheapest-1);
}
// Only the quantity which fits the gold cap can be selected. Division keeps
// even maliciously large quantities from overflowing or wrapping subtraction.
inline unsigned saleMaximum(const Game& g,unsigned id){if(!tradeSafe(g)||!tradable(id))return 0;if(tradeGear(id)&&g.equipped[gearSlot(id-19)]==id-19)return 0;unsigned unit=tradeUnit(g,id);return unit?std::min<uint32_t>(tradeStock(g,id),(999999u-g.p.gold)/unit):0;}
inline const char* saleError(const Game& g,unsigned id,unsigned quantity){if(!tradeSafe(g))return "Termine a acao atual";if(!tradable(id))return "Item protegido; nao negociavel";if(!quantity||quantity>tradeStock(g,id))return "Quantidade fora do estoque";if(tradeGear(id)&&g.equipped[gearSlot(id-19)]==id-19)return "Retire o equipamento antes";if(quantity>saleMaximum(g,id))return "Limite de ouro; venda menos";return nullptr;}
inline const char* quoteSale(const Game& g,SaleQuote& q,unsigned id){q.open=false;auto err=saleError(g,id,1);if(err)return err;q={true,uint8_t(id),g.city,uint8_t(tradeStock(g,id)),1,uint16_t(tradeUnit(g,id))};return nullptr;}
inline const char* confirmSale(Game& g,SaleQuote& q){
 if(!q.open)return "Venda ja encerrada";if(q.city!=g.city||q.stock!=tradeStock(g,q.item)||q.unit!=tradeUnit(g,q.item))return "Estoque mudou; revise a venda";
 auto err=saleError(g,q.item,q.quantity);if(err)return err;uint32_t value=uint32_t(q.unit)*q.quantity;
 switch(q.item){case 0:g.p.life-=q.quantity;break;case 1:g.p.mana-=q.quantity;break;case 2:g.rations-=q.quantity;break;case 3:g.charts-=q.quantity;break;case 4:g.charms-=q.quantity;break;case 5:g.scrap-=q.quantity;break;case 6:g.gazuas-=q.quantity;break;case 7:g.sleepKit=false;break;default:g.owned&=~(1u<<(q.item-20));break;}
 g.p.gold+=value;q.open=false;return nullptr;
}
inline unsigned tradeCount(const Game& g){unsigned n=0;for(unsigned id=0;id<38;++id)n+=tradable(id)&&tradeStock(g,id)>0;return n;}
inline unsigned tradeOffer(const Game& g,unsigned index){for(unsigned id=0;id<38;++id)if(tradable(id)&&tradeStock(g,id)&&!index--)return id;return 255;}
// Canonical commerce IDs: 0..7 are saleable supplies, 8 is a protected
// dungeon crystal (purchase only), and 20..37 are unique equipment.
struct PurchaseQuote {bool open=false;uint8_t item=0,city=0,stock=0,quantity=1;uint16_t unit=0;};
inline const char* purchaseName(unsigned id){return id==8?"CRISTAL DE DUNGEON":tradeName(id);}
inline unsigned purchaseStock(const Game& g,unsigned id){return id==8?g.crystals:tradeStock(g,id);}
inline unsigned purchaseLimit(unsigned id){return id<2?99:id>=2&&id<=4?goodLimit(id-2):id==6||id==8?9:id==7||tradeGear(id)?1:0;}
inline unsigned purchaseUnit(const Game& g,unsigned id){if(id<2)return potionPrice(g,id==1);if(id>=2&&id<=4)return goodPrice(g.city);if(id==6)return gazuaPrice(g);if(id==7)return 80;if(id==8)return 300;return tradeGear(id)?cityGearPrice(g,id-19):0;}
// A paused escort already permits buying the missing camp kit. Preserve
// that narrow recovery path without enabling other trading during a mission.
inline bool purchaseSafe(const Game& g,unsigned id){
 return tradeSafe(g)||(id==7&&escortCampAllowed(g)&&g.city<4&&g.p.gold<=999999u&&g.phase==Phase::Home&&!g.campStage&&!g.tripStage&&!g.dungeonFlags&&!g.discovery&&!g.campaignStage&&g.clubStage!=1&&g.clubStage!=2);
}
inline const char* purchaseAvailability(const Game& g,unsigned id){
 if(!purchaseSafe(g,id))return "Termine a acao atual";
 if(!purchaseLimit(id))return "Item indisponivel para compra";
 if(id>=2&&id<=4&&id!=unsigned(localGood(g.city))+2)return "Item ausente nesta cidade";
 if(id==8&&g.city!=1)return "Compre cristais nas Ruinas";
 if(tradeGear(id)){if(!cityStock(g,id-19))return "Item ausente nesta cidade";if(!gearAllowed(id-19,g.p.cls,g.p.level))return "Classe ou nivel insuficiente";}
 return nullptr;
}
inline unsigned purchaseMaximum(const Game& g,unsigned id){
 if(purchaseAvailability(g,id))return 0;
 unsigned stock=purchaseStock(g,id),limit=purchaseLimit(id),unit=purchaseUnit(g,id);
 return stock>=limit||!unit?0:std::min(limit-stock,unsigned(g.p.gold/unit));
}
inline const char* purchaseError(const Game& g,unsigned id,unsigned quantity){
 if(auto err=purchaseAvailability(g,id))return err;
 if(!quantity)return "Selecione uma quantidade";
 unsigned stock=purchaseStock(g,id),limit=purchaseLimit(id);
 if(stock>=limit)return limit==1?"Voce ja possui este item":"Bolsa cheia para este item";
 if(quantity>limit-stock)return "Quantidade excede a bolsa";
 unsigned unit=purchaseUnit(g,id);
 if(!unit||quantity>g.p.gold/unit)return "Ouro insuficiente para o lote";
 return nullptr;
}
inline const char* quotePurchase(const Game& g,PurchaseQuote& q,unsigned id){
 q.open=false;if(auto err=purchaseAvailability(g,id))return err;
 q={true,uint8_t(id),g.city,uint8_t(purchaseStock(g,id)),1,uint16_t(purchaseUnit(g,id))};return nullptr;
}
inline const char* confirmPurchase(Game& g,PurchaseQuote& q){
 if(!q.open)return "Compra ja encerrada";
 if(q.city!=g.city||q.stock!=purchaseStock(g,q.item)||q.unit!=purchaseUnit(g,q.item))return "Estoque mudou; revise a compra";
 if(auto err=purchaseError(g,q.item,q.quantity))return err;
 const uint32_t total=uint32_t(q.unit)*q.quantity;
 // Every check precedes every mutation: either the whole lot or nothing.
 switch(q.item){case 0:g.p.life+=q.quantity;break;case 1:g.p.mana+=q.quantity;break;case 2:g.rations+=q.quantity;break;case 3:g.charts+=q.quantity;break;case 4:g.charms+=q.quantity;break;case 6:g.gazuas+=q.quantity;break;case 7:g.sleepKit=true;break;case 8:g.crystals+=q.quantity;break;default:g.owned|=1u<<(q.item-20);break;}
 g.p.gold-=total;q.open=false;return nullptr;
}

}
