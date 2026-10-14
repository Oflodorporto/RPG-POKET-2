#pragma once
namespace tradeUi {
template<class C>void button(C& c,int x,int y,int w,const char* s,int h=40,bool enabled=true){
 bagSkin::crop(c,x<120?20:124,263,96,35,x,y,w,h);
 const int size=int(strlen(s))*12+1<=w-8?2:1;
 touchLabel(c,x+4,y+(h-8*size)/2,w-8,s,enabled?0xffff:UI_MUTED,size);
}
template<class C>void icon(C& c,unsigned id){
 if(rpg::tradeGear(id)){auto img=gear_icons[rpg::gearFamily(id-19)];for(int y=0;y<56;++y)for(int x=0;x<56;++x)if(img[y*56+x]!=SPRITE_KEY)c.fillRect(92+x,55+y,1,1,img[y*56+x]);}
 else if(id==8){c.drawRect(92,55,56,56,UI_GOLD);for(int y=0;y<36;++y){int half=y<18?y/2+1:(35-y)/2+1;c.fillRect(120-half,64+y,2*half+1,1,y<18?0x7e7f:0x349f);}c.fillRect(118,72,3,15,0xcfff);}
 else if(id==7)magicSprite(c,campArt::bedroll,48,48,92,55,56,56);
 else if(id<2)magicSprite(c,dungeonArt::props[id?7:6],32,32,92,55,56,56);
 else {c.drawRect(92,55,56,56,UI_GOLD);if(id==2){c.fillRect(102,68,36,26,0xc4eb);c.drawRect(102,68,36,26,UI_GOLD);for(int i=0;i<4;++i)c.fillRect(108+i*7,73,2,16,0x8b26);}
 else if(id==3){c.fillRect(101,64,38,36,STORY_PAPER);c.drawRect(101,64,38,36,0x8b26);for(int i=0;i<22;++i)c.fillRect(108+i,70+i,2,2,0x8b26);c.fillRect(127,89,5,5,UI_RED);}
 else if(id==4){c.drawRect(107,66,26,28,UI_GOLD);c.fillRect(115,60,8,9,UI_GOLD);c.fillRect(114,74,12,14,UI_GREEN);}
 else if(id==6){c.drawRect(104,67,13,13,UI_MUTED);c.fillRect(116,75,23,4,UI_MUTED);c.fillRect(133,76,4,12,UI_MUTED);}
 else {c.fillRect(103,82,25,10,0x8b26);c.fillRect(125,72,10,22,UI_MUTED);c.drawRect(102,81,26,12,UI_GOLD);}}
}
}
template<class Canvas>void drawTrading(Canvas& c,const rpg::Game& g,const ViewState& v,unsigned frame){
 drawBackdrop(c,bg_market);c.setTextWrap(false);if(!drawStoryPaper(c,v,frame))return;char b[64];
 auto label=[&](int y,const char* s,int size=1,uint16_t color=STORY_INK){c.setTextColor(color);c.setTextSize(size);c.setCursor((240-int(strlen(s))*6*size)/2,y);c.print(s);};
 auto button=[&](int x,int y,int w,const char* text,int h=40,bool enabled=true){tradeUi::button(c,x,y,w,text,h,enabled);};
 bool list=v.page==Page::TradeList,receipt=v.page==Page::TradeReceipt;unsigned id=list?rpg::tradeOffer(g,v.saleIndex):v.sale.item;
 label(20,list?"VENDER ITENS":receipt?"VENDA CONCLUIDA":"CONFIRMAR VENDA",2);snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);label(41,b);
 if(!rpg::tradable(id)){label(125,"Sem itens comuns para vender.");button(14,272,212,"Voltar");return;}
 tradeUi::icon(c,id);
 label(119,rpg::tradeName(id));unsigned unit=list?rpg::tradeUnit(g,id):v.sale.unit;snprintf(b,sizeof(b),"Unidade: %u ouro",unit);label(137,b);
 if(list){snprintf(b,sizeof(b),"Estoque: %u / item %u de %u",rpg::tradeStock(g,id),unsigned(v.saleIndex)+1,rpg::tradeCount(g));label(155,*v.message?v.message:b,1,*v.message?0xa000:STORY_INK);button(14,178,102,"Anterior");button(124,178,102,"Proximo");button(14,222,212,"Escolher quantidade");button(14,272,212,"Voltar");}
 else if(receipt){snprintf(b,sizeof(b),"Vendidos: %u",v.sale.quantity);label(158,b);snprintf(b,sizeof(b),"Recebido: %lu ouro",(unsigned long)(uint32_t(unit)*v.sale.quantity));label(182,b,1,0x0320);label(211,"Salvo no personagem.");button(14,272,212,"Continuar");}
 else {snprintf(b,sizeof(b),"Quantidade: %u de %u",v.sale.quantity,rpg::saleMaximum(g,id));label(153,b);snprintf(b,sizeof(b),"Total: %lu ouro",(unsigned long)(uint32_t(unit)*v.sale.quantity));label(165,b,1,0x0320);button(14,174,102,"- 1",40,v.sale.quantity>1);button(124,174,102,"+ 1",40,v.sale.quantity<rpg::saleMaximum(g,id));button(14,220,212,"Maximo",32,rpg::saleMaximum(g,id)>0);if(*v.message){c.fillRect(12,253,216,17,UI_PANEL);snprintf(b,sizeof(b),"%.36s",v.message);label(257,b,1,UI_RED);}button(14,272,102,"Cancelar");button(124,272,102,"Confirmar");}
}

