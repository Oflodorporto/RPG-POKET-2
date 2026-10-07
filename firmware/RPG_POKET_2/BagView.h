#pragma once
// Native UI shapes for the bag, existing flash item art for its contents.
template<class Canvas> void drawBag(Canvas& c,const rpg::Game& g,const ViewState& v){
  c.fillRect(0,0,240,320,UI_INK);c.setTextWrap(false);
  auto text=[&](int x,int y,const char* s,uint16_t color=UI_WHITE){c.setTextSize(1);c.setTextColor(color);c.setCursor(x,y);c.print(s);};
  auto center=[&](int y,const char* s,uint16_t color=UI_WHITE){text((240-int(strlen(s))*6)/2,y,s,color);};
  // Leather backpack with handle, straps, flap and buckle, built as UI geometry.
  c.drawRect(18,7,16,12,UI_GOLD);c.fillRect(11,16,30,29,0x8b26);c.drawRect(11,16,30,29,UI_GOLD);
  c.fillRect(8,25,3,14,UI_GOLD);c.fillRect(41,25,3,14,UI_GOLD);c.fillRect(13,18,26,9,0xa409);c.fillRect(23,25,6,7,UI_GOLD);
  text(55,14,"BOLSA DO HEROI",UI_GOLD);char b[48];snprintf(b,sizeof(b),"Ouro: %lu",(unsigned long)g.p.gold);text(55,30,b);
  snprintf(b,sizeof(b),"HP %u/%u   MP %u/%u",g.p.hp,g.p.maxhp,g.p.mp,g.p.maxmp);center(54,b);
  bool gear=v.page==Page::BagGear;unsigned count=rpg::gearOwnedCount(g.owned);
  const char* names[]={"Vida","Mana","Cristal","Racao","Mapa","Sorte"};
  const unsigned quantities[]={g.p.life,g.p.mana,g.crystals,g.rations,g.charts,g.charms};
  for(unsigned i=0;i<6;++i){int x=10+(i%3)*76,y=74+(i/3)*64;
    c.fillRect(x,y,68,58,UI_PANEL);c.drawRect(x,y,68,58,i==v.choice?UI_GOLD:0x3186);
    if(i==v.choice)c.drawRect(x+1,y+1,66,56,UI_GOLD);
    if(gear){unsigned index=v.gearIndex*6+i;if(index<count){auto id=rpg::gearOwnedAt(g.owned,index);auto img=gear_icons[rpg::gearFamily(id)];
        for(int py=0;py<32;++py)for(int px=0;px<32;++px){auto color=img[(py*56/32)*56+px*56/32];if(color!=SPRITE_KEY)c.fillRect(x+18+px,y+4+py,1,1,color);}
        text(x+54,y+35,"1",UI_GREEN);bool equipped=g.equipped[rpg::gearSlot(id)]==id;text(x+4,y+44,equipped?"Equipado":"Guardado",equipped?UI_GREEN:UI_MUTED);
      }else text(x+19,y+26,"Vazio",UI_MUTED);continue;}
    unsigned prop=i==0?6:i==1?7:i==2?4:i==3?0:i==4?5:11;
    const uint16_t* img=dungeonArt::props[prop];
    for(int py=0;py<32;++py)for(int px=0;px<32;++px)if(img[py*32+px]!=0xf81f)c.fillRect(x+18+px,y+4+py,1,1,img[py*32+px]);
    // Supply icons use clear symbols rather than misleading potion/crystal art.
    if(i>=3){c.fillRect(x+16,y+3,36,34,UI_PANEL);
      if(i==3){c.fillRect(x+23,y+10,24,18,0xc4eb);c.drawRect(x+23,y+10,24,18,UI_GOLD);for(int k=0;k<3;++k)c.fillRect(x+27+k*6,y+13,2,8,0x8b26);}
      if(i==4){c.fillRect(x+20,y+8,28,25,0xe6b7);for(int k=0;k<18;++k)c.fillRect(x+25+k,y+12+k/2,2,2,0x8b26);c.fillRect(x+40,y+25,4,4,UI_RED);}
      if(i==5){c.drawRect(x+22,y+8,24,24,UI_GOLD);c.drawRect(x+27,y+13,14,14,UI_GREEN);c.fillRect(x+32,y+3,4,8,UI_GOLD);}
    }
    text(x+4,y+44,names[i]);snprintf(b,sizeof(b),"%u",quantities[i]);text(x+64-int(strlen(b))*6,y+35,b,quantities[i]?UI_GREEN:UI_MUTED);
  }
  unsigned selected=std::min(5u,unsigned(v.choice));
  if(gear){for(int x:{10,162}){c.fillRect(x,200,68,24,UI_PANEL);c.drawRect(x,200,68,24,UI_GOLD);text(x+10,208,x==10?"< Pag":"Pag >");}
    snprintf(b,sizeof(b),"%u/%u",v.gearIndex+1,std::max(1u,(count+5)/6));center(208,b);
  }else {snprintf(b,sizeof(b),"%s x%u",names[selected],quantities[selected]);center(207,b,UI_GOLD);}
  c.fillRect(14,230,212,34,UI_PANEL);c.drawRect(14,230,212,34,UI_GOLD);
  unsigned index=v.gearIndex*6+selected;
  if(gear)snprintf(b,sizeof(b),"%.33s",index<count?rpg::gearName(rpg::gearOwnedAt(g.owned,index)):"Nenhum equipamento neste slot");
  else snprintf(b,sizeof(b),"Equipamentos: %u",count);center(243,b);
  const char* notice=*v.message?v.message:gear?"Selecione o item para equipar":selected==2?"Chave da cripta / entrada nas Ruinas":selected>2?"Consumido na viagem quando preciso":"Selecione e toque em Usar";
  center(269,notice,UI_MUTED);
  for(unsigned i=0;i<2;++i){int x=i?124:14;c.fillRect(x,278,102,40,UI_PANEL);c.drawRect(x,278,102,40,UI_GOLD);text(x+(i&&gear?30:33),294,i?(gear?"Equipar":"Usar"):"Voltar");}
}
