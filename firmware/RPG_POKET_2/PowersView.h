#pragma once
namespace powersUi {
constexpr scenicUi::Rect previous={14,114,102,28},next={124,114,102,28},use={14,240,212,32},back={14,278,212,34};
inline unsigned count(unsigned cls){return cls==0?7:cls==1?4:cls==2?4:cls==3?3:0;}
inline bool passive(unsigned cls,unsigned index){return (cls==0&&index%7>=5)||(cls==2&&index%4>=2)||(cls==3&&index%3>=1);}
inline rpg::Action action(unsigned cls,unsigned index){static const rpg::Action mage[]={rpg::Action::MagicMissile,rpg::Action::BurningHands,rpg::Action::ShieldSpell,rpg::Action::ScorchingRay,rpg::Action::Fireball};static const rpg::Action pal[]={rpg::Action::LayHands,rpg::Action::SacredWeapon,rpg::Action::TurnUndead,rpg::Action::DivineSmite};static const rpg::Action fighter[]={rpg::Action::SecondWind,rpg::Action::ActionSurge};return cls==0?mage[index%5]:cls==1?pal[index%4]:cls==2?fighter[index%2]:rpg::Action::RagePower;}
inline const char* description(rpg::Action a,unsigned line){if(a==rpg::Action::DivineSmite)return line%2?"3 MP so no acerto; corpo a corpo":"+2d8; morto-vivo recebe +1d8";static const char* text[][2]={{"3 dardos: 1d4+1 cada","Nao erra; ignora defesa"},{"Fogo: 3d6 no alvo","Teste reduz dano a metade"},{"Protege do proximo golpe","Reduz 75%; usa seu turno"},{"3 raios: 2d6 por acerto","Cada raio testa acerto"},{"Fogo: 8d6 no alvo","Teste reduz dano a metade"},{"Reserva de cura: 5 x nivel","Restaura HP ate a reserva"},{"+Carisma no dano, minimo 1","Dura 3 ataques; 1 canalizar"},{"Morto-vivo perde 2 turnos","Compartilha uso de canalizar"},{"Cura 1d10 + nivel","Nao gasta seu ataque"},{"Uma acao extra neste turno","Nv 17: 2 usos por descanso"},{"+2/3/4 dano por ataque","3 rodadas; resiste fisico"}};return text[unsigned(a)-unsigned(rpg::Action::MagicMissile)][line%2];}
}
template<class C>void drawPowers(C& c,const rpg::Game& g,const ViewState& v){
 drawBackdrop(c,bg_character);scenicHeading(c,g.p.cls==0?"GRIMORIO":"PODERES");char b[64];
 if(v.page==Page::OathConfirm){panelLabel(c,12,65,216,"JURAMENTO DA DEVOCAO",UI_GOLD);panelLabel(c,12,92,216,"Honestidade. Coragem. Compaixao.");panelLabel(c,12,113,216,"Proteja os inocentes de Aeldra.");panelLabel(c,12,151,216,"Esta escolha e permanente.");panelLabel(c,12,172,216,"Libera Arma sagrada e Expulsar.");panelLabel(c,12,199,216,v.message,UI_RED);scenicPanel(c,14,240,212,32);panelLabel(c,18,251,204,"Firmar juramento",UI_GOLD);scenicPanel(c,14,278,212,34);panelLabel(c,18,290,204,"Cancelar");return;}
 snprintf(b,sizeof(b),"%s / Nv %u / MP %u",rpg::className(g.p.cls),g.p.level,g.p.mp);panelLabel(c,10,52,220,b);
 panelHero(c,g,93,67,54,42);
 unsigned count=powersUi::count(g.p.cls);
 if(!g.dndProgression||!count){panelLabel(c,10,155,220,!g.dndProgression?"Heroi antigo: poderes preservados":"Novos poderes em preparo",UI_GOLD);scenicPanel(c,14,278,212,34);panelLabel(c,18,290,204,"Voltar");return;}
 auto a=powersUi::action(g.p.cls,v.powerIndex);scenicPanel(c,14,114,102,28);panelLabel(c,18,123,94,"< Anterior");scenicPanel(c,124,114,102,28);panelLabel(c,128,123,94,"Proximo >");
 if(g.p.cls==0&&powersUi::passive(g.p.cls,v.powerIndex)){
   bool mastery=v.powerIndex%7==6;unsigned level=mastery?18:10;
   scenicPanel(c,10,148,220,86);panelLabel(c,14,155,212,mastery?"MAESTRIA EM MAGIA":"EVOCACAO APRIMORADA",UI_GOLD);
   snprintf(b,sizeof(b),"Nv %u / passivo",level);panelLabel(c,14,171,212,b);
   if(mastery){panelLabel(c,14,187,212,"Misseis e raios: 0 MP");panelLabel(c,14,203,212,"Mesmo dano; gasta seu turno");panelLabel(c,14,218,212,"Outras magias mantem o custo",UI_BLUE);}
   else{snprintf(b,sizeof(b),"Dano extra atual: +%u",rpg::evocationBonus(g,rpg::Action::MagicMissile));panelLabel(c,14,187,212,b);panelLabel(c,14,203,212,"+mod INT uma vez por magia");panelLabel(c,14,218,212,"Sem bonus no cajado ou escudo",UI_BLUE);}
   scenicPanel(c,14,240,212,32);panelLabel(c,18,251,204,g.p.level>=level?"Passivo ativo":"Ainda nao desbloqueado",g.p.level>=level?UI_GREEN:UI_MUTED);
   scenicPanel(c,14,278,212,34);panelLabel(c,18,290,204,"Voltar");return;
 }
 if(g.p.cls==3&&powersUi::passive(g.p.cls,v.powerIndex)){
   bool persist=v.powerIndex%3==2;unsigned level=persist?15:9;
   scenicPanel(c,10,148,220,86);panelLabel(c,14,155,212,persist?"FURIA PERSISTENTE":"CRITICO BRUTAL",UI_GOLD);
   snprintf(b,sizeof(b),"Nv %u / passivo / sem mana",level);panelLabel(c,14,171,212,b);
   if(persist){panelLabel(c,14,187,212,"Furia dura ate o fim da luta");panelLabel(c,14,203,212,"Vitoria, derrota ou fuga encerra");panelLabel(c,14,218,212,"Ainda precisa ativar a Furia",UI_BLUE);}
   else{panelLabel(c,14,187,212,"Nv 9/13/17: +1d6/2d6/3d6");panelLabel(c,14,203,212,"So no critico que acerta");snprintf(b,sizeof(b),"Atual: +%ud6; uma vez por acao",rpg::brutalDice(g));panelLabel(c,14,218,212,b,UI_BLUE);}
   scenicPanel(c,14,240,212,32);panelLabel(c,18,251,204,g.p.level>=level?"Passivo ativo":"Ainda nao desbloqueado",g.p.level>=level?UI_GREEN:UI_MUTED);
   scenicPanel(c,14,278,212,34);panelLabel(c,18,290,204,"Voltar");return;
 }
 if(powersUi::passive(g.p.cls,v.powerIndex)){
   bool survivor=v.powerIndex%4==3;unsigned level=survivor?18:3;
   scenicPanel(c,10,148,220,86);panelLabel(c,14,155,212,survivor?"SOBREVIVENTE":"TRILHA DO CAMPEAO",UI_GOLD);
   snprintf(b,sizeof(b),"Nv %u / passivo / sem mana",level);panelLabel(c,14,171,212,b);
   if(survivor){snprintf(b,sizeof(b),"Inicio do turno: +%d HP",std::max(0,5+rpg::abilityMod(g.attributes[2])));panelLabel(c,14,187,212,b);panelLabel(c,14,203,212,"Se HP esta na metade ou menos");panelLabel(c,14,218,212,"Nao revive; Surto nao repete",UI_BLUE);}
   else{panelLabel(c,14,187,212,"Nv 3: 20% / Nv 15: 30%");panelLabel(c,14,203,212,"Ataque e tecnica com arma");snprintf(b,sizeof(b),"Chance atual: %u%%",rpg::weaponCriticalChance(g));panelLabel(c,14,218,212,b,UI_BLUE);}
   scenicPanel(c,14,240,212,32);panelLabel(c,18,251,204,g.p.level>=level?"Passivo ativo":"Ainda nao desbloqueado",g.p.level>=level?UI_GREEN:UI_MUTED);
   scenicPanel(c,14,278,212,34);panelLabel(c,18,290,204,"Voltar");return;
 }
 scenicPanel(c,10,148,220,86);panelLabel(c,14,155,212,rpg::powerName(a),UI_GOLD);
 snprintf(b,sizeof(b),"Nv %u / %u MP / %u de %u",rpg::powerLevel(a),rpg::powerCost(g,a),unsigned(v.powerIndex)%count+1,count);panelLabel(c,14,171,212,b);
 panelLabel(c,14,187,212,powersUi::description(a,0));panelLabel(c,14,203,212,a==rpg::Action::RagePower&&rpg::persistentRage(g)?"Ate fim da luta; resiste fisico":powersUi::description(a,1));
 if(g.p.cls==1){if(a==rpg::Action::LayHands)snprintf(b,sizeof(b),"Cura restante: %u HP",rpg::layRemaining(g));else if(a==rpg::Action::DivineSmite)snprintf(b,sizeof(b),g.p.level>=11?"Aprimorada: +1d8 por golpe":"Nv 11: aprimorada +1d8/golpe");else snprintf(b,sizeof(b),"Devocao: %s / canalizar %u",g.oath?"SIM":"NAO",g.channelSpent?0:1);panelLabel(c,14,218,212,b,UI_BLUE);}
 else if(g.p.cls==2){snprintf(b,sizeof(b),a==rpg::Action::SecondWind?"Folego: %u / 1":"Surto: %u / %u",a==rpg::Action::SecondWind?1-g.windSpent:rpg::surgeUses(g)-g.surgeSpent,rpg::surgeUses(g));panelLabel(c,14,218,212,b,UI_BLUE);}
 else if(g.p.cls==3){if(rpg::persistentRage(g)){if(g.p.level>=20)snprintf(b,sizeof(b),"Usos sem limite / %s",g.rageTurns?"ATIVA":"inativa");else snprintf(b,sizeof(b),"Furias: %u / %u; %s",rpg::rageUses(g)-g.rageSpent,rpg::rageUses(g),g.rageTurns?"ATIVA":"inativa");}else if(g.p.level>=20)snprintf(b,sizeof(b),"Sem limite / ativa %u rodadas",g.rageTurns);else snprintf(b,sizeof(b),"Furias: %u / %u; ativa %u",rpg::rageUses(g)-g.rageSpent,rpg::rageUses(g),g.rageTurns);panelLabel(c,14,218,212,b,UI_BLUE);}
 else panelLabel(c,14,218,212,rpg::masteredSpell(g,a)?"Maestria: conjuracao sem mana":"Mana adaptada / alvo unico",UI_BLUE);
 scenicPanel(c,14,240,212,32);const char* err=rpg::powerError(g,a);bool swear=g.p.cls==1&&!g.oath&&g.p.level>=3&&g.phase==rpg::Phase::Home&&!g.tripStage&&!g.campStage&&!g.dungeonFlags&&g.clubStage!=1&&g.clubStage!=2&&(a==rpg::Action::SacredWeapon||a==rpg::Action::TurnUndead);
 panelLabel(c,18,251,204,*v.message?v.message:swear?"Firmar juramento >":err?err:"Usar poder",*v.message?UI_GOLD:swear||!err?UI_GREEN:UI_MUTED);
 scenicPanel(c,14,278,212,34);panelLabel(c,18,290,204,"Voltar");
}