// The same amount selector is used for every purchase, including unique items.
// Preview quotes below are local values for read-only host/first-frame rendering.
template<class Canvas>void drawPurchase(Canvas& c,const rpg::Game& g,const ViewState& v,unsigned frame){
 drawBackdrop(c,bg_market);c.setTextWrap(false);if(!drawStoryPaper(c,v,frame))return;
 char b[64];auto label=[&](int y,const char* s,int size=1,uint16_t color=STORY_INK){c.setTextColor(color);c.setTextSize(size);c.setCursor((240-int(strlen(s))*6*size)/2,y);c.print(s);};
 const bool receipt=v.page==Page::PurchaseReceipt;auto q=v.purchase;
 if(!q.open&&!receipt){unsigned id=v.page==Page::GoodsBuy?2+rpg::localGood(g.city):v.page==Page::CrystalBuy?8:v.page==Page::CampKit?7:v.page==Page::GearBuy?19+v.itemId:v.choice;
   q.item=id;q.stock=rpg::purchaseStock(g,id);q.unit=rpg::purchaseUnit(g,id);q.quantity=1;
 }
 const unsigned id=q.item,maximum=rpg::purchaseMaximum(g,id),total=unsigned(q.unit)*q.quantity;
 label(20,receipt?"COMPRA CONCLUIDA":"CONFIRMAR COMPRA",2);snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);label(41,b);
 tradeUi::icon(c,id);label(119,rpg::purchaseName(id));snprintf(b,sizeof(b),"%u ouro cada / bolsa: %u/%u",q.unit,rpg::purchaseStock(g,id),rpg::purchaseLimit(id));label(137,b);
 if(receipt){snprintf(b,sizeof(b),"Comprados: %u",q.quantity);label(158,b);snprintf(b,sizeof(b),"Pago: %u ouro",total);label(182,b,1,0x0320);label(211,"Salvo no personagem.");tradeUi::button(c,14,272,212,"Continuar");return;}
 snprintf(b,sizeof(b),"Quantidade: %u de %u",q.quantity,maximum);label(153,b);
 snprintf(b,sizeof(b),"Total: %u ouro",total);label(165,b,1,0x0320);
 tradeUi::button(c,14,174,102,"- 1",40,q.quantity>1);tradeUi::button(c,124,174,102,"+ 1",40,q.quantity<maximum);
 tradeUi::button(c,14,220,212,"Maximo",32,maximum>0);
 c.fillRect(12,253,216,17,UI_PANEL);
 if(rpg::tradeGear(id)){auto preview=rpg::gearPreview(g,id-19);snprintf(b,sizeof(b),"ATQ %d>%d DEF %d>%d MP %u>%u",rpg::effectiveAttack(g),rpg::effectiveAttack(preview),rpg::effectiveDefense(g),rpg::effectiveDefense(preview),g.p.maxmp,preview.p.maxmp);label(257,b,1,UI_GOLD);}
 else label(257,id==2?"Uma racao por dia de viagem.":id==3||id==4?"Uma unidade usada por viagem.":id==7?"Item unico: um kit por heroi.":id==8?"Cristais nao podem ser vendidos.":"Limite: seu ouro e a bolsa.",1,UI_GOLD);
 if(*v.message){c.fillRect(12,253,216,17,UI_PANEL);snprintf(b,sizeof(b),"%.36s",v.message);label(257,b,1,UI_RED);}
 tradeUi::button(c,14,272,102,"Cancelar");tradeUi::button(c,124,272,102,"Comprar",40,q.open&&q.quantity&&q.quantity<=maximum);
}
