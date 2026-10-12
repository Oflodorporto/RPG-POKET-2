#pragma once
template<class Canvas>void drawTrading(Canvas& c,const rpg::Game& g,const ViewState& v,unsigned frame){
 drawBackdrop(c,bg_market);c.setTextWrap(false);if(!drawStoryPaper(c,v,frame))return;char b[64];
 auto label=[&](int y,const char* s,int size=1,uint16_t color=STORY_INK){c.setTextColor(color);c.setTextSize(size);c.setCursor((240-int(strlen(s))*6*size)/2,y);c.print(s);};
 auto button=[&](int x,int y,int w,const char* s,int h=40){c.fillRect(x,y,w,h,UI_PANEL);c.drawRect(x,y,w,h,UI_GOLD);c.setTextColor(UI_WHITE);int size=int(strlen(s))*12<=w-6?2:1;c.setTextSize(size);c.setCursor(x+(w-int(strlen(s))*6*size)/2,y+(h-8*size)/2);c.print(s);};
 bool list=v.page==Page::TradeList,receipt=v.page==Page::TradeReceipt;unsigned id=list?rpg::tradeOffer(g,v.saleIndex):v.sale.item;
 label(12,list?"VENDER ITENS":receipt?"VENDA CONCLUIDA":"CONFIRMAR VENDA",2);snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);label(35,b);
 if(!rpg::tradable(id)){label(125,"Sem itens comuns para vender.");button(14,272,212,"Voltar");return;}
 if(rpg::tradeGear(id)){auto img=gear_icons[rpg::gearFamily(id-19)];for(int y=0;y<56;++y)for(int x=0;x<56;++x)if(img[y*56+x]!=SPRITE_KEY)c.fillRect(92+x,55+y,1,1,img[y*56+x]);}
 else if(id==7)magicSprite(c,campArt::bedroll,48,48,92,55,56,56);
 else if(id<2)magicSprite(c,dungeonArt::props[id?7:6],32,32,92,55,56,56);
 else {c.drawRect(92,55,56,56,UI_GOLD);if(id==2){c.fillRect(102,68,36,26,0xc4eb);c.drawRect(102,68,36,26,UI_GOLD);for(int i=0;i<4;++i)c.fillRect(108+i*7,73,2,16,0x8b26);}
 else if(id==3){c.fillRect(101,64,38,36,STORY_PAPER);c.drawRect(101,64,38,36,0x8b26);for(int i=0;i<22;++i)c.fillRect(108+i,70+i,2,2,0x8b26);c.fillRect(127,89,5,5,UI_RED);}
 else if(id==4){c.drawRect(107,66,26,28,UI_GOLD);c.fillRect(115,60,8,9,UI_GOLD);c.fillRect(114,74,12,14,UI_GREEN);}
 else if(id==6){c.drawRect(104,67,13,13,UI_MUTED);c.fillRect(116,75,23,4,UI_MUTED);c.fillRect(133,76,4,12,UI_MUTED);}
 else {c.fillRect(103,82,25,10,0x8b26);c.fillRect(125,72,10,22,UI_MUTED);c.drawRect(102,81,26,12,UI_GOLD);}}
 label(119,rpg::tradeName(id));unsigned unit=list?rpg::tradeUnit(g,id):v.sale.unit;snprintf(b,sizeof(b),"Unidade: %u ouro",unit);label(137,b);
 if(list){snprintf(b,sizeof(b),"Estoque: %u / item %u de %u",rpg::tradeStock(g,id),unsigned(v.saleIndex)+1,rpg::tradeCount(g));label(155,b);button(14,178,102,"Anterior");button(124,178,102,"Proximo");button(14,222,212,"Escolher quantidade");button(14,272,212,"Voltar");}
 else if(receipt){snprintf(b,sizeof(b),"Vendidos: %u",v.sale.quantity);label(158,b);snprintf(b,sizeof(b),"Recebido: %lu ouro",(unsigned long)(uint32_t(unit)*v.sale.quantity));label(182,b,1,0x0320);label(211,"Salvo no personagem.");button(14,272,212,"Continuar");}
 else {snprintf(b,sizeof(b),"Qtd: %u / estoque: %u",v.sale.quantity,v.sale.stock);label(153,b);snprintf(b,sizeof(b),"Total: %lu ouro",(unsigned long)(uint32_t(unit)*v.sale.quantity));label(165,b,1,0x0320);button(14,174,102,"- 1");button(124,174,102,"+ 1");button(14,220,212,"Todos",32);label(44,v.message,1,0xa000);button(14,272,102,"Cancelar");button(124,272,102,"Confirmar");}
}
