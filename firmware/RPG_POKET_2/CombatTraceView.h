#pragma once
// Pure formatting and rendering: all numbers come from the resolved RAM receipt.
namespace combatTraceUi {
constexpr unsigned linesPerPage=8;
inline bool surface(Page p,const rpg::Game& g){return p==Page::Battle||p==Page::DungeonVictory||p==Page::Result||p==Page::CampaignResult||p==Page::EventResult||(p==Page::Dungeon&&g.phase!=rpg::Phase::Home);}
inline bool detailsHit(int x,int y){return x>=4&&x<110&&y>=0&&y<24;}
inline bool skipHit(int x,int y){return x>=130&&x<216&&y>=0&&y<24;}
inline bool touchAllowed(const ViewState& v,const rpg::Game& g,bool effect,bool journey,int x,int y){return !journey&&(v.traceOpen||(surface(v.page,g)&&(detailsHit(x,y)||skipHit(x,y)))||(!effect&&!(v.page==Page::Battle&&g.phase!=rpg::Phase::Hero)));}
inline const char* title(const rpg::CombatTrace& t){if(t.enemy)return "ACAO DO INIMIGO";auto a=rpg::Action(t.action);return rpg::powerAction(a)?rpg::powerName(a):a==rpg::Action::Attack?"Ataque basico":a==rpg::Action::Offensive?rpg::skillName(t.actorClass,false):a==rpg::Action::Defensive?rpg::skillName(t.actorClass,true):a==rpg::Action::Life?"Pocao de HP":a==rpg::Action::Mana?"Pocao de MP":"Fuga";}
inline bool selfTarget(const rpg::CombatTrace& t){auto a=rpg::Action(t.action);return t.healing||a==rpg::Action::Defensive||a==rpg::Action::ShieldSpell||a==rpg::Action::RagePower||a==rpg::Action::SacredWeapon||a==rpg::Action::ActionSurge||a==rpg::Action::Flee;}
inline const char* diceName(unsigned g){using D=rpg::TraceDice;switch(D(g)){case D::Missile:return "Misseis";case D::Ray1:return "Raio 1";case D::Ray2:return "Raio 2";case D::Ray3:return "Raio 3";case D::Brutal:return "Brutal";case D::Smite:return "Punicao";case D::Wind:return "Folego";default:return "Dano";}}
// Walk rows without a heap buffer or keeping generated text in ViewState.
template<class Emit>unsigned lines(const rpg::CombatTrace& t,Emit emit){
 unsigned row=0;char b[96];auto out=[&](const char* s){emit(row++,s);};
 if(!t.valid){out("Resumo salvo; dados indisponiveis.");out("Os dados nao sao gravados no save.");out("Nenhuma rolagem foi refeita.");return row;}
 for(unsigned i=0;i<t.count;++i){auto s=t.steps[i];using K=rpg::TraceStep;
 switch(s.kind){
 case K::AttackStat:snprintf(b,sizeof(b),"ATQ base %d; atributo %+d; prof %+d",s.a,s.b,s.c);out(b);break;
 case K::Equipment:snprintf(b,sizeof(b),"Forja +%d; arma +%d = ATQ %d",s.a,s.b,s.c);out(b);break;
 case K::Weapon:snprintf(b,sizeof(b),"1d3=%d; variacao %d-1=%d",s.b,s.b,s.b-1);out(b);snprintf(b,sizeof(b),"max(1,%d+%d-%d/3)=%d",s.a,s.b-1,s.c,std::max(1,int(s.a)+s.b-1-s.c/3));out(b);break;
 case K::Critical:snprintf(b,sizeof(b),"Critico d100=%d <=%d? %s",s.a,s.b,s.a<=s.b?"SIM":"NAO");out(b);if(s.a<=s.b){snprintf(b,sizeof(b),"Critico: +metade (piso) = %d",s.c);out(b);}break;
 case K::Attacks:snprintf(b,sizeof(b),"%d golpes: %d x %d = %d",s.a,s.b,s.a,s.c);out(b);if(s.a>1)out("Mesma rolagem agregada da arma.");break;
 case K::Technique:snprintf(b,sizeof(b),"Tecnica: %d x %d%% = %d",s.b,s.a,s.c);out(b);break;
 case K::Rage:case K::Sacred:case K::Evocation:snprintf(b,sizeof(b),"%s: %d + %d = %d",s.kind==K::Rage?"Furia":s.kind==K::Sacred?"Sagrada":"INT (1x)",s.b,s.a,s.c);out(b);break;
 case K::Dice:{unsigned start=s.a,n=s.b,group=unsigned(s.c)>>8,sides=s.c&255,total=0;snprintf(b,sizeof(b),"%s %ud%u: ",diceName(group),n,sides);for(unsigned d=0;d<n&&start+d<t.diceCount;++d){char num[8];unsigned value=t.dice[start+d];total+=value;snprintf(num,sizeof(num),"%s%u",d?"+":"",value);strncat(b,num,sizeof(b)-strlen(b)-1);}char sum[12];snprintf(sum,sizeof(sum),"=%u",total);strncat(b,sum,sizeof(b)-strlen(b)-1);out(b);if(group==unsigned(rpg::TraceDice::Missile)){snprintf(b,sizeof(b),"+1 por dardo: %u + %u = %u",total,n,total+n);out(b);}break;}
 case K::RayHit:{bool hit=s.a!=1&&(s.a==20||s.a+s.b>=s.c);snprintf(b,sizeof(b),"Raio d20 %d%+d vs %d: %s",s.a,s.b,s.c,hit?"ACERTO":"ERRO");out(b);if(s.a==1||s.a==20)out(s.a==1?"1 natural: erra este raio.":"20 natural: acerta este raio.");break;}
 case K::Save:snprintf(b,sizeof(b),"Save inimigo: d20 %d + %d = %d",s.a,s.b,s.a+s.b);out(b);snprintf(b,sizeof(b),"CD %d: %s",s.c,s.a+s.b>=s.c?"passou; metade do dano":"falhou; dano integral");out(b);break;
 case K::Half:snprintf(b,sizeof(b),"Metade: %d / 2 = %d (piso)",s.a,s.b);out(b);break;
 case K::Fatigue:snprintf(b,sizeof(b),"Fadiga: max(1,%d-%d)=%d",s.b,s.a,s.c);out(b);out("Uma reducao por acao ofensiva.");break;
 case K::Dodge:snprintf(b,sizeof(b),"Esquiva d100=%d <=%d? %s",s.a,s.b,s.a<=s.b?"SIM":"NAO");out(b);break;
 case K::Heavy:snprintf(b,sizeof(b),"Golpe forte: %d x 3/2 = %d",s.a,s.b);out(b);break;
 case K::Guard:snprintf(b,sizeof(b),"Defesa %d%%: %d -> %d (teto)",s.a,s.b,s.c);out(b);break;
 case K::Resistance:snprintf(b,sizeof(b),"Furia fisica: %d / 2 = %d",s.a,s.b);out(b);break;
 case K::Flee:snprintf(b,sizeof(b),"Fuga d100=%d <=%d? %s",s.a,s.b,s.a<=s.b?"SIM":"NAO");out(b);break;
 case K::Fixed:snprintf(b,sizeof(b),"Bonus de nivel: +%d",s.a);out(b);break;
 case K::Restore:snprintf(b,sizeof(b),"Pocao: %d x %d%% = %d",s.a,s.b,s.c);out(b);break;
 case K::Defense:snprintf(b,sizeof(b),"Proximo golpe: bloqueio de %d%%",s.a);out(b);break;
 case K::Skip:out("Expulso: nao atacou nem rolou.");break;
 }
 }
 if(t.damaging||t.healing){snprintf(b,sizeof(b),"%s %u; aplicado %u",t.healing?"Faltava":"HP alvo",t.targetHp,t.applied);out(b);if(t.net>t.applied)out(t.healing?"Cura limitada ao recurso faltante.":"Dano aplicado limitado ao HP.");}
 if(t.mpDrained){snprintf(b,sizeof(b),"Drenagem adicional: -%u MP",t.mpDrained);out(b);}
 if(!t.count&&!t.healing){out("Efeito fixo: nenhum dado usado.");if(t.enemy)out("Inimigo prepara o proximo golpe.");else {auto a=rpg::Action(t.action);if(a==rpg::Action::ShieldSpell||a==rpg::Action::Defensive)out("Defesa aplicada ao proximo golpe.");else if(a==rpg::Action::ActionSurge)out("Uma acao extra; turno preservado.");else if(a==rpg::Action::TurnUndead)out("Morto-vivo perde dois turnos.");else if(a==rpg::Action::SacredWeapon)out("Bonus de Carisma nos 3 ataques.");else if(a==rpg::Action::RagePower)out("Furia ativada; regras preservadas.");}}
 if(t.overflow)out("Detalhes truncados; total preservado.");
 return row;
}
inline unsigned pageCount(const rpg::CombatTrace& t){return std::max(1u,(lines(t,[](unsigned,const char*){})+linesPerPage-1)/linesPerPage);}
template<class C>void text(C& c,int x,int y,const char* s,uint16_t color=UI_WHITE,unsigned size=1){c.setTextSize(size);c.setTextColor(color);c.setCursor(x,y);c.print(s);}
template<class C>void button(C& c,int x,int y,int w,int h,const char* s){c.fillRect(x,y,w,h,UI_PANEL);c.drawRect(x,y,w,h,UI_GOLD);text(c,x+(w-int(strlen(s))*6)/2,y+(h-8)/2,s,UI_GOLD);}
template<class C>void drawDetails(C& c,const rpg::Game& g,const ViewState& v){
 const auto& t=v.traceEnemy?v.enemyTrace:v.heroTrace;bool missingOther=!t.valid&&(v.traceEnemy?v.heroTrace:v.enemyTrace).valid;char b[64];c.fillRect(0,0,240,320,UI_INK);
 text(c,12,12,"DADOS REAIS",UI_GOLD,2);button(c,12,38,102,28,v.traceEnemy?"Heroi":"Heroi *");button(c,126,38,102,28,v.traceEnemy?"Inimigo *":"Inimigo");
 text(c,12,76,t.valid?title(t):missingOther?"SEM REGISTRO":"RESUMO SALVO",UI_GOLD);
 if(t.valid){snprintf(b,sizeof(b),"%u MP / alvo: %s",t.mpCost,t.enemy?(t.healing?"inimigo":"heroi"):selfTarget(t)?"proprio":rpg::enemySpec(t.enemyId).name);text(c,12,91,b);if(t.damaging||t.healing){snprintf(b,sizeof(b),"Bruto %u > final %u > %s %u",t.potential,t.net,t.action==uint8_t(rpg::Action::Mana)?"MP":"HP",t.applied);text(c,12,107,b,t.healing?UI_GREEN:UI_WHITE);}else text(c,12,107,"Efeito aplicado; sem dano.");}
 else if(missingOther){text(c,12,93,"Esta acao ainda nao tem registro.");}
 else {snprintf(b,sizeof(b),"Valor salvo: %u / HP inimigo: %u",g.damage,g.enemyHp);text(c,12,93,b);text(c,12,107,"HP/MP e turno salvos preservados.",UI_MUTED);}
 if(!missingOther)text(c,12,117,"Resultado ja aplicado; so consulta.",UI_MUTED);
 unsigned pages=pageCount(t),page=std::min(unsigned(v.tracePage),pages-1);
 if(missingOther){text(c,12,129,"Nenhum dado foi inventado.");}else lines(t,[&](unsigned i,const char* s){if(i/linesPerPage==page)text(c,12,129+(i%linesPerPage)*14,s);});
 button(c,12,246,62,28,"<");snprintf(b,sizeof(b),"%u / %u",page+1,pages);text(c,101,256,b,UI_GOLD);button(c,166,246,62,28,">");button(c,12,282,216,30,"Voltar");
}
// Tiny fixed control and a brief reveal of actual values, never fake rolling faces.
// Destructor overlays the controls even on early-return result/dungeon pages.
template<class C>struct Overlay {
 C& c;const rpg::Game& g;const ViewState& v;
 ~Overlay(){if(v.traceOpen||!surface(v.page,g))return;button(c,4,0,106,22,"Dados >");
  if(v.effect==Effect::None&&rpg::fatigueCombatPenalty(g)){char b[32];snprintf(b,sizeof(b),"Fadiga %u%% -%u dano",g.fatigue,rpg::fatigueCombatPenalty(g));c.fillRect(114,0,124,22,UI_INK);text(c,116,7,b,UI_GOLD);}
  if(v.effect!=Effect::None){button(c,130,0,86,22,"Pular >");const auto& t=v.combatResult.enemyAction?v.enemyTrace:v.heroTrace;if(t.valid&&t.diceCount){c.fillRect(10,143,220,29,UI_INK);c.drawRect(10,143,220,29,UI_GOLD);char b[64]="Dados reais: ";unsigned ms=v.effectElapsedMs==UINT32_MAX?v.effectFrame*60:v.effectElapsedMs;unsigned n=std::min(unsigned(t.diceCount),1+ms*unsigned(t.diceCount)/480);for(unsigned i=0;i<n;++i){char d[8];snprintf(d,sizeof(d),"%s%u",i?",":"",t.dice[i]);if(strlen(b)+strlen(d)<36)strcat(b,d);}text(c,16,149,b);snprintf(b,sizeof(b),"%s %u  > HP %u",t.healing?"Cura":"Liquido",t.net,t.applied);text(c,16,160,b,UI_GOLD);}}}
};
}
