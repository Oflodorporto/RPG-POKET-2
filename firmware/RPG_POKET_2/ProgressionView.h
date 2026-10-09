#pragma once
template<class C>void drawProgression(C& c,const rpg::Game& g,const ViewState& v){
 drawBackdrop(c,bg_character);scenicHeading(c,v.page==Page::AttributeInfo?"ESCOLHA DE EVOLUCAO":"TRILHA DA CLASSE");char b[64];
 auto label=[&](int y,const char* s,uint16_t color=UI_WHITE){panelLabel(c,12,y,216,s,color);};
 auto button=[&](int x,int y,int w,const char* s){scenicPanel(c,x,y,w,32);panelLabel(c,x+4,y+11,w-8,s,UI_GOLD);};
 snprintf(b,sizeof(b),"%s / nivel %u",rpg::className(g.p.cls),g.p.level);label(50,b,UI_GOLD);
 if(v.page==Page::AttributeInfo){unsigned i=std::min(6u,unsigned(v.attributeIndex));bool feat=i==6;rpg::Game preview=g;const char* err=feat?rpg::learnTough(preview):rpg::improveAttribute(preview,i);
  scenicPanel(c,8,76,224,155);label(86,feat?"TALENTO RESISTENTE":rpg::abilityName(i),UI_GOLD);label(105,rpg::abilityEffect(i));
  snprintf(b,sizeof(b),"Pontos: %u / custo %u",rpg::advancementPoints(g),feat?2:1);label(126,b,UI_GREEN);
  if(!feat){snprintf(b,sizeof(b),"Valor %u > %u / bonus %+d > %+d",g.attributes[i],preview.attributes[i],rpg::abilityMod(g.attributes[i]),rpg::abilityMod(preview.attributes[i]));label(147,b);}
  snprintf(b,sizeof(b),"HP max %u > %u",g.p.maxhp,preview.p.maxhp);label(168,b);snprintf(b,sizeof(b),"ATQ %d > %d / DEF %d > %d",rpg::effectiveAttack(g),rpg::effectiveAttack(preview),rpg::effectiveDefense(g),rpg::effectiveDefense(preview));label(187,b);snprintf(b,sizeof(b),"MP max %u > %u",g.p.maxmp,preview.p.maxmp);label(208,b);
  label(240,*v.message?v.message:err?err:"Confirme para gastar os pontos",err||*v.message?UI_RED:UI_GREEN);
  if(!feat)label(258,"O bonus muda nos valores pares",UI_MUTED);button(14,278,102,"Voltar");button(124,278,102,"Confirmar");return;
 }
 if(!g.dndProgression){label(90,"Heroi antigo: regras preservadas",UI_GOLD);label(118,"Esta trilha vale para novos herois.");button(14,278,212,"Voltar");return;}
 unsigned level=std::min(20u,std::max(1u,unsigned(v.evolutionLevel)));
 if(g.p.level==20)label(65,"Nivel maximo / jornada continua",UI_GREEN);else {snprintf(b,sizeof(b),"XP %lu / %u / faltam %lu",(unsigned long)g.p.xp,rpg::xpNeeded(g),(unsigned long)(rpg::xpNeeded(g)-std::min<uint32_t>(g.p.xp,rpg::xpNeeded(g))));label(65,b);}
 panelBar(c,14,81,212,g.p.level==20?1:g.p.xp,g.p.level==20?1:rpg::xpNeeded(g),UI_GREEN);
 button(14,94,102,"< Anterior");button(124,94,102,"Proximo >");
 scenicPanel(c,8,133,224,111);snprintf(b,sizeof(b),"Nv %u / %s / XP %lu",level,level<=g.p.level?"ALCANCADO":"FUTURO",(unsigned long)rpg::dndXp[level-1]);label(141,b,level<=g.p.level?UI_GREEN:UI_GOLD);
 rpg::Game before=g,after=g;before.p.level=std::max(1u,level-1);after.p.level=level;
 snprintf(b,sizeof(b),level==1?"Recursos iniciais da sua classe":"+%u HP / +%u MP / +%u ATQ base",rpg::hpPerLevel(after),rpg::totalMana(after)-rpg::totalMana(before),g.p.atk<255?1:0);label(157,b);
 auto benefits=rpg::levelBenefits(g,level);for(unsigned i=0;i<std::min(5u,benefits.count);++i)label(174+13*i,benefits.lines[i],UI_GOLD);
 if(!benefits.count)label(183,"Crescimento de HP, MP e ataque");
 label(248,"Previa: seus atributos atuais",UI_MUTED);button(14,263,102,"Atributos");button(124,263,102,"Poderes");label(303,"Voltar ao personagem",UI_GOLD);
}
